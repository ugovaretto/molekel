import assert from "node:assert/strict";
import fs from "node:fs";
import path from "node:path";
import { pathToFileURL } from "node:url";

const [wasm, examples] = process.argv.slice(2);
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
console.log("Packaged examples: both load with 3 atoms and 2 bonds.");
