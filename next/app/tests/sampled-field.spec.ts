import { test, expect } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { Grid, EigenVistaDocument, Surface } from "../src/types";

test("owned WASM samples match legacy orbital and density paths and reject stale scientific inputs", async ({
  page,
}) => {
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  const result = await page.evaluate(async () => {
    const url = "/src/wasm/eigenvista_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    const json = core.example(false);
    const document: EigenVistaDocument = JSON.parse(json);
    const fields = [document.view.field!, document.densities[0].id];
    const reports = [];
    let freed = 0;
    for (const field of fields) {
      const legacyJSON = core.sample(json, field, 24);
      const { values, ...expectedMetadata }: Grid = JSON.parse(legacyJSON);
      const sampled = new core.SampledField(json, field, 24);
      try {
        const display: Float32Array = sampled.display_values();
        const samplesMatch =
          display.length === values.length &&
          display.every((value, index) =>
            Object.is(value, Math.fround(values[index])),
          );
        const metadata = JSON.parse(sampled.metadata());
        const meshes = [0.08, 0.04].map((iso) => {
          const cached = sampled.surfaces(json, iso);
          const legacy = core.surfaces(json, legacyJSON, field, iso);
          const surfaces: Surface[] = JSON.parse(cached);
          return {
            iso,
            exact: cached === legacy,
            levels: surfaces.map((surface) => surface.isovalue),
            vertices: surfaces.reduce(
              (sum, surface) => sum + surface.positions.length / 3,
              0,
            ),
          };
        });
        const afterMeshing: Float32Array = sampled.display_values();
        const samplesUnchanged = afterMeshing.every((value, index) =>
          Object.is(value, Math.fround(values[index])),
        );
        // A display export must not expose mutable access to the retained f64 grid.
        display[0] = 12345;
        const afterDisplayEdit: Float32Array = sampled.display_values();
        const displayIndependent = afterDisplayEdit.every((value, index) =>
          Object.is(value, Math.fround(values[index])),
        );
        const changed: EigenVistaDocument = JSON.parse(json);
        changed.basis[0].coefficients[0] *= 1.125;
        let sourceError = "";
        try {
          sampled.surfaces(JSON.stringify(changed), 0.08);
        } catch (error) {
          sourceError = String(error);
        }
        reports.push({
          field,
          metadata,
          expectedMetadata,
          typed: display instanceof Float32Array,
          sampleCount: display.length,
          samplesMatch,
          samplesUnchanged,
          displayIndependent,
          meshes,
          sourceError,
          recovered:
            sampled.surfaces(json, 0.08) ===
            core.surfaces(json, legacyJSON, field, 0.08),
        });
      } finally {
        sampled.free();
        freed++;
      }
    }
    let resolutionError = "";
    try {
      const unexpected = new core.SampledField(json, fields[0], 257);
      unexpected.free();
    } catch (error) {
      resolutionError = String(error);
    }
    return { reports, freed, resolutionError };
  });

  expect(result.freed).toBe(2);
  expect(result.reports).toHaveLength(2);
  for (const report of result.reports) {
    expect(report.metadata).toEqual(report.expectedMetadata);
    expect(report.metadata).not.toHaveProperty("values");
    expect(report.metadata.dims).toEqual([24, 24, 24]);
    expect(report.typed).toBeTruthy();
    expect(report.sampleCount).toBe(24 ** 3);
    expect(report.samplesMatch).toBeTruthy();
    expect(report.samplesUnchanged).toBeTruthy();
    expect(report.displayIndependent).toBeTruthy();
    expect(report.sourceError).toContain(
      "Cached samples do not match the current scientific inputs",
    );
    expect(report.recovered).toBeTruthy();
    for (const mesh of report.meshes) {
      expect(mesh.exact).toBeTruthy();
      expect(mesh.vertices).toBeGreaterThan(0);
      expect(mesh.levels).toContain(mesh.iso);
      if (report.field === "antibonding")
        expect(mesh.levels).toContain(-mesh.iso);
    }
  }
  expect(result.resolutionError).toContain("between 12 and 256");
});

