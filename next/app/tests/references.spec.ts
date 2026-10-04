import { test, expect } from "@playwright/test";
import { readFileSync } from "node:fs";
import path from "node:path";
import type { EigenVistaDocument } from "../src/types";

interface ReferenceFixture {
  document: EigenVistaDocument;
  reference: {
    points: [number, number, number][];
    fields: { id: string; samples: [number, number, number, number][] }[];
    grid_field: string;
  };
}

test("native meshes with signed-zero inputs survive browser material edits", async ({
  page,
}) => {
  const bytes = Array.from(
    readFileSync(
      path.resolve(
        "../artifacts/references/general-spdfg-spherical.eigenvista",
      ),
    ),
  );
  await page.goto("/");
  const result = await page.evaluate(async (bytes) => {
    const url = "/src/wasm/eigenvista_wasm.js";
    const core = await import(/* @vite-ignore */ url);
    await core.default();
    const doc = JSON.parse(
      core.decode(new Uint8Array(bytes)),
    ) as EigenVistaDocument;
    const negativeZeros = doc.densities
      .flatMap((d) => d.matrix)
      .filter((v) => Object.is(v, -0)).length;
    const hashes = doc.surfaces.map((s) => s.source_hash);
    doc.surfaces[0].opacity = 0.41;
    doc.surfaces[0].color = "#186f92";
    const json = JSON.stringify(doc);
    core.validate(json);
    const restored = JSON.parse(
      core.decode(core.encode(json)),
    ) as EigenVistaDocument;
    return {
      negativeZeros,
      hashes,
      restoredHashes: restored.surfaces.map((s) => s.source_hash),
      surfaces: restored.surfaces.length,
      opacity: restored.surfaces[0].opacity,
      color: restored.surfaces[0].color,
    };
  }, bytes);
  expect(result.negativeZeros).toBeGreaterThan(0);
  expect(result.restoredHashes).toEqual(result.hashes);
  expect(result.surfaces).toBe(2);
  expect(result.opacity).toBe(0.41);
  expect(result.color).toBe("#186f92");
});

for (const name of [
  "water-rhf-ccpvdz",
  "hydroxyl-uhf-sto3g",
  "general-spdfg-spherical",
  "general-spdfg-cartesian",
]) {
  const fixture: ReferenceFixture = JSON.parse(
    readFileSync(path.resolve(`../fixtures/pyscf/${name}.json`), "utf8"),
  );
  test(`WASM values, gradients and native roundtrip match PySCF: ${name}`, async ({
    page,
  }) => {
    await page.goto("/");
    const result = await page.evaluate(async (fixture) => {
      const url = "/src/wasm/eigenvista_wasm.js";
      const core = await import(/* @vite-ignore */ url);
      await core.default();
      const json = JSON.stringify(fixture.document);
      const decoded = core.decode(core.encode(json));
      let maxError = 0;
      let maxScaledError = 0;
      let comparisons = 0;
      for (const field of fixture.reference.fields) {
        for (let p = 0; p < fixture.reference.points.length; p++) {
          const actual = core.point_with_gradient(
            decoded,
            field.id,
            ...fixture.reference.points[p],
          );
          for (let component = 0; component < 4; component++) {
            const expected = field.samples[p][component];
            const error = Math.abs(actual[component] - expected);
            maxError = Math.max(maxError, error);
            maxScaledError = Math.max(
              maxScaledError,
              error / (1e-10 + 1e-8 * Math.abs(expected)),
            );
            comparisons++;
          }
        }
      }
      let invalidRejected = false;
      try {
        core.point_with_gradient(
          decoded,
          fixture.reference.fields[0].id,
          NaN,
          0,
          0,
        );
      } catch {
        invalidRejected = true;
      }
      return {
        maxError,
        maxScaledError,
        comparisons,
        invalidRejected,
        document: JSON.parse(decoded),
      };
    }, fixture);
    // Compare to the actual JSON input: JSON.stringify canonicalizes signed zero.
    expect(result.document).toEqual(
      JSON.parse(JSON.stringify(fixture.document)),
    );
    expect(result.comparisons).toBeGreaterThan(400);
    expect(result.maxScaledError).toBeLessThanOrEqual(1);
    expect(result.invalidRejected).toBeTruthy();
    test.info().annotations.push({
      type: "reference-error",
      description: `max absolute error: ${result.maxError}; ${result.comparisons} value/gradient comparisons`,
    });
  });
}
