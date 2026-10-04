import { test, expect, type Locator, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { EigenVistaDocument } from "../src/types";

const water = {
  name: "water.molden",
  mimeType: "text/plain",
  buffer: readFileSync(
    path.resolve("../fixtures/molden/water-rhf-ccpvdz.molden"),
  ),
};

async function ready(page: Page) {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  page.on("dialog", (dialog) => dialog.accept());
}

async function browse(page: Page) {
  await page
    .getByRole("button", { name: "Browse orbitals", exact: true })
    .click();
  const dialog = page.getByRole("dialog", { name: "Orbitals", exact: true });
  await expect(dialog).toBeVisible();
  return dialog;
}

function row(dialog: Locator, number: number) {
  return dialog.getByRole("row").filter({
    has: dialog.page().getByRole("checkbox", {
      name: `Select orbital ${number}`,
      exact: true,
    }),
  });
}

async function select(dialog: Locator, numbers: number[], iso = "0.05") {
  const clear = dialog.getByRole("button", { name: "Clear orbital selection" });
  if (await clear.isEnabled()) await clear.click();
  for (const number of numbers)
    await dialog
      .getByRole("checkbox", { name: `Select orbital ${number}`, exact: true })
      .check();
  await dialog.getByLabel("Orbital isovalue", { exact: true }).fill(iso);
  await dialog
    .getByLabel("Orbital grid resolution", { exact: true })
    .selectOption("24");
  await expect(dialog.locator(".orbital-selection-count")).toHaveText(
    `${numbers.length} selected`,
  );
}

async function save(page: Page, name: string): Promise<EigenVistaDocument> {
  const download = page.waitForEvent("download");
  await page.getByTitle("Save document", { exact: true }).click();
  const filename = path.resolve(`../artifacts/${name}.eigenvista`);
  await (await download).saveAs(filename);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
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
    let positive = 0,
      negative = 0,
      checksum = 0;
    for (let i = 0; i < values.length; i += 4) {
      const r = values[i],
        g = values[i + 1],
        b = values[i + 2];
      if (g > r + 20 && g > b + 8) positive++;
      if (r > g + 20 && b > g + 8) negative++;
      checksum = (checksum + r * ((i % 97) + 1) + g) % 2147483647;
    }
    return { positive, negative, checksum };
  });
}

