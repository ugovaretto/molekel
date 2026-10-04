import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { EigenVistaDocument } from "../src/types";

interface MoldenReference {
  reference: {
    points: [number, number, number][];
    ao: [number, number, number, number][][];
    orbitals: { spin: string; samples: [number, number, number, number][] }[];
    densities: {
      kind: string;
      samples: [number, number, number, number][];
      electrons: number;
    }[];
    overlap: number[];
  };
}

const water = readFileSync(
  path.resolve("../fixtures/molden/water-rhf-ccpvdz.molden"),
);
const upload = {
  name: "water.molden.input",
  mimeType: "text/plain",
  buffer: water,
};

async function ready(page: Page) {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  page.on("dialog", (dialog) => dialog.accept());
}

interface DelayedReadControl {
  releaseDelayedRead: (fail?: boolean) => Promise<void>;
}

async function delayMoldenRead(page: Page) {
  await page.evaluate(() => {
    const original = File.prototype.arrayBuffer;
    File.prototype.arrayBuffer = function () {
      if (this.name !== "delayed.molden") return original.call(this);
      const file = this;
      return new Promise<ArrayBuffer>((resolve, reject) => {
        (window as unknown as DelayedReadControl).releaseDelayedRead = async (
          fail = false,
        ) => {
          File.prototype.arrayBuffer = original;
          if (fail) reject(new Error("Delayed file read failed"));
          else resolve(await original.call(file));
          await new Promise((done) =>
            requestAnimationFrame(() => requestAnimationFrame(done)),
          );
        };
      });
    };
  });
}

async function pixels(page: Page) {
  return page.locator("canvas").evaluate((canvas) => {
    const gl = (canvas as HTMLCanvasElement).getContext("webgl2")!;
    const values = new Uint8Array(
      gl.drawingBufferWidth * gl.drawingBufferHeight * 4,
    );
    gl.readPixels(
      0,
      0,
      gl.drawingBufferWidth,
      gl.drawingBufferHeight,
      gl.RGBA,
      gl.UNSIGNED_BYTE,
      values,
    );
    let colored = 0;
    let checksum = 0;
    for (let i = 0; i < values.length; i += 4) {
      if (
        Math.max(values[i], values[i + 1], values[i + 2]) -
          Math.min(values[i], values[i + 1], values[i + 2]) >
        30
      )
        colored++;
      checksum =
        (checksum + values[i] * ((i % 97) + 1) + values[i + 1]) % 2147483647;
    }
    return { colored, checksum };
  });
}

async function decode(
  page: Page,
  filename: string,
): Promise<EigenVistaDocument> {
  return page.evaluate(
    async (bytes) => {
      const url = "/src/wasm/eigenvista_wasm.js";
      const core = await import(/* @vite-ignore */ url);
      await core.default();
      return JSON.parse(core.decode(new Uint8Array(bytes)));
    },
    Array.from(readFileSync(filename)),
  );
}

test("Molden Open generates a field and saves quantum data plus durable surfaces", async ({
  page,
}, testInfo) => {
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  await ready(page);
  await page.getByLabel("Open molecular file").setInputFiles(upload);
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened water.molden.input",
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
  await expect(page.locator(".document-summary")).toContainText("3 atoms");
  await expect(page.locator(".document-summary")).toContainText("2 bonds");
  await expect(page.locator(".document-summary")).toContainText(
    "24 basis functions",
  );
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await expect(page.locator(".import-report")).toContainText("import warning");
  await page.locator(".import-report summary").click();
  await expect(page.locator(".import-report ul")).not.toBeEmpty();
  await page.getByLabel("Dismiss import warnings").click();
  const density = page.locator(".field-group").filter({
    has: page.getByRole("heading", { name: "Density matrices", exact: true }),
  });
  await density.getByRole("button").first().click();
  await page.getByLabel("Grid resolution").selectOption("24");
  await page.getByRole("button", { name: "Generate surfaces" }).click();
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect(page.locator(".surface-row")).not.toHaveCount(0);
  await expect
    .poll(async () => (await pixels(page)).colored)
    .toBeGreaterThan(250);
  const initial = await pixels(page);
  const canvas = (await page.locator("canvas").boundingBox())!;
  await page.mouse.move(
    canvas.x + canvas.width * 0.4,
    canvas.y + canvas.height * 0.5,
  );
  await page.mouse.down();
  await page.mouse.move(
    canvas.x + canvas.width * 0.65,
    canvas.y + canvas.height * 0.65,
    { steps: 12 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(initial.checksum);
  await page.screenshot({
    path: `../artifacts/molden-${testInfo.project.name}-desktop.png`,
  });
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const saved = await download;
  const filename = path.resolve(
    `../artifacts/molden-${testInfo.project.name}-roundtrip.eigenvista`,
  );
  await saved.saveAs(filename);
  const doc = await decode(page, filename);
  expect(doc.atoms).toHaveLength(3);
  expect(doc.bonds).toHaveLength(2);
  expect(doc.basis).toHaveLength(24);
  expect(doc.orbitals.length).toBeGreaterThan(5);
  expect(doc.densities.length).toBeGreaterThan(0);
  expect(doc.surfaces.length).toBeGreaterThan(0);
  expect(doc.provenance.join(" ")).toMatch(/molden/i);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await page.getByLabel("Open molecular file").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await expect(
    page.getByLabel("Rendering mode").locator("option[value=raycast]"),
  ).toHaveAttribute("disabled", "");
  await page.setViewportSize({ width: 390, height: 844 });
  await page.getByRole("button", { name: "Fit scene", exact: true }).click();
  await expect
    .poll(async () => (await pixels(page)).colored)
    .toBeGreaterThan(100);
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= window.innerWidth,
    ),
  ).toBeTruthy();
  const scene = (await page.locator("canvas").boundingBox())!;
  const panel = (await page.locator(".left-panel").boundingBox())!;
  expect(scene.y + scene.height).toBeLessThanOrEqual(panel.y);
  await page.screenshot({
    path: `../artifacts/molden-${testInfo.project.name}-mobile.png`,
    fullPage: true,
  });
  expect(errors).toEqual([]);
});

