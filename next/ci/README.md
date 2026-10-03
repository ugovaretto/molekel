# Portability checks

The shared core has no platform-specific dependencies. The same commands run on macOS and Linux after installing Rust, Node, the WASM target, and wasm-bindgen CLI 0.2.108:

```sh
cd next
mkdir -p ../tmp
export TMPDIR="$(cd ../tmp && pwd)"
export TMP="$TMPDIR" TEMP="$TMPDIR"
cargo fmt --all --check
cargo test --locked
cargo clippy --locked --all-targets -- -D warnings
cd app
npm ci
npm run build
npm exec playwright -- install chromium webkit
npm run test:e2e
```

The independent chemistry fixtures are frozen test data, so Python/PySCF is not required in CI. The browser test runner first builds three native `.molekel` reference documents under ignored `next/artifacts/references/`, then checks native-to-browser persistence and the Rust/WASM results against those references. See [fixture provenance and regeneration](../fixtures/pyscf/README.md).

`cargo check -p molekel-desktop --locked` additionally checks the Tauri shell. Linux needs the distribution's Tauri/WebKitGTK development dependencies. A desktop build does not establish GPU or window-system compatibility.

No active root `.github/workflows` file was added: implementation is confined to `next/`, as requested. These checks are runnable locally or from a future CI job. Linux and Windows execution has not been verified on this Mac; do not mark those platforms supported based on the WASM build.
