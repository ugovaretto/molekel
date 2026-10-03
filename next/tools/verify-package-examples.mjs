import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";
import { spawnSync } from "node:child_process";

const [wasm, examples, converter, output] = process.argv.slice(2);
const core = await import(pathToFileURL(path.join(wasm, "molekel_wasm.js")));
await core.default({
  module_or_path: fs.readFileSync(path.join(wasm, "molekel_wasm_bg.wasm")),
});
for (const name of ["water.pdb", "water.xyz"]) {
  const document = JSON.parse(
    core.import_text(fs.readFileSync(path.join(examples, name), "utf8"), name),
  );
  assert.equal(document.atoms.length, 3, `${name}: atom count`);
  assert.equal(document.bonds.length, 2, `${name}: inferred bond count`);
}
const molden = path.join(examples, "water.molden");
const imported = JSON.parse(
  core.import_document(fs.readFileSync(molden), "water.molden"),
);
assert.equal(imported.report.format, "molden");
assert.equal(imported.document.atoms.length, 3);
assert.equal(imported.document.bonds.length, 2);
assert.equal(imported.document.basis.length, 24);
assert.ok(imported.document.orbitals.length > 0);
assert.ok(imported.document.densities.length > 0);
const converted = spawnSync(converter, ["--json", "--output", output, molden], {
  env: process.env,
  encoding: "utf8",
});
assert.equal(converted.status, 0, converted.stderr);
assert.equal(JSON.parse(converted.stdout)[0].status, "converted");
assert.deepEqual(
  JSON.parse(core.decode(fs.readFileSync(output))),
  imported.document,
);
const checked = spawnSync(converter, ["--check", output], {
  env: process.env,
  encoding: "utf8",
});
assert.equal(checked.status, 0, checked.stderr);
const cubeName = "signed-affine.cube";
const cube = JSON.parse(
  core.import_document(
    fs.readFileSync(path.join(examples, cubeName)),
    cubeName,
  ),
);
assert.equal(cube.document.atoms.length, 2);
assert.deepEqual(cube.document.bonds, [[0, 1]]);
assert.equal(cube.document.basis.length, 0);
assert.equal(cube.document.grids[0].values.length, 343);
const grid = core.sample(JSON.stringify(cube.document), "cube", 24);
const surfaces = JSON.parse(
  core.surfaces(JSON.stringify(cube.document), grid, "cube", 0.08),
);
assert.ok(surfaces.some((s) => s.isovalue > 0));
assert.ok(surfaces.some((s) => s.isovalue < 0));
const cubeOutput = `${output}.cube.molekel`;
const cubeConverted = spawnSync(
  converter,
  ["--output", cubeOutput, path.join(examples, cubeName)],
  { env: process.env, encoding: "utf8" },
);
assert.equal(cubeConverted.status, 0, cubeConverted.stderr);
assert.deepEqual(
  JSON.parse(core.decode(fs.readFileSync(cubeOutput))),
  cube.document,
);
console.log(
  "Packaged PDB/XYZ/Molden/cube examples, signed cube meshes, and converter/WASM roundtrips passed.",
);