test("the orbital browser lists all 118 imported orbitals with metadata, filtering and bounded selection", async ({
  page,
}, testInfo) => {
  await ready(page);
  await page
    .getByLabel("Open molecular file")
    .setInputFiles(path.resolve("../../data/molden.input"));
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened molden.input",
  );
  const before = await save(
    page,
    `orbitals-${testInfo.project.name}-large-before`,
  );
  const selection = (await page.locator(".selection-status").textContent())!;
  const dialog = await browse(page);
  await expect(
    dialog.getByRole("checkbox", { name: /^Select orbital [0-9]+$/ }),
  ).toHaveCount(118);
  await expect(
    dialog.getByRole("columnheader", { name: "Energy (hartree)", exact: true }),
  ).toBeVisible();
  await expect(row(dialog, 1).getByRole("cell").nth(2)).toContainText("1a");
  await expect(row(dialog, 1).getByRole("cell").nth(3)).toHaveText("spatial");
  await expect(row(dialog, 1).getByRole("cell").nth(4)).toHaveText("2.000");
  await expect(row(dialog, 1).getByRole("cell").nth(5)).toHaveText("-9.917540");
  await expect(row(dialog, 118).getByRole("cell").nth(4)).toHaveText("0.000");
  await expect(row(dialog, 118).getByRole("cell").nth(5)).toHaveText(
    "2.764712",
  );
  await dialog
    .getByRole("checkbox", {
      name: "Select listed orbitals",
      exact: true,
    })
    .check();
  await expect(dialog.locator(".orbital-selection-count")).toHaveText(
    "118 selected",
  );
  await expect(dialog.getByRole("alert")).toHaveText(
    "Select at most 32 orbitals per generation.",
  );
  await expect(
    dialog.getByRole("button", { name: "Generate selected", exact: true }),
  ).toBeDisabled();
  await dialog.getByRole("button", { name: "Clear orbital selection" }).click();
  await dialog.getByLabel("Filter orbitals").fill("118");
  await expect(
    dialog.getByRole("checkbox", { name: /^Select orbital [0-9]+$/ }),
  ).toHaveCount(1);
  await dialog
    .getByRole("checkbox", { name: "Select orbital 118", exact: true })
    .check();
  await dialog.getByLabel("Filter orbitals").fill("no-such-orbital");
  await expect(
    dialog.getByText("No matching orbitals", { exact: true }),
  ).toBeVisible();
  await expect(
    dialog.getByRole("checkbox", {
      name: "Select listed orbitals",
      exact: true,
    }),
  ).toBeDisabled();
  await dialog.getByLabel("Filter orbitals").fill("");
  await expect(
    dialog.getByRole("checkbox", { name: "Select orbital 118", exact: true }),
  ).toBeChecked();
  await dialog.getByRole("button", { name: "Clear orbital selection" }).click();
  for (let number = 1; number <= 32; number++)
    await dialog
      .getByRole("checkbox", { name: `Select orbital ${number}`, exact: true })
      .check();
  await expect(dialog.locator(".orbital-selection-count")).toHaveText(
    "32 selected",
  );
  await dialog
    .getByRole("checkbox", { name: "Select orbital 33", exact: true })
    .check();
  await expect(
    dialog.getByRole("button", { name: "Generate selected", exact: true }),
  ).toBeDisabled();
  await dialog
    .getByRole("checkbox", { name: "Select orbital 33", exact: true })
    .uncheck();
  await expect(
    dialog.getByRole("button", { name: "Generate selected", exact: true }),
  ).toBeEnabled();
  await page.screenshot({
    path: `../artifacts/orbitals-${testInfo.project.name}-desktop.png`,
  });
  await page.setViewportSize({ width: 390, height: 844 });
  await dialog.getByLabel("Filter orbitals").fill("118");
  const bounds = (await dialog.boundingBox())!;
  expect(bounds.x).toBeGreaterThanOrEqual(0);
  expect(bounds.y).toBeGreaterThanOrEqual(0);
  expect(bounds.x + bounds.width).toBeLessThanOrEqual(390);
  expect(bounds.y + bounds.height).toBeLessThanOrEqual(844);
  expect(
    await dialog.evaluate(
      (element) => element.scrollWidth <= element.clientWidth,
    ),
  ).toBeTruthy();
  for (const cell of await row(dialog, 118).getByRole("cell").all())
    expect(
      await cell.evaluate(
        (element) => element.scrollWidth <= element.clientWidth,
      ),
    ).toBeTruthy();
  await expect(
    dialog.getByRole("button", { name: "Generate selected", exact: true }),
  ).toBeInViewport();
  await page.screenshot({
    path: `../artifacts/orbitals-${testInfo.project.name}-mobile.png`,
    fullPage: true,
  });
  await dialog.locator(".orbital-table-scroll").evaluate((element) => {
    element.scrollLeft = element.scrollWidth;
  });
  await expect(row(dialog, 118).getByRole("cell").nth(5)).toBeInViewport();
  await page.screenshot({
    path: `../artifacts/orbitals-${testInfo.project.name}-mobile-metadata.png`,
    fullPage: true,
  });
  await dialog
    .getByRole("button", { name: "Close orbitals", exact: true })
    .focus();
  await page.keyboard.press("Escape");
  await expect(dialog).toHaveCount(0);
  await expect(page.locator(".selection-status")).toHaveText(selection);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  const after = await save(
    page,
    `orbitals-${testInfo.project.name}-large-after`,
  );
  expect(after).toEqual(before);
});

