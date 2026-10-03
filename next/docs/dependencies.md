# Dependency record

The application code in this directory is new Rust and TypeScript. No legacy Molekel source is copied, no C++ source is added, and the C-library checkouts in the ignored repository `tmp/` are not build dependencies.

Direct dependency versions are resolved in the committed-source lockfiles `Cargo.lock` and `app/package-lock.json` (not yet committed by this task). Initial candidates:

| Dependency | Role | Upstream license |
| --- | --- | --- |
| mcubes 0.1.7 | Classic marching-cubes implementation | MIT |
| lin_alg | mcubes-compatible vector types | MIT |
| kiddo 5.2.4 | Exact immutable k-d-tree neighbor search, native/WASM | MIT OR Apache-2.0 |
| pdbtbx 0.12.0 | PDB atomic-record parser and published covalent-radius dataset | MIT |
| serde / serde_json | Structured model serialization | MIT OR Apache-2.0 |
| sha2 | Scientific and binary-array hashes | MIT OR Apache-2.0 |
| zip | Portable native document container | MIT |
| wasm-bindgen 0.2.108 | Shared Rust browser bindings | MIT OR Apache-2.0 |
| Tauri 2 and its dialog/fs plugins | Desktop shell and local file selection | MIT OR Apache-2.0 |
| React / React DOM | Interface | MIT |
| Three.js | Scene graph, camera controls, WebGL2 renderer | MIT |
| lucide-react | Interface icons | ISC |
| Vite / TypeScript | Frontend build | MIT / Apache-2.0 |
| Playwright | Browser workflow and rendering checks | Apache-2.0 |
| Prettier | Source formatting | MIT |

The app icon is newly authored in `app/src-tauri/icons/source.svg`; bitmap/native icon formats are generated with the Tauri icon tool. It is not copied from the legacy application.

Independent numerical reference generation uses PySCF 2.14.0 (Apache-2.0) in an ignored development-only Python environment under repository `tmp/`. Its compiled numerical libraries are not linked into Molekel or shipped in the desktop/browser builds. The frozen generated data and conventions are documented in [fixtures/pyscf/README.md](../fixtures/pyscf/README.md); generator dependencies are pinned in `tools/reference-requirements.txt`. Normal tests require only Rust and the existing frontend toolchain.

This is a development inventory, not a completed release-license audit. Before distribution, collect notices from exact locked transitive dependencies, check platform libraries, choose the new application's license, and include the required notices in the bundle. A debug bundle is not a signed/notarized public release.
