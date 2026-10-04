import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { MolekelDocument } from "../src/types";

type WorkerProbe = {
  sampling: boolean;
  completed: boolean;
  terminated: boolean;
};
type ProbedWindow = Window & {
  highResolutionProbe: WorkerProbe;
  failHighResolutionWorker: (fatal: boolean) => void;
};
type DisplayGridProbe = {
  dims: number[];
  count: number;
  typed: boolean;
  samples: { position: number[]; value: number }[];
};
type CacheProbedWindow = Window & {
  highResolutionCacheProbe: {
    sampling: number;
    meshing: number;
    grids: DisplayGridProbe[];
  };
};

async function ready(page: Page) {
  page.on("dialog", (dialog) => dialog.accept());
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText(
    "triangles",
    { timeout: 45_000 },
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
}

async function save(page: Page, filename: string): Promise<MolekelDocument> {
  const download = page.waitForEvent("download");
  await page.getByTitle("Save document", { exact: true }).click();
  await (await download).saveAs(filename);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  return page.evaluate(
    async (bytes) => {
      const url = "/src/wasm/molekel_wasm.js";
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
      checksum = 0;
    for (let i = 0; i < values.length; i += 4) {
      if (values[i + 1] > values[i] + 20 && values[i + 1] > values[i + 2] + 8)
        positive++;
      checksum =
        (checksum + values[i] * ((i % 97) + 1) + values[i + 1]) % 2147483647;
    }
    return { positive, checksum };
  });
}

async function rotate(page: Page) {
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
    { steps: 3 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(initial.checksum);
}

async function generate(page: Page, resolution: number) {
  await page
    .getByLabel("Grid resolution", { exact: true })
    .selectOption(String(resolution));
  await page
    .getByRole("button", { name: "Generate surfaces", exact: true })
    .click();
  await expect(page.locator("footer [role=status]")).toContainText(
    "triangles",
    { timeout: 150_000 },
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
}

test("a scene allocation failure preserves the rendered scene and a later rebuild recovers", async ({
  page,
}) => {
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  await ready(page);
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(250);
  const before = await pixels(page);
  await page.evaluate(() => {
    const original = window.Float32Array;
    window.Float32Array = new Proxy(original, {
      construct() {
        window.Float32Array = original;
        throw new RangeError("Deliberate scene allocation failure");
      },
    });
    const select = document.querySelector<HTMLSelectElement>(
      'select[aria-label="Representation"]',
    )!;
    select.value = "space-fill";
    select.dispatchEvent(new Event("change", { bubbles: true }));
  });
  await expect(page.getByRole("alert")).toContainText(
    "Deliberate scene allocation failure",
  );
  await expect(page.getByRole("alert")).toContainText(
    "previous scene is still displayed",
  );
  expect(await pixels(page)).toEqual(before);
  await rotate(page);
  await expect(
    page.getByRole("button", { name: "Fit scene", exact: true }),
  ).toBeEnabled();
  await page
    .getByLabel("Representation", { exact: true })
    .selectOption("ball-stick");
  await expect(page.getByRole("alert")).toHaveCount(0);
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(250);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  expect(errors).toEqual([]);
});

test("256-cubed density meshes render and retain their requested resolution on save and reopen", async ({
  page,
}, testInfo) => {
  test.setTimeout(180_000);
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  await page.addInitScript(() => {
    const control = window as unknown as CacheProbedWindow;
    control.highResolutionCacheProbe = { sampling: 0, meshing: 0, grids: [] };
    const post = Worker.prototype.postMessage;
    Worker.prototype.postMessage = function (
      message,
      options?: StructuredSerializeOptions | Transferable[],
    ) {
      if (message.action === "generate" && message.args.resolution === 256) {
        this.addEventListener("message", ({ data }) => {
          if (data.id !== message.id) return;
          if (data.progress === "sampling")
            control.highResolutionCacheProbe.sampling++;
          if (data.progress === "meshing")
            control.highResolutionCacheProbe.meshing++;
          const grid = data.result?.grid;
          if (!grid) return;
          control.highResolutionCacheProbe.grids.push({
            dims: grid.dims,
            count: grid.values.length,
            typed: grid.values instanceof Float32Array,
            samples: [
              [0, 0, 0],
              [255, 255, 255],
              [128, 128, 128],
              [119, 137, 131],
            ].map(([i, j, k]) => ({
              position: grid.origin.map(
                (value: number, axis: number) =>
                  value +
                  i * grid.axes[0][axis] +
                  j * grid.axes[1][axis] +
                  k * grid.axes[2][axis],
              ),
              value: grid.values[(k * 256 + j) * 256 + i],
            })),
          });
        });
      }
      post.call(
        this,
        message,
        Array.isArray(options) ? { transfer: options } : options,
      );
    };
  });
  await ready(page);
  const prefix = path.resolve(
    `../artifacts/high-resolution-256-${testInfo.project.name}`,
  );
  const resolutions = [24, 32, 40, 48, 64, 80, 96, 128, 160, 192, 224, 256];
  await expect(
    page.getByLabel("Grid resolution", { exact: true }).locator("option"),
  ).toHaveText(resolutions.map((n) => `${n}\u00b3`));
  await page
    .getByRole("button", { name: "Browse orbitals", exact: true })
    .click();
  const browser = page.getByRole("dialog", { name: "Orbitals", exact: true });
  await expect(
    browser.getByLabel("Orbital grid resolution").locator("option"),
  ).toHaveText(resolutions.map((n) => `${n}\u00b3`));
  await browser.getByLabel("Orbital grid resolution").selectOption("256");
  await expect(
    browser.getByRole("button", { name: "Generate selected", exact: true }),
  ).toBeEnabled();
  await browser
    .getByRole("button", { name: "Close orbitals", exact: true })
    .click();
  await page
    .getByRole("button", { name: "Total electron density total", exact: true })
    .click();
  await page.getByLabel("Isovalue", { exact: true }).fill("0.08");
  // Leave only the density visible so pixel assertions cannot pass on an orbital.
  const remove = page.getByRole("button", { name: /Delete antibonding/ });
  while (await remove.count()) await remove.first().click();
  const source = await save(page, `${prefix}-before.molekel`);
  expect(source.id).toBe("analytic-h2-v1");
  expect(source.basis).toHaveLength(2);
  expect(source.surfaces).toEqual([]);
  await generate(page, 256);
  await expect(page.locator(".surface-row")).toHaveCount(1);
  const first = await page.evaluate(
    () => (window as unknown as CacheProbedWindow).highResolutionCacheProbe,
  );
  expect(first.sampling).toBe(1);
  expect(first.meshing).toBe(1);
  expect(first.grids).toHaveLength(1);
  expect(first.grids[0]).toMatchObject({
    dims: [256, 256, 256],
    count: 256 ** 3,
    typed: true,
  });
  const reference = await page.evaluate(
    async ({ document, samples }) => {
      const url = "/src/wasm/molekel_wasm.js";
      const core = await import(/* @vite-ignore */ url);
      await core.default();
      const json = JSON.stringify(document);
      return samples.map(({ position }) =>
        Math.fround(core.point(json, "total", ...position)),
      );
    },
    { document: source, samples: first.grids[0].samples },
  );
  for (const [i, sample] of first.grids[0].samples.entries()) {
    expect(Number.isFinite(sample.value)).toBeTruthy();
    expect(sample.value).toBe(reference[i]);
  }
  await page.getByLabel("Isovalue", { exact: true }).fill("0.1");
  await generate(page, 256);
  const cached = await page.evaluate(
    () => (window as unknown as CacheProbedWindow).highResolutionCacheProbe,
  );
  expect(cached.sampling).toBe(1);
  expect(cached.meshing).toBe(2);
  expect(cached.grids).toHaveLength(2);
  expect(cached.grids[1]).toEqual(cached.grids[0]);
  for (const mode of ["mesh", "raycast", "volume"]) {
    await page.getByLabel("Rendering mode").selectOption(mode);
    await expect
      .poll(async () => (await pixels(page)).positive, { timeout: 20_000 })
      .toBeGreaterThan(250);
    await page.screenshot({ path: `${prefix}-${mode}.png` });
  }
  await page.getByLabel("Rendering mode").selectOption("mesh");
  await rotate(page);
  const filename = `${prefix}-generated.molekel`;
  const generated = await save(page, filename);
  expect(generated).toEqual({
    ...source,
    view: { ...source.view, isovalue: 0.1 },
    surfaces: generated.surfaces,
  });
  expect(generated.surfaces).toHaveLength(1);
  expect(generated.surfaces[0]).toMatchObject({
    field: "total",
    isovalue: 0.1,
    resolution: [256, 256, 256],
    visible: true,
  });
  expect(generated.surfaces[0].source_hash).toMatch(/^[0-9a-f]{64}$/);
  expect(generated.surfaces[0].indices.length).toBeGreaterThan(0);
  await page.getByLabel("Open molecular file").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.locator(".surface-row")).toHaveCount(1);
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(250);
  expect(await save(page, `${prefix}-reopened.molekel`)).toEqual(generated);
  await page.setViewportSize({ width: 390, height: 844 });
  await page.getByRole("button", { name: "Fit scene", exact: true }).click();
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

for (const outcome of ["cancel", "worker-failure", "fatal-message"] as const) {
  test(`real 256-cubed Molden sampling stays responsive and preserves prior meshes after ${outcome}`, async ({
    page,
  }, testInfo) => {
    test.setTimeout(180_000);
    const errors: string[] = [];
    page.on("pageerror", (error) => errors.push(error.message));
    await page.addInitScript(() => {
      const control = window as unknown as ProbedWindow;
      control.highResolutionProbe = {
        sampling: false,
        completed: false,
        terminated: false,
      };
      let generatingWorker: Worker | null = null;
      let requestId: number | null = null;
      const post = Worker.prototype.postMessage;
      const terminate = Worker.prototype.terminate;
      Worker.prototype.postMessage = function (
        message,
        options?: StructuredSerializeOptions | Transferable[],
      ) {
        if (message.action === "generate" && message.args.resolution === 256) {
          generatingWorker = this;
          requestId = message.id;
          this.addEventListener("message", ({ data }) => {
            if (data.id !== message.id) return;
            if (data.progress === "sampling")
              control.highResolutionProbe.sampling = true;
            if ("result" in data || "error" in data)
              control.highResolutionProbe.completed = true;
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
          control.highResolutionProbe.terminated = true;
        terminate.call(this);
      };
      control.failHighResolutionWorker = (fatal) => {
        if (!generatingWorker)
          throw new Error("No high-resolution worker to fail");
        // Exercise the real worker client's crash path without exhausting browser memory.
        generatingWorker.dispatchEvent(
          fatal
            ? new MessageEvent("message", {
                data: {
                  id: requestId,
                  fatal: true,
                  error: "Deliberate scientific-worker allocation failure",
                },
              })
            : new ErrorEvent("error", {
                message: "Deliberate scientific-worker failure",
              }),
        );
      };
    });
    await ready(page);
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
    await page.getByLabel("Isovalue", { exact: true }).fill("0.08");
    await generate(page, 24);
    const prefix = path.resolve(
      `../artifacts/high-resolution-${outcome}-${testInfo.project.name}`,
    );
    const before = await save(page, `${prefix}-before.molekel`);
    await page
      .getByLabel("Grid resolution", { exact: true })
      .selectOption("256");
    await page
      .getByRole("button", { name: "Generate surfaces", exact: true })
      .click();
    await page.waitForFunction(
      () => (window as unknown as ProbedWindow).highResolutionProbe.sampling,
    );
    await expect(page.locator("footer [role=status]")).toContainText(
      "Sampling field",
    );
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
        completed: (window as unknown as ProbedWindow).highResolutionProbe
          .completed,
      };
    });
    expect(response.timer).toBeTruthy();
    expect(response.frames).toBeGreaterThan(1);
    expect(response.completed).toBeFalsy();
    await rotate(page);
    await expect(
      page.getByRole("button", { name: "Cancel", exact: true }),
    ).toBeEnabled();
    if (outcome === "cancel") {
      const start = Date.now();
      await page.getByRole("button", { name: "Cancel", exact: true }).click();
      await expect(page.locator("footer [role=status]")).toContainText(
        "Cancelled; previous surfaces retained",
      );
      expect(Date.now() - start).toBeLessThan(2000);
      await expect(page.getByRole("alert")).toHaveCount(0);
    } else {
      await page.evaluate(
        (fatal) =>
          (window as unknown as ProbedWindow).failHighResolutionWorker(fatal),
        outcome === "fatal-message",
      );
      await expect(page.getByRole("alert")).toContainText(
        outcome === "fatal-message"
          ? "Deliberate scientific-worker allocation failure"
          : "Deliberate scientific-worker failure",
      );
      await expect(
        page.getByRole("button", { name: "Generate surfaces", exact: true }),
      ).toBeEnabled();
    }
    expect(
      await page.evaluate(
        () => (window as unknown as ProbedWindow).highResolutionProbe,
      ),
    ).toEqual({
      sampling: true,
      completed: outcome === "fatal-message",
      terminated: true,
    });
    await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
    await expect(page.locator(".surface-row")).toHaveCount(1);
    expect(await save(page, `${prefix}-after.molekel`)).toEqual(before);
    await generate(page, 24);
    await expect
      .poll(async () => (await pixels(page)).positive)
      .toBeGreaterThan(250);
    expect(errors).toEqual([]);
  });
}
