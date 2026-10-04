import { test, expect, type Page } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { EigenVistaDocument } from "../src/types";

async function ready(page: Page) {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  await expect(page.getByRole("alert")).toHaveCount(0);
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

test("EigenVista branding and unchanged atom logo fit desktop and mobile, with renamed downloads", async ({
  page,
}, testInfo) => {
  await ready(page);
  await expect(page).toHaveTitle("EigenVista");
  await expect(
    page.getByRole("heading", { name: "EigenVista", exact: true }),
  ).toBeVisible();
  await expect(page.locator(".brand svg.lucide-atom")).toHaveCount(1);
  const accept = await page
    .getByLabel("Open molecular file")
    .getAttribute("accept");
  expect(accept?.split(",")).toEqual(
    expect.arrayContaining([".eigenvista", ".molekel"]),
  );
  for (const width of [1440, 390, 320]) {
    await page.setViewportSize({ width, height: width === 1440 ? 960 : 844 });
    const layout = await page.locator(".topbar").evaluate((header) => {
      const selectors = [".brand svg", ".brand h1", ".file-actions"];
      return {
        width: document.documentElement.scrollWidth,
        boxes: selectors.map((selector) => {
          const element = header.querySelector(selector)!;
          const box = element.getBoundingClientRect();
          return {
            left: box.left,
            right: box.right,
            top: box.top,
            bottom: box.bottom,
            clipped: element.scrollWidth > element.clientWidth + 1,
          };
        }),
      };
    });
    expect(layout.width).toBeLessThanOrEqual(width);
    for (const [index, box] of layout.boxes.entries()) {
      expect(box.left).toBeGreaterThanOrEqual(0);
      expect(box.right).toBeLessThanOrEqual(width);
      expect(box.top).toBeGreaterThanOrEqual(0);
      expect(box.bottom).toBeLessThanOrEqual(65);
      expect(box.clipped).toBe(false);
      if (index > 0)
        expect(box.left).toBeGreaterThanOrEqual(layout.boxes[index - 1].right);
    }
    await page.screenshot({
      path: `../artifacts/branding-${width}-${testInfo.project.name}.png`,
      fullPage: true,
    });
  }
  const image = page.waitForEvent("download");
  await page.getByRole("button", { name: "Export image", exact: true }).click();
  expect((await image).suggestedFilename()).toBe("eigenvista-view.png");
  const documentDownload = page.waitForEvent("download");
  await page.getByTitle("Save document", { exact: true }).click();
  expect((await documentDownload).suggestedFilename()).toMatch(/\.eigenvista$/);
  await expect(page.getByRole("alert")).toHaveCount(0);
});

test("legacy Molekel documents reopen and save as EigenVista without changing scientific data or cached meshes", async ({
  page,
}, testInfo) => {
  await ready(page);
  page.on("dialog", (dialog) => dialog.accept());
  const legacy = path.resolve("../artifacts/references/legacy-water.molekel");
  const before = await decode(page, legacy);
  expect(before.surfaces.length).toBeGreaterThan(0);
  await page.getByLabel("Open molecular file").setInputFiles(legacy);
  await expect(page.locator("footer [role=status]")).toContainText(
    "Opened legacy-water.molekel",
  );
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.getByLabel("Unsaved changes")).toHaveCount(0);
  await expect(page.getByRole("alert")).toHaveCount(0);
  const downloaded = page.waitForEvent("download");
  await page.getByRole("button", { name: "Save", exact: true }).click();
  const saved = await downloaded;
  expect(saved.suggestedFilename()).toMatch(/\.eigenvista$/);
  const filename = path.resolve(
    `../artifacts/legacy-upgrade-${testInfo.project.name}.eigenvista`,
  );
  await saved.saveAs(filename);
  expect(await decode(page, filename)).toEqual(before);
  await page.getByLabel("Open molecular file").setInputFiles(filename);
  await expect(page.locator("footer [role=status]")).toContainText(
    `Opened legacy-upgrade-${testInfo.project.name}.eigenvista`,
  );
  await expect(page.locator("footer [role=status]")).toContainText(
    "saved geometry restored",
  );
  await expect(page.getByRole("alert")).toHaveCount(0);
});
