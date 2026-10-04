import { spawnSync } from "node:child_process";
import { mkdirSync } from "node:fs";
import { fileURLToPath } from "node:url";
import path from "node:path";

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "..");
const app = path.join(root, "app");
const tmp = path.resolve(root, "../tmp");
mkdirSync(tmp, { recursive: true });
const env = { ...process.env, TMPDIR: tmp, TMP: tmp, TEMP: tmp };
function run(command, args, cwd = root) {
  const result = spawnSync(command, args, {
    cwd,
    env,
    stdio: "inherit",
    shell: process.platform === "win32",
  });
  if (result.error) {
    console.error(result.error.message);
    process.exit(1);
  }
  if (result.status !== 0) process.exit(result.status ?? 1);
}
function wasm() {
  run("cargo", [
    "build",
    "--locked",
    "--release",
    "-p",
    "eigenvista-wasm",
    "--target",
    "wasm32-unknown-unknown",
  ]);
  run("wasm-bindgen", [
    "target/wasm32-unknown-unknown/release/eigenvista_wasm.wasm",
    "--target",
    "web",
    "--out-dir",
    "app/src/wasm",
  ]);
}
const bin = (name) => path.join(app, "node_modules", ".bin", name);
switch (process.argv[2]) {
  case "wasm":
    wasm();
    break;
  case "dev":
    wasm();
    run(bin("vite"), ["--host", "127.0.0.1"], app);
    break;
  case "build":
    wasm();
    run(bin("tsc"), ["--noEmit"], app);
    run(bin("vite"), ["build"], app);
    break;
  case "test":
    run("cargo", ["test", "--locked"]);
    break;
  case "e2e":
    run("cargo", [
      "run",
      "--locked",
      "-p",
      "eigenvista-format",
      "--example",
      "reference_documents",
    ]);
    run(bin("playwright"), ["test"], app);
    break;
  case "desktop":
    run(bin("tauri"), ["dev"], app);
    break;
  case "desktop-build":
    run(bin("tauri"), ["build", "--debug", "--bundles", "app"], app);
    break;
  default:
    throw new Error(
      "Expected wasm, dev, build, test, e2e, desktop, or desktop-build",
    );
}
