# Building and running from source

For an already packaged application, use the [user guide](user-guide.md).
This guide is for the rewrite under `next/`, not the root legacy C++ build.
Commands below start from the **repository root** unless explicitly stated.

## Prerequisites

The exercised environment is an Apple Silicon Mac running macOS 27, Rust
1.92.0, arm64 Node.js 26 with npm, and wasm-bindgen CLI **0.2.108**. These are
recorded tested versions, not a fully qualified minimum-version matrix. Cargo
and npm dependencies are locked; there is no checked-in Rust toolchain selector.

- Install Rust/rustup and Node.js/npm, and make their executables available in
  the shell. An arm64 Node installation is required by the tester packager.
- On macOS, install Xcode command-line tools (`xcode-select --install` if absent).
  Native platform prerequisites are described by [Tauri](https://v2.tauri.app/start/prerequisites/).
- Initial dependency installation/builds require network access. Running the
  completed native application on local files does not require a server.
- Python/PySCF is needed only to deliberately regenerate independent reference
  data, not to build the app or run the normal tests.

The browser frontend uses WebGL2. Linux also needs its distribution's Tauri/
WebKitGTK native prerequisites for the desktop shell; Windows has separate
toolchain/WebView2 requirements. Neither is an already verified release target.

## Fresh checkout setup

Check out the user-selected rewrite branch, then open a shell at its repository
root. Do not run root CMake or build legacy dependencies. If starting from the
tester source archive, extract it into a working directory and use that directory
as the root; it includes required test fixtures but no Git history.

```sh
mkdir -p tmp
export TMPDIR="$PWD/tmp" TMP="$PWD/tmp" TEMP="$PWD/tmp"
rustup toolchain install 1.92.0
export RUSTUP_TOOLCHAIN=1.92.0
rustup target add wasm32-unknown-unknown
cargo install wasm-bindgen-cli --version 0.2.108 --locked
npm --prefix next/app ci
```

Keep the temporary-directory environment in every build/test shell. **Never
use system `/tmp`.** Project wrappers also create/use the repository's ignored
`tmp/`, including child build tools. `RUSTUP_TOOLCHAIN` selects the documented
toolchain for this shell without changing another project's default. Installing
the CLI is needed once; its version must match the pinned wasm-bindgen crate.

Useful checks are `rustc --version`, `node --version`, `node -p process.arch`,
`wasm-bindgen --version`, and `xcode-select -p`. Do not solve a binding-version
mismatch by updating one side of the bridge independently.

## Run the viewer

Choose **one** development command:

```sh
npm --prefix next/app run dev
```

This builds WASM and starts Vite at `http://127.0.0.1:5178`. Open that address in
a WebGL2-capable browser. Stop your server with Ctrl-C when finished.

```sh
npm --prefix next/app run desktop
```

This starts Tauri and its own Vite server on the same port. Stop a separately
started server that you own before using this command; do not kill unknown
processes or the user's running packaged app. A separately installed tester
app does not use the development port.

The port is strict, not automatically incremented. If it is occupied by another
project, choose a free port and keep [Vite](../app/vite.config.ts),
[Tauri's devUrl](../app/src-tauri/tauri.conf.json), and both Playwright URLs in
[its config](../app/playwright.config.ts) aligned. Playwright currently reuses
an existing server: verify it belongs to this checkout before trusting results.

## Build outputs

| Command from the root | Result |
| --- | --- |
| `npm --prefix next/app run wasm` | Optimized Rust/WASM and web bindings in `next/app/src/wasm/` |
| `npm --prefix next/app run build` | WASM, TypeScript checking, and production frontend in `next/app/dist/` |
| `npm --prefix next/app run desktop:build` | Debug macOS bundle at `next/target/debug/bundle/macos/Molekel Preview.app` |
| `npm --prefix next/app run package:macos` | Verified optimized Apple Silicon tester ZIP under `next/artifacts/distributions/` |

The web build is an HTTP-served application, not a standalone `file://` HTML file.
For a local production-build preview, after `run build`:

```sh
npm --prefix next/app exec -- vite preview --host 127.0.0.1 --port 5180 --strictPort --outDir "$PWD/next/app/dist"
```

Open `http://127.0.0.1:5180`; choose another free port if needed. This command
explicitly locates the build output from the repository root. It is a local
preview server, not a deployment setup.

To launch a debug bundle already built on this Mac:

```sh
open "next/target/debug/bundle/macos/Molekel Preview.app"
```

The debug bundle is not Developer ID-signed/notarized and is not the supported
sharing workflow. Share the packager's verified ZIP instead of a raw target
bundle: the staged distributable also embeds notices and is re-signed after
staging. See [tester packaging](tester-packaging.md).

Do not set `CARGO_TARGET_DIR` to an arbitrary location for ordinary scripts:
`run.mjs` expects the workspace's `next/target/` layout. The packager explicitly
sets that location and uses an architecture-specific release subdirectory.
Generated outputs, downloaded dependencies, reports, and staging are ignored.
Keep `Cargo.lock` and `app/package-lock.json` under version control.

## Verification commands

Use the root-shell temporary-directory setup above, then:

```sh
cargo fmt --manifest-path next/Cargo.toml --all --check
npm --prefix next/app test
cargo clippy --manifest-path next/Cargo.toml --locked --workspace --all-targets -- -D warnings
npm --prefix next/app run build
npm --prefix next/app exec -- playwright install chromium webkit
npm --prefix next/app run test:e2e
npm --prefix next/app run test:packaging
```

`npm test` runs `cargo test --locked` for the default scientific/format/WASM
members, not the desktop shell. Workspace Clippy checks the shell too and needs
its platform prerequisites. A focused shell check is:

```sh
cargo check --manifest-path next/Cargo.toml --locked -p molekel-desktop
```

| Coverage | Location and meaning |
| --- | --- |
| Scientific model/evaluator | Core unit tests plus [independent references](../crates/molekel-core/tests/independent_references.rs) |
| XYZ/PDB and connectivity | [Connectivity regressions](../crates/molekel-core/tests/connectivity.rs), including read-only legacy fixtures |
| Container/native saving | Format unit tests and [reference roundtrips](../crates/molekel-format/tests/reference_roundtrips.rs) |
| Browser workflows/numerics | [Playwright tests](../app/tests): Chromium/WebKit, real file input/download, pixels, camera, desktop/mobile layout |
| Packaging safety/notices | [Packaging tests](../tools/package-support.test.mjs), followed by a full ZIP build when packaging changes |

`test:e2e` first generates native reference documents in
`next/artifacts/references/`, then runs Playwright. Its server command builds
WASM when starting a server. Inspect screenshots under `next/artifacts/` after
UI changes, including `desktop.png`, `mobile.png`, and reference/import scenes;
tests also assert canvas pixels and camera movement. Playwright failure output
lives in `next/app/test-results/`. Screenshots with shared names can be overwritten
by the second engine; retain specifically needed evidence before another run.

Numerical tests read frozen fixtures without Python. Deliberate regeneration
and extra `.molekel` example generation are documented in the
[fixture guide](../fixtures/pyscf/README.md). Do not regenerate expected values
as a substitute for investigating a regression. The optional release-mode bond
benchmark is:

```sh
cargo run --manifest-path next/Cargo.toml --locked --release -p molekel-core --example bond_benchmark
```

There is no active hosted rewrite CI workflow. [Portability checks](../ci/README.md)
are commands to run, not evidence that Linux or Windows has passed them.

## Manual native acceptance

For a change to file handling or packaging, separately verify the actual native
bundle: launch it, open the included water PDB/XYZ, check 3 atoms/2 bonds, save
to a new path, reopen, cancel a save, and exercise replacement/close behavior
without losing unsaved data. Test cached orbital meshes with a reference document.
Use disposable files under repository `tmp/`, never the user's original data.

Record what was actually observed, including the OS and app build. Browser
WebKit tests are not equivalent to Tauri's system WebView. Native filesystem
tests and signature checks do not establish native-dialog interaction or a
clean-machine Gatekeeper install. Coordinate with the user before using their
active application window. Current gaps are in [status](status.md).

## Troubleshooting

| Problem | Action |
| --- | --- |
| `../tmp` or temporary path missing | Create root `tmp/` first and export its absolute path as above |
| Missing WASM module or bindgen mismatch | Install the exact 0.2.108 CLI and target, then run `wasm` or `build`; do not commit generated bindings |
| Port 5178 occupied | Identify its owner; stop only your own server or align all three configs to a free port |
| Playwright browser missing | Run the browser-install command with repo-local temporary variables |
| Rust native link/build fails | Check platform prerequisites/toolchain, then the focused shell check; preserve diagnostics |
| New dependency notice fails packaging | Review the locked dependency's actual license and pinned notice source; do not bypass the check |
| Test refers to missing legacy PDB fixture | Use the full checkout or supplied source snapshot, not `next/` copied alone |
| Desktop display differs from browser | Record both runtimes; test the packaged system WebView separately |
| App file is rejected | Read the exact profile/validation error and import/format docs; do not weaken checks to accept malformed input |

For an agent taking over, continue with [handoff](handoff.md). For distributing
instead of developing, follow [tester packaging](tester-packaging.md).