test("multiple Molden orbitals generate signed persistent meshes while preserving unselected surfaces", async ({
  page,
}, testInfo) => {
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  await ready(page);
  await page.getByLabel("Open molecular file").setInputFiles(water);
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened water.molden",
  );
  const densities = page.locator(".field-group").filter({
    has: page.getByRole("heading", { name: "Density matrices", exact: true }),
  });
  await densities.getByRole("button").first().click();
  await page.getByLabel("Grid resolution", { exact: true }).selectOption("24");
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  const prior = await save(
    page,
    `orbitals-${testInfo.project.name}-density-before`,
  );
  expect(prior.surfaces.length).toBeGreaterThan(0);
  const dialog = await browse(page);
  await expect(dialog.locator(".orbital-selection-count")).toHaveText(
    "0 selected",
  );
  await select(dialog, [4, 5]);
  await dialog
    .getByRole("button", { name: "Generate selected", exact: true })
    .click();
  await expect(dialog).toHaveCount(0);
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  const saved = await save(page, `orbitals-${testInfo.project.name}-batch`);
  expect(saved.orbitals).toEqual(prior.orbitals);
  expect(saved.basis).toEqual(prior.basis);
  expect(saved.densities).toEqual(prior.densities);
  expect(saved.view.field).toBe(saved.orbitals[3].id);
  expect(saved.view.isovalue).toBe(0.05);
  expect(
    saved.surfaces.filter((surface) => surface.field === prior.view.field),
  ).toEqual(prior.surfaces);
  for (const orbital of [saved.orbitals[3], saved.orbitals[4]]) {
    const surfaces = saved.surfaces.filter(
      (surface) => surface.field === orbital.id,
    );
    expect(surfaces.map((surface) => surface.isovalue).sort()).toEqual([
      -0.05, 0.05,
    ]);
    for (const surface of surfaces) {
      expect(surface.positions.length).toBeGreaterThan(0);
      expect(surface.indices.length).toBeGreaterThan(0);
      expect(surface.source_hash).toMatch(/^[0-9a-f]{64}$/);
    }
  }
  const reopened = await browse(page);
  await expect(
    reopened.getByRole("checkbox", { name: "Select orbital 4", exact: true }),
  ).toBeChecked();
  await expect(row(reopened, 4).getByRole("cell").nth(6)).toHaveText("2");
  await expect(row(reopened, 5).getByRole("cell").nth(6)).toHaveText("2");
  await reopened
    .getByRole("button", { name: "Close orbitals", exact: true })
    .click();
  await page.getByLabel("Rendering mode").selectOption("volume");
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(100);
  await expect
    .poll(async () => (await pixels(page)).negative)
    .toBeGreaterThan(100);
  const volumeBefore = await pixels(page);
  // The retained preview must be the chosen active orbital, not the last batch result.
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .toBe(volumeBefore.checksum);
  await page.getByLabel("Rendering mode").selectOption("mesh");
  await page.screenshot({
    path: `../artifacts/orbitals-${testInfo.project.name}-batch-meshes.png`,
  });
  await page
    .getByLabel("Open molecular file")
    .setInputFiles(
      path.resolve(
        `../artifacts/orbitals-${testInfo.project.name}-batch.eigenvista`,
      ),
    );
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.locator(".surface-row")).toHaveCount(saved.surfaces.length);
  const roundtrip = await save(
    page,
    `orbitals-${testInfo.project.name}-batch-reopened`,
  );
  expect(roundtrip).toEqual(saved);
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(100);
  await expect
    .poll(async () => (await pixels(page)).negative)
    .toBeGreaterThan(100);
  const initialPixels = await pixels(page);
  const canvas = (await page.locator("canvas").boundingBox())!;
  await page.mouse.move(
    canvas.x + canvas.width * 0.4,
    canvas.y + canvas.height * 0.5,
  );
  await page.mouse.down();
  await page.mouse.move(
    canvas.x + canvas.width * 0.65,
    canvas.y + canvas.height * 0.6,
    { steps: 12 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(initialPixels.checksum);
  expect(errors).toEqual([]);
});

interface WorkerControl {
  orbitalRequests: number;
  restoreOrbitalWorker: () => void;
}

