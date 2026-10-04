import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { Grid, EigenVistaDocument } from "../src/types";

type DensityWorkerProbe = {
  request: number | null;
  sampling: boolean;
  completed: boolean;
  terminated: boolean;
};
type ProbedWindow = Window & { densityWorkerProbe: DensityWorkerProbe };

async function save(page: Page, filename: string): Promise<EigenVistaDocument> {
  const download = page.waitForEvent("download");
  await page.getByTitle("Save document", { exact: true }).click();
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

test("a real high-resolution density worker leaves the scene responsive and can be cancelled without changing saved meshes", async ({
  page,
}, testInfo) => {
  test.setTimeout(120_000);
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("dialog", (dialog) => dialog.accept());
  await page.addInitScript(() => {
    const control = window as unknown as ProbedWindow;
    control.densityWorkerProbe = {
      request: null,
      sampling: false,
      completed: false,
      terminated: false,
    };
    let generatingWorker: Worker | null = null;
    const post = Worker.prototype.postMessage;
    const terminate = Worker.prototype.terminate;
    Worker.prototype.postMessage = function (
      message,
      options?: StructuredSerializeOptions | Transferable[],
    ) {
      if (message.action === "generate" && message.args.resolution === 48) {
        generatingWorker = this;
        control.densityWorkerProbe.request = message.id;
        this.addEventListener("message", ({ data }) => {
          if (data.id !== message.id) return;
          if (data.progress === "sampling")
            control.densityWorkerProbe.sampling = true;
          if ("result" in data || "error" in data)
            control.densityWorkerProbe.completed = true;
        });
      }
      post.call(
        this,
        message,
        Array.isArray(options) ? { transfer: options } : options,
      );
    };
    Worker.prototype.terminate = function () {
      if (this === generatingWorker)
        control.densityWorkerProbe.terminated = true;
      terminate.call(this);
    };
  });
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await page
    .getByLabel("Open molecular file")
    .setInputFiles(path.resolve("../../data/molden.input"));
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened molden.input",
  );
  const fields = page.locator(".field-group").filter({
    has: page.getByRole("heading", { name: "Density matrices", exact: true }),
  });
  await fields.getByRole("button", { name: /Occupation-derived/ }).click();
  await page.getByLabel("Grid resolution", { exact: true }).selectOption("24");
  await page.getByLabel("Isovalue", { exact: true }).fill("0.08");
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await expect(page.locator("footer [role=status]")).toContainText(
    "triangles",
    {
      timeout: 90_000,
    },
  );
  await expect(page.locator(".surface-row")).toHaveCount(1);
  const prefix = path.resolve(
    `../artifacts/density-cancel-${testInfo.project.name}`,
  );
  const before = await save(page, `${prefix}-before.eigenvista`);
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(250);
  const beforeRotation = await pixels(page);
  await page.getByLabel("Grid resolution", { exact: true }).selectOption("48");
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await page.waitForFunction(
    () => (window as unknown as ProbedWindow).densityWorkerProbe.sampling,
  );
  await expect(page.locator("footer [role=status]")).toContainText(
    "Sampling field",
  );
  // Observe the actual worker, without delaying or substituting its calculation.
  const response = await page.evaluate(async () => {
    let frames = 0;
    let timer = false;
    const frame = () => {
      frames++;
      if (!timer) requestAnimationFrame(frame);
    };
    requestAnimationFrame(frame);
    await new Promise<void>((resolve) =>
      setTimeout(() => {
        timer = true;
        resolve();
      }, 120),
    );
    return {
      frames,
      timer,
      completed: (window as unknown as ProbedWindow).densityWorkerProbe
        .completed,
    };
  });
  expect(response.timer).toBeTruthy();
  expect(response.frames).toBeGreaterThan(1);
  expect(response.completed).toBeFalsy();
  await expect(
    page.getByRole("button", { name: "Cancel", exact: true }),
  ).toBeEnabled();
  const canvas = (await page.locator("canvas").boundingBox())!;
  await page.mouse.move(
    canvas.x + canvas.width * 0.4,
    canvas.y + canvas.height * 0.5,
  );
  await page.mouse.down();
  await page.mouse.move(
    canvas.x + canvas.width * 0.65,
    canvas.y + canvas.height * 0.65,
    { steps: 3 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(beforeRotation.checksum);
  expect(
    await page.evaluate(
      () => (window as unknown as ProbedWindow).densityWorkerProbe.completed,
    ),
  ).toBeFalsy();
  const cancelled = Date.now();
  await page.getByRole("button", { name: "Cancel", exact: true }).click();
  await expect(page.locator("footer [role=status]")).toContainText(
    "Cancelled; previous surfaces retained",
  );
  expect(Date.now() - cancelled).toBeLessThan(2000);
  expect(
    await page.evaluate(
      () => (window as unknown as ProbedWindow).densityWorkerProbe,
    ),
  ).toMatchObject({
    sampling: true,
    completed: false,
    terminated: true,
  });
  await expect(page.getByRole("alert")).toHaveCount(0);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await expect(page.locator(".surface-row")).toHaveCount(1);
  await page.screenshot({ path: `${prefix}.png` });
  expect(await save(page, `${prefix}-after.eigenvista`)).toEqual(before);
  await page.getByLabel("Grid resolution", { exact: true }).selectOption("24");
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await expect(page.locator("footer [role=status]")).toContainText(
    "triangles",
    { timeout: 90_000 },
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
  expect(errors).toEqual([]);
});

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
      checksum = 0;
    for (let i = 0; i < values.length; i += 4) {
      const r = values[i],
        g = values[i + 1],
        b = values[i + 2];
      if (g > r + 20 && g > b + 8) positive++;
      checksum = (checksum + r * ((i % 97) + 1) + g) % 2147483647;
    }
    return { positive, checksum };
  });
}

