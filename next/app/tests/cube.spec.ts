import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { MolekelDocument } from "../src/types";

const cube = readFileSync(path.resolve("../fixtures/cube/signed-affine.cube"));
const upload = {
  name: "signed-affine.cube",
  mimeType: "text/plain",
  buffer: cube,
};

async function ready(page: Page) {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  page.on("dialog", (dialog) => dialog.accept());
}

async function openCube(page: Page) {
  await page.getByLabel("Open molecular file").setInputFiles(upload);
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened signed-affine.cube",
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
  await expect(page.locator(".document-summary")).toContainText("2 atoms");
  await expect(page.locator(".document-summary")).toContainText("1 bond");
  await expect(page.locator(".document-summary")).toContainText(
    "0 basis functions",
  );
  await expect(
    page.getByRole("heading", { name: "Sampled fields", exact: true }),
  ).toBeVisible();
  await expect(
    page.getByRole("heading", { name: "Orbitals", exact: true }),
  ).toHaveCount(0);
  await expect(
    page.getByRole("heading", { name: "Density matrices", exact: true }),
  ).toHaveCount(0);
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

async function signedPixels(page: Page, minimum = 100) {
  // Hydrogen and its bond are gray: both colors must come from scalar data.
  await expect
    .poll(async () => (await pixels(page)).positive)
    .toBeGreaterThan(minimum);
  await expect
    .poll(async () => (await pixels(page)).negative)
    .toBeGreaterThan(minimum);
}

async function decode(page: Page, filename: string): Promise<MolekelDocument> {
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

function assertScalarData(doc: MolekelDocument) {
  expect(doc.atoms).toEqual([
    { element: 1, position: [-0.7, 0, 0] },
    { element: 1, position: [0.7, 0, 0] },
  ]);
  expect(doc.bonds).toEqual([[0, 1]]);
  expect(doc.basis).toEqual([]);
  expect(doc.orbitals).toEqual([]);
  expect(doc.densities).toEqual([]);
  expect(doc.grids).toHaveLength(1);
  const grid = doc.grids[0];
  expect(grid.origin).toEqual([-3.6, -3.3, -3]);
  expect(grid.axes).toEqual([
    [1, 0, 0],
    [0.2, 1, 0],
    [0, 0.1, 1],
  ]);
  expect(grid.dims).toEqual([7, 7, 7]);
  expect(grid.values).toHaveLength(343);
  for (let k = 0; k < 7; k++)
    for (let j = 0; j < 7; j++)
      for (let i = 0; i < 7; i++) {
        const x = -3.6 + i + 0.2 * j;
        const y = -3.3 + j + 0.1 * k;
        const z = -3 + k;
        const expected =
          0.8 *
          (x + 0.25 * y) *
          Math.exp(-0.7 * (x * x + 0.8 * y * y + 1.2 * z * z));
        expect(
          Math.abs(grid.values[(k * 7 + j) * 7 + i] - expected),
        ).toBeLessThan(1e-13);
      }
}

test("cube Open renders both scalar signs directly in volume and raycast on desktop/mobile", async ({
  page,
}, testInfo) => {
  const errors: string[] = [];
  page.on("pageerror", (error) => errors.push(error.message));
  page.on("console", (message) => {
    if (message.type() === "error") errors.push(message.text());
  });
  await ready(page);
  await openCube(page);
  await expect(page.getByLabel("Rendering mode")).toHaveValue("volume");
  await expect(page.locator(".surface-row")).toHaveCount(0);
  await expect(
    page.getByRole("button", { name: "Generate surfaces" }),
  ).toBeEnabled();
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await page.getByRole("slider", { name: "Opacity" }).press("Home");
  await expect
    .poll(async () => {
      const p = await pixels(page);
      return p.positive + p.negative;
    })
    .toBeLessThan(30);
  await page.getByRole("slider", { name: "Opacity" }).press("End");
  for (const mode of ["volume", "raycast"]) {
    await page.getByLabel("Rendering mode").selectOption(mode);
    await signedPixels(page);
    await expect(page.locator(".surface-row")).toHaveCount(0);
    await page.screenshot({
      path: `../artifacts/cube-${testInfo.project.name}-${mode}-desktop.png`,
    });
  }
  const before = await pixels(page);
  const canvas = (await page.locator("canvas").boundingBox())!;
  await page.mouse.move(
    canvas.x + canvas.width * 0.4,
    canvas.y + canvas.height * 0.5,
  );
  await page.mouse.down();
  await page.mouse.move(
    canvas.x + canvas.width * 0.7,
    canvas.y + canvas.height * 0.6,
    { steps: 12 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(before.checksum);
  await page.getByLabel("Grid resolution").selectOption("24");
  await expect(
    page.getByRole("button", { name: "Generate surfaces" }),
  ).toBeEnabled();
  await expect(page.locator(".surface-row")).toHaveCount(0);
  await page.setViewportSize({ width: 390, height: 844 });
  await page.getByRole("button", { name: "Fit scene", exact: true }).click();
  for (const mode of ["raycast", "volume"]) {
    await page.getByLabel("Rendering mode").selectOption(mode);
    await signedPixels(page, 35);
    await page.screenshot({
      path: `../artifacts/cube-${testInfo.project.name}-${mode}-mobile.png`,
      fullPage: true,
    });
  }
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= window.innerWidth,
    ),
  ).toBeTruthy();
  const scene = (await page.locator("canvas").boundingBox())!;
  const panel = (await page.locator(".left-panel").boundingBox())!;
  expect(scene.y + scene.height).toBeLessThanOrEqual(panel.y);
  expect(errors).toEqual([]);
});

test("cube meshes persist with the authoritative affine field, and native Open fills missing bonds", async ({
  page,
}, testInfo) => {
  await ready(page);
  await openCube(page);
  await page.getByLabel("Grid resolution").selectOption("24");
  await page.getByRole("button", { name: "Generate surfaces" }).click();
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect(page.locator(".surface-row")).toHaveCount(2);
  await page.getByLabel("Rendering mode").selectOption("mesh");
  await signedPixels(page);
  await page.screenshot({
    path: `../artifacts/cube-${testInfo.project.name}-meshes.png`,
  });
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const filename = path.resolve(
    `../artifacts/cube-${testInfo.project.name}-roundtrip.molekel`,
  );
  await (await download).saveAs(filename);
  const saved = await decode(page, filename);
  assertScalarData(saved);
  expect(saved.surfaces).toHaveLength(2);
  expect(saved.surfaces.map((surface) => surface.isovalue).sort()).toEqual([
    -0.08, 0.08,
  ]);
  for (const surface of saved.surfaces) {
    expect(surface.field).toBe(saved.grids[0].id);
    expect(surface.source_hash).toMatch(/^[0-9a-f]{64}$/);
    expect(surface.positions.length).toBeGreaterThan(0);
    expect(surface.indices.length).toBeGreaterThan(0);
  }
  await page.getByLabel("Open molecular file").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.getByLabel("Rendering mode")).toHaveValue("mesh");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  await signedPixels(page);
  for (const mode of ["raycast", "volume"]) {
    await expect(
      page.getByLabel("Rendering mode").locator(`option[value=${mode}]`),
    ).not.toHaveAttribute("disabled", "");
    await page.getByLabel("Rendering mode").selectOption(mode);
    await signedPixels(page);
  }
  // Model-valid older native documents can lack inferred display connectivity.
  const noBonds = await page.evaluate(async (doc) => {
    const url = "/src/wasm/molekel_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    return Array.from(
      core.encode(JSON.stringify({ ...doc, bonds: [] })) as Uint8Array,
    );
  }, saved);
  await page.getByLabel("Open molecular file").setInputFiles({
    name: "missing-bonds.molekel",
    mimeType: "application/octet-stream",
    buffer: Buffer.from(noBonds),
  });
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.locator(".document-summary")).toContainText("1 bond");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await expect(page.locator(".surface-row")).toHaveCount(2);
  const enrichedDownload = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const enrichedPath = path.resolve(
    `../artifacts/cube-${testInfo.project.name}-enriched.molekel`,
  );
  await (await enrichedDownload).saveAs(enrichedPath);
  const enriched = await decode(page, enrichedPath);
  assertScalarData(enriched);
  expect(enriched.surfaces).toEqual(saved.surfaces);
  expect(enriched.grids).toEqual(saved.grids);
});