for (const outcome of [
  "cancel",
  "cancel-without-active",
  "worker-error",
  "invalid-mesh",
  "native-budget",
] as const) {
  test(`a second-orbital ${outcome} retains the entire previous document atomically`, async ({
    page,
  }, testInfo) => {
    await ready(page);
    let before = await save(
      page,
      `orbitals-${testInfo.project.name}-${outcome}-before`,
    );
    if (outcome === "cancel-without-active") {
      const bytes = await page.evaluate(async (doc) => {
        const url = "/src/wasm/eigenvista_wasm.js";
        const core = await import(/* @vite-ignore */ url);
        await core.default();
        return Array.from(
          core.encode(
            JSON.stringify({ ...doc, view: { ...doc.view, field: null } }),
          ) as Uint8Array,
        );
      }, before);
      await page.getByLabel("Open molecular file").setInputFiles({
        name: "no-active-orbital.eigenvista",
        mimeType: "application/octet-stream",
        buffer: Buffer.from(bytes),
      });
      await expect(page.locator("footer [role=status]")).toContainText(
        "Opened no-active-orbital.eigenvista",
      );
      before = await save(
        page,
        `orbitals-${testInfo.project.name}-${outcome}-before`,
      );
      expect(before.view.field).toBeNull();
    }
    const resolution = await page
      .getByLabel("Grid resolution", { exact: true })
      .inputValue();
    await page.evaluate((outcome) => {
      const control = window as unknown as WorkerControl;
      control.orbitalRequests = 0;
      const original = Worker.prototype.postMessage;
      control.restoreOrbitalWorker = () => {
        Worker.prototype.postMessage = original;
      };
      Worker.prototype.postMessage = function (
        message,
        options?: StructuredSerializeOptions | Transferable[],
      ) {
        if (message.action === "generate" && ++control.orbitalRequests === 2) {
          if (outcome.startsWith("cancel")) return;
          if (outcome === "worker-error") {
            queueMicrotask(() =>
              this.onmessage?.call(
                this,
                new MessageEvent("message", {
                  data: {
                    id: message.id,
                    error: "Second orbital failed deliberately",
                  },
                }),
              ),
            );
            return;
          }
          const receive = this.onmessage;
          this.onmessage = (event) => {
            if (
              event.data.id === message.id &&
              event.data.result?.surfaces?.length
            ) {
              if (outcome === "invalid-mesh")
                event.data.result.surfaces[0].source_hash = "0".repeat(64);
              else
                event.data.result.surfaces[0].label = "x".repeat(
                  8 * 1024 * 1024,
                );
              this.onmessage = receive;
            }
            receive?.call(this, event);
          };
        }
        original.call(
          this,
          message,
          Array.isArray(options) ? { transfer: options } : options,
        );
      };
    }, outcome);
    const dialog = await browse(page);
    if (outcome === "cancel-without-active")
      await expect(dialog.locator(".orbital-selection-count")).toHaveText(
        "0 selected",
      );
    else
      await expect(
        dialog.getByRole("checkbox", { name: "Select orbital 2", exact: true }),
      ).toBeChecked();
    await select(dialog, [1, 2], "0.04");
    await dialog
      .getByRole("button", { name: "Generate selected", exact: true })
      .click();
    await expect
      .poll(() =>
        page.evaluate(
          () => (window as unknown as WorkerControl).orbitalRequests,
        ),
      )
      .toBe(2);
    if (outcome.startsWith("cancel")) {
      await expect(page.locator(".surface-row")).toHaveCount(
        before.surfaces.length,
      );
      await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
      await page.getByRole("button", { name: "Cancel", exact: true }).click();
      await expect(page.locator("footer [role=status]")).toContainText(
        "previous surfaces retained",
      );
    } else {
      await expect(page.getByRole("alert")).not.toBeEmpty();
      if (outcome === "worker-error")
        await expect(page.getByRole("alert")).toContainText(
          "Second orbital failed deliberately",
        );
      else if (outcome === "invalid-mesh")
        await expect(page.getByRole("alert")).toContainText("source");
      else
        await expect(page.getByRole("alert")).toContainText(
          "Native document exceeds preview budget",
        );
    }
    await page.evaluate(() =>
      (window as unknown as WorkerControl).restoreOrbitalWorker(),
    );
    await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
    await expect(
      page.getByLabel("Grid resolution", { exact: true }),
    ).toHaveValue(resolution);
    const after = await save(
      page,
      `orbitals-${testInfo.project.name}-${outcome}-after`,
    );
    expect(after).toEqual(before);
    if (outcome === "cancel") {
      const retry = await browse(page);
      await select(retry, [1, 2], "0.04");
      await retry
        .getByRole("button", { name: "Generate selected", exact: true })
        .click();
      await expect(page.locator("footer [role=status]")).toContainText(
        "triangles",
      );
      const result = await save(
        page,
        `orbitals-${testInfo.project.name}-retry`,
      );
      expect(result.view.field).toBe("antibonding");
      expect(new Set(result.surfaces.map((surface) => surface.field))).toEqual(
        new Set(["bonding", "antibonding"]),
      );
      expect(
        result.surfaces.some(
          (surface) =>
            surface.field === "antibonding" && surface.isovalue === -0.04,
        ),
      ).toBeTruthy();
    }
  });
}

test("missing orbital metadata remains unknown and invalid batch settings cannot submit", async ({
  page,
}) => {
  await ready(page);
  const bytes = await page.evaluate(async () => {
    const url = "/src/wasm/eigenvista_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    const doc = JSON.parse(core.example(false)) as EigenVistaDocument;
    doc.orbitals[0].occupation = null;
    doc.orbitals[0].energy = null;
    return Array.from(core.encode(JSON.stringify(doc)) as Uint8Array);
  });
  await page.getByLabel("Open molecular file").setInputFiles({
    name: "unknown.eigenvista",
    mimeType: "application/octet-stream",
    buffer: Buffer.from(bytes),
  });
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened unknown.eigenvista",
  );
  const dialog = await browse(page);
  await expect(row(dialog, 1).getByRole("cell").nth(3)).toHaveText("spatial");
  await expect(row(dialog, 1).getByRole("cell").nth(4)).toHaveText("--");
  await expect(row(dialog, 1).getByRole("cell").nth(5)).toHaveText("--");
  await expect(row(dialog, 2).getByRole("cell").nth(4)).toHaveText("0.000");
  await expect(row(dialog, 2).getByRole("cell").nth(5)).toHaveText("--");
  await dialog.getByRole("button", { name: "Clear orbital selection" }).click();
  const generate = dialog.getByRole("button", {
    name: "Generate selected",
    exact: true,
  });
  await expect(generate).toBeDisabled();
  await dialog
    .getByRole("checkbox", { name: "Select orbital 1", exact: true })
    .check();
  for (const invalid of ["", "0", "-1"]) {
    await dialog.getByLabel("Orbital isovalue", { exact: true }).fill(invalid);
    await expect(generate).toBeDisabled();
  }
  await dialog.getByLabel("Orbital isovalue", { exact: true }).fill("0.08");
  await expect(generate).toBeEnabled();
  await dialog
    .getByRole("button", { name: "Close orbitals", exact: true })
    .click();
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
});