test("content detection opens a renamed Molden and malformed input preserves it", async ({
  page,
}) => {
  await ready(page);
  await page
    .getByLabel("Open molecular file")
    .setInputFiles({ ...upload, name: "wavefunction.unknown" });
  await expect(page.locator(".document-summary")).toContainText(
    "24 basis functions",
  );
  const title = await page.locator(".document-title").innerText();
  await page.getByLabel("Open molecular file").setInputFiles({
    name: "broken.molden",
    mimeType: "text/plain",
    buffer: Buffer.from(
      "[Molden Format]\n[Atoms] AU\nO 1 8 0 0 0\n[GTO]\n1 0\ns 1 1\nnot a primitive\n",
    ),
  });
  await expect(page.getByRole("alert")).not.toBeEmpty();
  await expect(page.locator(".document-title")).toHaveText(title);
  await expect(page.locator(".document-summary")).toContainText(
    "24 basis functions",
  );
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
});

for (const outcome of ["success", "failure"]) {
  test(`a late file-read ${outcome} cannot replace or interrupt a newer Open`, async ({
    page,
  }) => {
    await ready(page);
    await delayMoldenRead(page);
    await page
      .getByLabel("Open molecular file")
      .setInputFiles({ ...upload, name: "delayed.molden" });
    await expect(page.locator("footer [role=status]")).toHaveText(
      "Opening document",
    );
    await expect(
      page.getByRole("button", { name: "Open", exact: true }),
    ).toBeDisabled();
    await page.getByLabel("Open molecular file").setInputFiles({
      name: "newer.xyz",
      mimeType: "text/plain",
      buffer: Buffer.from(
        "3\nNewer structure\nO 0 0 0\nH 0.9572 0 0\nH -0.239 0.927 0\n",
      ),
    });
    await expect(page.locator("footer [role=status]")).toHaveText(
      "Opened newer.xyz",
    );
    await page.evaluate(
      (fail) =>
        (window as unknown as DelayedReadControl).releaseDelayedRead(fail),
      outcome === "failure",
    );
    await expect(page.locator("footer [role=status]")).toHaveText(
      "Opened newer.xyz",
    );
    await expect(page.locator(".document-summary")).toContainText(
      "0 basis functions",
    );
    await expect(page.getByRole("alert")).toHaveCount(0);
    await expect(
      page.getByRole("button", { name: "Open", exact: true }),
    ).toBeEnabled();
  });
}

test("edits during a file read are protected by a fresh replacement confirmation", async ({
  page,
}) => {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  const title = await page.locator(".document-title").innerText();
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  await download;
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await delayMoldenRead(page);
  await page
    .getByLabel("Open molecular file")
    .setInputFiles({ ...upload, name: "delayed.molden" });
  await expect(page.locator("footer [role=status]")).toHaveText(
    "Opening document",
  );
  await page.getByLabel("Representation").selectOption("space-fill");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  let confirmations = 0;
  page.on("dialog", async (dialog) => {
    confirmations++;
    await dialog.dismiss();
  });
  await page.evaluate(() =>
    (window as unknown as DelayedReadControl).releaseDelayedRead(),
  );
  await expect(
    page.getByRole("button", { name: "Open", exact: true }),
  ).toBeEnabled();
  expect(confirmations).toBe(1);
  await expect(page.locator(".document-title")).toHaveText(title);
  await expect(page.getByLabel("Representation")).toHaveValue("space-fill");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  await expect(page.getByRole("alert")).toHaveCount(0);
});