test("cube batch conversion retains every scalar sample and bonds without generating meshes", async ({
  page,
}, testInfo) => {
  await ready(page);
  const originalTitle = await page.locator(".document-title").innerText();
  await page
    .getByRole("button", { name: "Convert Files", exact: true })
    .click();
  const dialog = page.getByRole("dialog", { name: "Convert Files" });
  await page.getByLabel("Files to convert").setInputFiles(upload);
  await dialog.getByRole("button", { name: "Convert", exact: true }).click();
  await expect(dialog.locator(".conversion-item.ready")).toHaveCount(1);
  const download = page.waitForEvent("download");
  await dialog
    .getByRole("button", { name: "Save signed-affine.molekel", exact: true })
    .click();
  const filename = path.resolve(
    `../artifacts/cube-${testInfo.project.name}-converted.molekel`,
  );
  await (await download).saveAs(filename);
  const converted = await decode(page, filename);
  assertScalarData(converted);
  expect(converted.surfaces).toEqual([]);
  await dialog.getByRole("button", { name: "Close conversion queue" }).click();
  await expect(page.locator(".document-title")).toHaveText(originalTitle);
  await page.getByLabel("Open molecular file").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened cube-",
  );
  await expect(page.getByLabel("Rendering mode")).toHaveValue("volume");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await expect(page.locator(".surface-row")).toHaveCount(0);
  await signedPixels(page);
});