test("owned WASM cube samples preserve signed affine source data and match legacy meshes", async ({
  page,
}) => {
  const bytes = Array.from(
    readFileSync(path.resolve("../fixtures/cube/signed-affine.cube")),
  );
  await page.goto("/");
  await expect(page.locator("footer [role=status]")).toContainText("triangles");
  const result = await page.evaluate(async (sourceBytes) => {
    const url = "/src/wasm/eigenvista_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    const { document }: { document: EigenVistaDocument } = JSON.parse(
      core.import_document(new Uint8Array(sourceBytes), "signed-affine.cube"),
    );
    const json = JSON.stringify(document);
    const originalGridJSON = JSON.stringify(document.grids);
    const field = document.grids[0].id;
    const legacyJSON = core.sample(json, field, 24);
    const { values, ...expectedMetadata }: Grid = JSON.parse(legacyJSON);
    const sampled = new core.SampledField(json, field, 24);
    let report;
    let freed = false;
    try {
      const display: Float32Array = sampled.display_values();
      const meshes = [0.08, 0.16].map((iso) => {
        const cached = sampled.surfaces(json, iso);
        const legacy = core.surfaces(json, legacyJSON, field, iso);
        const surfaces: Surface[] = JSON.parse(cached);
        return {
          iso,
          exact: cached === legacy,
          levels: surfaces.map((surface) => surface.isovalue),
          nonempty: surfaces.every((surface) => surface.indices.length > 0),
        };
      });
      const afterMeshing: Float32Array = sampled.display_values();
      const changed: EigenVistaDocument = JSON.parse(json);
      changed.grids[0].values[0] += 0.125;
      let sourceError = "";
      try {
        sampled.surfaces(JSON.stringify(changed), 0.08);
      } catch (error) {
        sourceError = String(error);
      }
      const reopened: EigenVistaDocument = JSON.parse(
        core.decode(core.encode(json)),
      );
      report = {
        metadata: JSON.parse(sampled.metadata()),
        expectedMetadata,
        typed: display instanceof Float32Array,
        sampleCount: display.length,
        samplesMatch: display.every((value, index) =>
          Object.is(value, Math.fround(values[index])),
        ),
        samplesUnchanged: afterMeshing.every((value, index) =>
          Object.is(value, Math.fround(values[index])),
        ),
        nativeSourceUnchanged:
          JSON.stringify(reopened.grids) === originalGridJSON,
        inputUnchanged: JSON.stringify(document) === json,
        meshes,
        sourceError,
        recovered:
          sampled.surfaces(json, 0.08) ===
          core.surfaces(json, legacyJSON, field, 0.08),
      };
    } finally {
      sampled.free();
      freed = true;
    }
    return { ...report, freed };
  }, bytes);

  expect(result.freed).toBeTruthy();
  expect(result.metadata).toEqual(result.expectedMetadata);
  expect(result.metadata).not.toHaveProperty("values");
  expect(result.metadata.dims).toEqual([7, 7, 7]);
  expect(result.metadata.origin).toEqual([-3.6, -3.3, -3]);
  expect(result.metadata.axes).toEqual([
    [1, 0, 0],
    [0.2, 1, 0],
    [0, 0.1, 1],
  ]);
  expect(result.typed).toBeTruthy();
  expect(result.sampleCount).toBe(343);
  expect(result.samplesMatch).toBeTruthy();
  expect(result.samplesUnchanged).toBeTruthy();
  expect(result.nativeSourceUnchanged).toBeTruthy();
  expect(result.inputUnchanged).toBeTruthy();
  expect(result.sourceError).toContain(
    "Cached samples do not match the current scientific inputs",
  );
  expect(result.recovered).toBeTruthy();
  expect(result.meshes).toHaveLength(2);
  for (const mesh of result.meshes!) {
    expect(mesh.exact).toBeTruthy();
    expect(mesh.nonempty).toBeTruthy();
    expect(mesh.levels).toEqual([mesh.iso, -mesh.iso]);
  }
});
