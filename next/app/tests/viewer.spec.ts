import { test, expect, type Page } from "@playwright/test";
import path from "node:path";

async function ready(page: Page) {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText(
    "triangles",
    { timeout: 45000 },
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
}
async function pixels(page: Page) {
  return page.locator("canvas").evaluate((canvas) => {
    const gl = (canvas as HTMLCanvasElement).getContext("webgl2")!;
    const pixels = new Uint8Array(
      gl.drawingBufferWidth * gl.drawingBufferHeight * 4,
    );
    gl.readPixels(
      0,
      0,
      gl.drawingBufferWidth,
      gl.drawingBufferHeight,
      gl.RGBA,
      gl.UNSIGNED_BYTE,
      pixels,
    );
    let colorful = 0,
      checksum = 0;
    for (let i = 0; i < pixels.length; i += 4) {
      if (
        Math.max(pixels[i], pixels[i + 1], pixels[i + 2]) -
          Math.min(pixels[i], pixels[i + 1], pixels[i + 2]) >
        30
      )
        colorful++;
      checksum =
        (checksum + pixels[i] * ((i % 97) + 1) + pixels[i + 1]) % 2147483647;
    }
    return { colorful, checksum, total: pixels.length / 4 };
  });
}
test("desktop scene, orbit interaction, raycast and volume are nonblank", async ({
  page,
}) => {
  const errors: string[] = [];
  page.on("pageerror", (e) => errors.push(e.message));
  page.on("console", (msg) => {
    if (msg.type() === "error") errors.push(msg.text());
  });
  await ready(page);
  const initial = await pixels(page);
  expect(initial.colorful).toBeGreaterThan(initial.total * 0.01);
  await page.screenshot({ path: "../artifacts/desktop.png" });
  const canvas = await page.locator("canvas").boundingBox();
  await page.mouse.move(
    canvas!.x + canvas!.width * 0.4,
    canvas!.y + canvas!.height / 2,
  );
  await page.mouse.down();
  await page.mouse.move(
    canvas!.x + canvas!.width * 0.7,
    canvas!.y + canvas!.height * 0.6,
    { steps: 15 },
  );
  await page.mouse.up();
  await expect
    .poll(async () => (await pixels(page)).checksum)
    .not.toBe(initial.checksum);
  await page.getByRole("button", { name: "Fit scene", exact: true }).click();
  for (const mode of ["raycast", "volume"]) {
    await page.getByLabel("Rendering mode").selectOption(mode);
    await expect(page.locator(".scene-label")).toContainText(
      mode === "raycast" ? "Sampled raycast" : "Volume preview",
    );
    await page.evaluate(
      () =>
        new Promise((resolve) =>
          requestAnimationFrame(() => requestAnimationFrame(resolve)),
        ),
    );
    await expect
      .poll(async () => (await pixels(page)).colorful, { timeout: 20000 })
      .toBeGreaterThan(500);
    await page.screenshot({ path: `../artifacts/${mode}.png` });
  }
  expect(errors).toEqual([]);
});
test("portable save/reopen retains both orbital signs without calculation", async ({
  page,
}) => {
  await ready(page);
  await page.getByRole("slider", { name: "Opacity" }).press("End");
  await page.getByRole("slider", { name: "Opacity" }).press("ArrowLeft");
  await expect(page.locator("output")).toHaveText("99%");
  const download = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const saved = await download;
  const filename = path.resolve("../artifacts/roundtrip.molekel");
  await saved.saveAs(filename);
  await page
    .getByRole("button", { name: /Delete antibonding/ })
    .first()
    .click();
  page.on("dialog", (d) => d.accept());
  await page.locator("input[type=file]").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(
    page.getByRole("button", { name: /Delete antibonding/ }),
  ).toHaveCount(2);
  await expect(
    page.getByLabel("Rendering mode").locator("option[value=raycast]"),
  ).toHaveAttribute("disabled", "");
  await expect(page.locator("output")).toHaveText("99%");
  expect((await pixels(page)).colorful).toBeGreaterThan(500);
});
test("cancelling a worker retains the scene and a fresh worker can run", async ({
  page,
}) => {
  await ready(page);
  const result = await page.evaluate(async () => {
    const url = "/src/science.ts";
    const science = await import(/* @vite-ignore */ url);
    const doc = await science.request("example", { openShell: false });
    const pending = science.request("generate", {
      doc,
      field: "total",
      resolution: 48,
      iso: 0.08,
    });
    science.cancel();
    let cancelled = false;
    try {
      await pending;
    } catch (error) {
      cancelled = String(error).includes("cancelled");
    }
    const fresh = await science.request("example", { openShell: true });
    return { cancelled, id: fresh.id };
  });
  expect(result).toEqual({ cancelled: true, id: "analytic-open-shell-v1" });
  await expect(
    page.getByRole("button", { name: /Delete antibonding/ }),
  ).toHaveCount(2);
  expect((await pixels(page)).colorful).toBeGreaterThan(500);
});
test("native chemistry reference opens cached orbital and density meshes", async ({
  page,
}) => {
  await ready(page);
  page.on("dialog", (d) => d.accept());
  await page
    .locator("input[type=file]")
    .setInputFiles(
      path.resolve("../artifacts/references/water-rhf-ccpvdz.molekel"),
    );
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
  await expect(
    page.getByRole("button", { name: /Delete spatial-5/ }),
  ).toHaveCount(2);
  await expect(page.getByRole("button", { name: /Delete total/ })).toHaveCount(
    1,
  );
  await expect(
    page.getByLabel("Rendering mode").locator("option[value=raycast]"),
  ).toHaveAttribute("disabled", "");
  await expect
    .poll(async () => (await pixels(page)).colorful)
    .toBeGreaterThan(500);
  await page.screenshot({ path: "../artifacts/water-reference.png" });
});
test("explicit density matrix renders and invalid import keeps current document", async ({
  page,
}) => {
  await ready(page);
  await page
    .getByRole("button", { name: "Total electron density total" })
    .click();
  await page.getByRole("button", { name: "Generate surfaces" }).click();
  await expect(page.getByRole("button", { name: /Delete total/ })).toHaveCount(
    1,
  );
  page.on("dialog", (d) => d.accept());
  await page.locator("input[type=file]").setInputFiles({
    name: "broken.xyz",
    mimeType: "text/plain",
    buffer: Buffer.from("2\nbad\nH 0 0 0\n"),
  });
  await expect(page.getByRole("alert")).toContainText("Truncated XYZ");
  await expect(page.getByRole("button", { name: /Delete total/ })).toHaveCount(
    1,
  );
});
test("mobile layout and Rust WASM analytic reference", async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 });
  await ready(page);
  expect(
    await page.evaluate(
      () => document.documentElement.scrollWidth <= window.innerWidth,
    ),
  ).toBeTruthy();
  expect((await pixels(page)).colorful).toBeGreaterThan(300);
  await page.screenshot({ path: "../artifacts/mobile.png", fullPage: true });
  const value = await page.evaluate(async () => {
    const url = "/src/wasm/molekel_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    return core.point(core.example(true), "spin", 0.4, 0.1, 0.7);
  });
  const expected =
    0.75 * Math.pow(2 / Math.PI, 1.5) * Math.exp(-2 * (0.16 + 0.01 + 0.49));
  expect(Math.abs(value - expected)).toBeLessThan(1e-12);
});