test("an invalid cube leaves the existing sampled scene and scalar rendering intact", async ({
  page,
}) => {
  await ready(page);
  await openCube(page);
  const title = await page.locator(".document-title").innerText();
  await page.getByLabel("Open molecular file").setInputFiles({
    name: "truncated.cub",
    mimeType: "text/plain",
    buffer: Buffer.from(
      cube.toString("utf8").split("\n").slice(0, 10).join("\n"),
    ),
  });
  await expect(page.getByRole("alert")).not.toBeEmpty();
  await expect(page.locator(".document-title")).toHaveText(title);
  await expect(page.locator(".document-summary")).toContainText("1 bond");
  await expect(page.getByLabel("Rendering mode")).toHaveValue("volume");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  await signedPixels(page);
});

test("a finite scalar grid outside GPU precision still opens and saves without data loss", async ({
  page,
}, testInfo) => {
  await ready(page);
  await page.getByLabel("Open molecular file").setInputFiles({
    name: "large-finite.cube",
    mimeType: "text/plain",
    buffer: Buffer.from(
      [
        "Finite f64 scalar regression",
        "Valid scientific values outside f32 preview range",
        "2 -1 -1 -1",
        "2 2 0 0",
        "2 0 2 0",
        "2 0 0 2",
        "1 1 -0.7 0 0",
        "1 1 0.7 0 0",
        "1e100 1e100 1e100 1e100 1e100 1e100 1e100 1e100",
      ].join("\n"),
    ),
  });
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened large-finite.cube",
  );
  await expect(page.getByRole("alert")).toContainText(
    "Sampled preview unavailable",
  );
  await expect(page.locator(".document-summary")).toContainText("2 atoms");
  await expect(page.locator(".document-summary")).toContainText("1 bond");
  await expect(page.getByLabel("Rendering mode")).toHaveValue("mesh");
  await expect(
    page.getByLabel("Rendering mode").locator("option[value=volume]"),
  ).toHaveAttribute("disabled", "");
  await expect(
    page.getByLabel("Rendering mode").locator("option[value=raycast]"),
  ).toHaveAttribute("disabled", "");
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(1);
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const filename = path.resolve(
    `../artifacts/cube-${testInfo.project.name}-large-finite.molekel`,
  );
  await (await download).saveAs(filename);
  const saved = await decode(page, filename);
  expect(saved.grids).toHaveLength(1);
  expect(saved.grids[0].values).toEqual(Array(8).fill(1e100));
  expect(saved.bonds).toEqual([[0, 1]]);
  expect(saved.surfaces).toEqual([]);
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
});

test("volume rays remain visible from inside both ordinary and reflected affine grids", async ({
  page,
}) => {
  await ready(page);
  const result = await page.evaluate(async () => {
    const rendererUrl = "/src/volume.ts";
    const threeUrl = performance
      .getEntriesByType("resource")
      .map((entry) => entry.name)
      .find(
        (url) => new URL(url).pathname === "/node_modules/.vite/deps/three.js",
      );
    if (!threeUrl)
      throw new Error("The viewer did not load its Three.js module");
    const { volumeObject } = await import(/* @vite-ignore */ rendererUrl);
    const THREE = await import(/* @vite-ignore */ threeUrl);
    const renderer = new THREE.WebGLRenderer({ preserveDrawingBuffer: true });
    renderer.setSize(128, 128);
    renderer.setClearColor("#e8edef");
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    const camera = new THREE.PerspectiveCamera(45, 1, 0.01, 10);
    camera.position.set(0, 0, 0);
    camera.lookAt(0, 0, -1);
    const samples: { positive: number; center: number[]; glError: number }[] =
      [];
    for (const reflected of [false, true]) {
      const scene = new THREE.Scene();
      const object = volumeObject(
        {
          id: "inside-grid",
          label: "Inside-grid regression",
          quantity: "scalar",
          origin: [reflected ? 1 : -1, -1, -1],
          axes: [
            [reflected ? -1 : 1, 0, 0],
            [0, 1, 0],
            [0, 0, 1],
          ],
          dims: [3, 3, 3],
          values: Array(27).fill(1),
        },
        "volume",
        0.08,
        "#259d86",
        "#cb4e72",
        0.7,
        false,
      );
      scene.add(object);
      renderer.render(scene, camera);
      const gl = renderer.getContext() as WebGL2RenderingContext;
      const values = new Uint8Array(128 * 128 * 4);
      gl.readPixels(0, 0, 128, 128, gl.RGBA, gl.UNSIGNED_BYTE, values);
      let positive = 0;
      for (let i = 0; i < values.length; i += 4) {
        if (values[i + 1] > values[i] + 20 && values[i + 1] > values[i + 2] + 8)
          positive++;
      }
      const center = (64 * 128 + 64) * 4;
      samples.push({
        positive,
        center: Array.from(values.subarray(center, center + 4)),
        glError: gl.getError(),
      });
      object.geometry.dispose();
      object.material.dispose();
      object.userData.texture.dispose();
    }
    renderer.dispose();
    return samples;
  });
  for (const sample of result) {
    expect(sample.positive).toBeGreaterThan(15000);
    expect(sample.glError).toBe(0);
  }
  expect(result[1].center).toEqual(result[0].center);
});