async function rendered(page: Page) {
  await page.evaluate(
    () =>
      new Promise<void>((resolve) =>
        requestAnimationFrame(() => requestAnimationFrame(() => resolve())),
      ),
  );
}

for (const resolution of [24, 48]) {
  test(`the repository Molden density renders at ${resolution} cubed and preserves science and surfaces on reopen`, async ({
    page,
  }, testInfo) => {
    test.setTimeout(120_000);
    const errors: string[] = [];
    page.on("pageerror", (error) => errors.push(error.message));
    page.on("dialog", (dialog) => dialog.accept());
    await page.goto("/");
    await expect(page.locator("footer [role=status]")).toContainText(
      "triangles",
    );
    await page
      .getByLabel("Open molecular file")
      .setInputFiles(path.resolve("../../data/molden.input"));
    await expect(page.locator("footer [role=status]")).toContainText(
      "Opened molden.input",
    );
    await expect(page.locator(".document-summary")).toContainText("17 atoms");
    await expect(page.locator(".document-summary")).toContainText(
      "125 basis functions",
    );
    const prefix = path.resolve(
      `../artifacts/density-budget-${resolution}-${testInfo.project.name}`,
    );
    const source = await save(page, `${prefix}-before.eigenvista`);
    expect(source.basis).toHaveLength(125);
    expect(source.orbitals).toHaveLength(118);
    expect(source.densities).toHaveLength(1);
    expect(source.surfaces).toEqual([]);
    const density = source.densities[0];
    expect(density.kind).toBe("total");
    expect(density.label).toContain("Occupation-derived");
    const numerical = await page.evaluate(
      async ({ document, field, resolution }) => {
        const url = "/src/wasm/eigenvista_wasm.js";
        const core = await import(/* @vite-ignore */ url);
        await core.default();
        const json = JSON.stringify(document);
        const grid: Grid = JSON.parse(core.sample(json, field, resolution));
        const center = Math.floor(resolution / 2);
        const probes = [
          [0, 0, 0],
          [resolution - 1, resolution - 1, resolution - 1],
          [0, center, center],
          [center, center, center],
          [center - 3, center - 2, center],
          [center + 2, center + 1, center - 2],
        ].map(([i, j, k]) => {
          const point = grid.origin.map(
            (value, axis) =>
              value +
              i * grid.axes[0][axis] +
              j * grid.axes[1][axis] +
              k * grid.axes[2][axis],
          );
          return {
            sampled: grid.values[(k * resolution + j) * resolution + i],
            reference: core.point(json, field, point[0], point[1], point[2]),
          };
        });
        return { dims: grid.dims, count: grid.values.length, probes };
      },
      { document: source, field: density.id, resolution },
    );
    expect(numerical.dims).toEqual([resolution, resolution, resolution]);
    expect(numerical.count).toBe(resolution ** 3);
    for (const probe of numerical.probes) {
      expect(Number.isFinite(probe.sampled)).toBeTruthy();
      expect(Number.isFinite(probe.reference)).toBeTruthy();
      expect(Math.abs(probe.sampled - probe.reference)).toBeLessThanOrEqual(
        1e-12 + 1e-10 * Math.abs(probe.reference),
      );
    }
    const fields = page.locator(".field-group").filter({
      has: page.getByRole("heading", { name: "Density matrices", exact: true }),
    });
    await fields.getByRole("button", { name: /Occupation-derived/ }).click();
    await page
      .getByLabel("Grid resolution", { exact: true })
      .selectOption(String(resolution));
    await page.getByLabel("Isovalue", { exact: true }).fill("0.08");
    await page
      .getByRole("button", { name: "Generate surfaces", exact: true })
      .click();
    await expect(page.locator("footer [role=status]")).toContainText(
      "triangles",
      { timeout: 90_000 },
    );
    await expect(page.getByRole("alert")).toHaveCount(0);
    await expect(page.locator(".surface-row")).toHaveCount(1);
    for (const mode of ["mesh", "raycast", "volume"]) {
      await expect(
        page.getByLabel("Rendering mode").locator(`option[value=${mode}]`),
      ).toBeEnabled();
      await page.getByLabel("Rendering mode").selectOption(mode);
      await rendered(page);
      // All source atoms are C/H: green pixels must come from the density.
      await expect
        .poll(async () => (await pixels(page)).positive)
        .toBeGreaterThan(250);
      await page.screenshot({ path: `${prefix}-${mode}.png` });
    }
    await page.getByLabel("Rendering mode").selectOption("mesh");
    await rendered(page);
    await expect
      .poll(async () => (await pixels(page)).positive)
      .toBeGreaterThan(250);
    const beforeRotation = await pixels(page);
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
      .not.toBe(beforeRotation.checksum);
    const filename = `${prefix}-generated.eigenvista`;
    const generated = await save(page, filename);
    expect(generated).toEqual({
      ...source,
      view: { ...source.view, field: density.id, isovalue: 0.08 },
      surfaces: generated.surfaces,
    });
    expect(generated.surfaces).toHaveLength(1);
    expect(generated.surfaces[0]).toMatchObject({
      field: density.id,
      isovalue: 0.08,
      resolution: [resolution, resolution, resolution],
      visible: true,
    });
    expect(generated.surfaces[0].indices.length).toBeGreaterThan(0);
    expect(generated.surfaces[0].source_hash).toMatch(/^[0-9a-f]{64}$/);
    await page.getByLabel("Open molecular file").setInputFiles(filename);
    await expect(page.locator("footer [role=status]")).toContainText(
      "saved geometry restored",
    );
    await expect(page.getByRole("alert")).toHaveCount(0);
    await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
    await expect(page.locator(".surface-row")).toHaveCount(1);
    await expect
      .poll(async () => (await pixels(page)).positive)
      .toBeGreaterThan(250);
    expect(await save(page, `${prefix}-reopened.eigenvista`)).toEqual(
      generated,
    );
    await page.setViewportSize({ width: 390, height: 844 });
    await page.getByRole("button", { name: "Fit scene", exact: true }).click();
    await rendered(page);
    await expect
      .poll(async () => (await pixels(page)).positive)
      .toBeGreaterThan(100);
    expect(
      await page.evaluate(
        () => document.documentElement.scrollWidth <= window.innerWidth,
      ),
    ).toBeTruthy();
    await page.screenshot({ path: `${prefix}-mobile.png`, fullPage: true });
    expect(errors).toEqual([]);
  });
}