test("batch conversion keeps the active document and saves only on request", async ({
  page,
}, testInfo) => {
  await ready(page);
  const title = await page.locator(".document-title").innerText();
  let downloads = 0;
  page.on("download", () => downloads++);
  await page
    .getByRole("button", { name: "Convert Files", exact: true })
    .click();
  const dialog = page.getByRole("dialog", { name: "Convert Files" });
  await page.getByLabel("Files to convert").setInputFiles([
    upload,
    {
      name: "water.xyz",
      mimeType: "text/plain",
      buffer: Buffer.from(
        "3\nWater\nO 0 0 0\nH 0.9572 0 0\nH -0.239 0.927 0\n",
      ),
    },
    {
      name: "broken.molden",
      mimeType: "text/plain",
      buffer: Buffer.from("[Molden Format]\n[Atoms] AU\n"),
    },
  ]);
  await dialog.getByRole("button", { name: "Convert", exact: true }).click();
  await expect(dialog.getByRole("status")).toHaveText("Processed 3 files");
  await expect(dialog.locator(".conversion-item.ready")).toHaveCount(2);
  await expect(dialog.locator(".conversion-item.failed")).toHaveCount(1);
  expect(downloads).toBe(0);
  const row = dialog
    .locator(".conversion-item")
    .filter({ has: page.getByText("water.molden.input", { exact: true }) });
  const download = page.waitForEvent("download");
  await row
    .getByRole("button", { name: "Save water.eigenvista", exact: true })
    .click();
  const saved = await download;
  expect(saved.suggestedFilename()).toBe("water.eigenvista");
  const filename = path.resolve(
    `../artifacts/molden-${testInfo.project.name}-converted.eigenvista`,
  );
  await saved.saveAs(filename);
  const doc = await decode(page, filename);
  expect(doc.basis).toHaveLength(24);
  expect(doc.surfaces).toHaveLength(0);
  await expect(row).toContainText("Download requested");
  await page.setViewportSize({ width: 390, height: 844 });
  const bounds = (await dialog.boundingBox())!;
  expect(bounds.x).toBeGreaterThanOrEqual(0);
  expect(bounds.x + bounds.width).toBeLessThanOrEqual(390);
  expect(
    await dialog.evaluate(
      (element) => element.scrollWidth <= element.clientWidth,
    ),
  ).toBeTruthy();
  await page.screenshot({
    path: `../artifacts/molden-${testInfo.project.name}-conversion-mobile.png`,
    fullPage: true,
  });
  await dialog.getByRole("button", { name: "Close conversion queue" }).click();
  await expect(page.locator(".document-title")).toHaveText(title);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  await page
    .getByRole("button", { name: "Convert Files", exact: true })
    .click();
  await expect(dialog.locator(".conversion-item")).toHaveCount(3);
});

test("conversion cancellation retains the scene and supports retry", async ({
  page,
}) => {
  await ready(page);
  const title = await page.locator(".document-title").innerText();
  // Hold only this page's import request so cancellation is deterministic.
  await page.evaluate(() => {
    const original = Worker.prototype.postMessage;
    (window as unknown as { restoreWorker: () => void }).restoreWorker = () => {
      Worker.prototype.postMessage = original;
    };
    Worker.prototype.postMessage = function (
      message,
      options?: StructuredSerializeOptions | Transferable[],
    ) {
      if (message.action === "import_document") return;
      original.call(
        this,
        message,
        Array.isArray(options) ? { transfer: options } : options,
      );
    };
  });
  await page
    .getByRole("button", { name: "Convert Files", exact: true })
    .click();
  const dialog = page.getByRole("dialog", { name: "Convert Files" });
  await page
    .getByLabel("Files to convert")
    .setInputFiles([upload, { ...upload, name: "second.molden" }]);
  await dialog.getByRole("button", { name: "Convert", exact: true }).click();
  await expect(dialog.locator(".conversion-item.converting")).toHaveCount(1);
  await expect(
    dialog.getByRole("button", { name: "Close conversion queue" }),
  ).toBeDisabled();
  await dialog.getByRole("button", { name: "Cancel conversion" }).click();
  await expect(dialog.locator(".conversion-item.cancelled")).toHaveCount(2);
  await page.evaluate(() =>
    (window as unknown as { restoreWorker: () => void }).restoreWorker(),
  );
  await dialog.getByRole("button", { name: "Convert", exact: true }).click();
  await expect(dialog.locator(".conversion-item.ready")).toHaveCount(2);
  await dialog.getByRole("button", { name: "Close conversion queue" }).click();
  await expect(page.locator(".document-title")).toHaveText(title);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  await page.getByRole("button", { name: "Generate surfaces" }).click();
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect(page.getByRole("alert")).toHaveCount(0);
});

for (const name of [
  "water-rhf-ccpvdz",
  "hydroxyl-uhf-sto3g",
  "general-spdfg-spherical",
  "general-spdfg-cartesian",
  "third-party/orca-nh3",
]) {
  const reference: MoldenReference = JSON.parse(
    readFileSync(path.resolve(`../fixtures/molden/${name}.json`), "utf8"),
  );
  const bytes = Array.from(
    readFileSync(path.resolve(`../fixtures/molden/${name}.molden`)),
  );
  test(`imported Molden WASM fields and gradients match independent reference: ${name}`, async ({
    page,
  }) => {
    await page.goto("/");
    const result = await page.evaluate(
      async ({ bytes, name, reference }) => {
        const url = "/src/wasm/eigenvista_wasm.js";
        const core = await import(/* @vite-ignore */ url);
        await core.default();
        const imported = JSON.parse(
          core.import_document(new Uint8Array(bytes), `${name}.molden`),
        );
        const document = imported.document as EigenVistaDocument;
        const restored = core.decode(core.encode(JSON.stringify(document)));
        const fixture = reference.reference;
        let maxError = 0;
        let maxScaledError = 0;
        let comparisons = 0;
        function compare(actual: number, expected: number) {
          const error = Math.abs(actual - expected);
          maxError = Math.max(maxError, error);
          // Match the independent native import tests' finite text-precision budget.
          maxScaledError = Math.max(
            maxScaledError,
            error / (2e-9 + 2e-8 * Math.abs(expected)),
          );
          comparisons++;
        }
        for (let orbital = 0; orbital < fixture.orbitals.length; orbital++) {
          for (let point = 0; point < fixture.points.length; point++) {
            const actual = core.point_with_gradient(
              restored,
              document.orbitals[orbital].id,
              ...fixture.points[point],
            );
            fixture.orbitals[orbital].samples[point].forEach(
              (expected, component) => compare(actual[component], expected),
            );
          }
        }
        for (const density of fixture.densities) {
          const actualDensity = document.densities.find(
            (item) => item.kind === density.kind,
          )!;
          for (let point = 0; point < fixture.points.length; point++) {
            const actual = core.point_with_gradient(
              restored,
              actualDensity.id,
              ...fixture.points[point],
            );
            density.samples[point].forEach((expected, component) =>
              compare(actual[component], expected),
            );
          }
          const n = document.basis.length;
          let electrons = 0;
          for (let i = 0; i < n; i++)
            for (let j = 0; j < n; j++)
              electrons +=
                actualDensity.matrix[i * n + j] * fixture.overlap[j * n + i];
          compare(electrons, density.electrons);
        }
        // Unit-coefficient test orbitals expose every imported AO through the public bridge.
        for (let ao = 0; ao < document.basis.length; ao++) {
          const probe: EigenVistaDocument = {
            ...document,
            orbitals: [
              {
                id: "ao-probe",
                label: "AO probe",
                spin: "spatial",
                occupation: null,
                energy: null,
                coefficients: document.basis.map((_, index) =>
                  index === ao ? 1 : 0,
                ),
              },
            ],
            densities: [],
            surfaces: [],
            view: { ...document.view, field: "ao-probe" },
          };
          const json = JSON.stringify(probe);
          for (let point = 0; point < fixture.points.length; point++) {
            const actual = core.point_with_gradient(
              json,
              "ao-probe",
              ...fixture.points[point],
            );
            fixture.ao[point][ao].forEach((expected, component) =>
              compare(actual[component], expected),
            );
          }
        }
        return {
          maxError,
          maxScaledError,
          comparisons,
          format: imported.report.format,
          orbitalSpins: document.orbitals.map((orbital) => orbital.spin),
          densityKinds: document.densities
            .map((density) => density.kind)
            .sort(),
          basisCount: document.basis.length,
          roundtripEqual:
            JSON.stringify(JSON.parse(restored)) === JSON.stringify(document),
        };
      },
      { bytes, name, reference },
    );
    expect(result.format).toBe("molden");
    expect(result.orbitalSpins).toEqual(
      reference.reference.orbitals.map((orbital) => orbital.spin),
    );
    expect(result.densityKinds).toEqual(
      reference.reference.densities.map((density) => density.kind).sort(),
    );
    expect(result.basisCount).toBe(reference.reference.ao[0].length);
    expect(result.roundtripEqual).toBeTruthy();
    expect(result.comparisons).toBeGreaterThan(500);
    expect(result.maxScaledError).toBeLessThanOrEqual(1);
    test.info().annotations.push({
      type: "molden-reference-error",
      description: `${result.comparisons} independent value/gradient/electron comparisons; max absolute error ${result.maxError}`,
    });
  });
}
