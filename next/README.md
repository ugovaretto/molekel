# Molekel rewrite

A clean Rust scientific core with a React/Three.js interface, shared by a Tauri macOS application and the browser. This directory is independent of the legacy implementation. No legacy source is copied or built, and no C++ is used in the application.

**Status:** first working development preview, not v1 and not completion of the M0-M7 plan. See [implementation status](docs/status.md) for evidence and remaining gates.

## Start here

- **Using the application:** [User guide](docs/user-guide.md), including Mac
  installation, supported files, controls, surfaces, saving, and troubleshooting.
- **Building or running from source:** [Development guide](docs/development.md).
- **Sharing a tester ZIP:** [Packaging guide](docs/tester-packaging.md).
- **Continuing development:** [Root AGENTS.md](../AGENTS.md),
  [handoff](docs/handoff.md), and [as-built architecture](docs/architecture.md).
- **Planning and all documentation:** [Documentation map](docs/README.md).

The historical research and M0-M7 plan remain in `../doc/rewrite/`. They record
the initial proposal; current implementation facts and later decisions are in
`docs/`. In particular, proposed features are not all available in this preview.

## Run

Tested toolchain: Rust 1.92.0, Node 26, wasm-bindgen CLI 0.2.108. Dependency versions are locked in Cargo.lock and app/package-lock.json.

From `next/`:

```sh
mkdir -p ../tmp
export TMPDIR="$(cd ../tmp && pwd)"
export TMP="$TMPDIR" TEMP="$TMPDIR"
rustup target add wasm32-unknown-unknown
cargo install wasm-bindgen-cli --version 0.2.108 --locked
cd app
npm ci
npm run dev
```

The development viewer is served at `http://127.0.0.1:5178`. The port is deliberately strict; choose another in Vite, Tauri, and Playwright config if it is occupied. All project scripts use the ignored repository `tmp/` for temporary work.

```sh
npm run desktop        # Tauri development app, starts its own Vite server
npm run desktop:build  # Debug macOS .app, not Developer ID-signed/notarized
npm run package:macos  # Optimized, ad-hoc-signed Apple Silicon tester ZIP
npm run build          # WASM, TypeScript check, production web bundle
npm test               # Rust scientific and native-format tests
npm run test:e2e       # Chromium/WebKit interaction, persistence, pixel and mobile tests
npm run test:packaging # Packaging safety and license-notice regression tests
```

Stop an independently started Vite server before `npm run desktop`. Install Playwright's browsers once with `npm exec playwright -- install chromium webkit`, using the temporary-directory variables above.

For sharing with friends, see [tester packaging](docs/tester-packaging.md).
The ZIP is a development build for Apple Silicon Macs, not an Apple-notarized
release. It includes source and notices under the [application license](LICENSE).

## Available

- Analytic two-center and fractional-open-shell fixtures, clearly identified as mathematical models rather than chemistry calculations.
- Frozen independent PySCF references for closed/open-shell molecules and Cartesian/spherical general contractions through G, with native/WASM value and gradient checks. [Reference suite and example documents](fixtures/pyscf/README.md).
- Explicit Gaussian-polynomial basis evaluation and gradients through G, signed orbitals, and supplied real density matrices in Rust/f64.
- Ball-and-stick, liquorice, and space-filling representations; camera controls, atom picking, and image export.
- Rust marching-cubes mesh extraction, signed colors, opacity, visibility, deletion, sampling/meshing/validation progress, worker cancellation, and sampled-field reuse.
- Searchable orbital table with energy, occupation, spin, and saved-surface counts;
  select up to 32 orbitals per batch to generate persistent signed meshes together.
  Cancel or failure preserves prior geometry, and successful batches retain
  surfaces belonging to unselected fields. See the [user guide](docs/user-guide.md).
- Experimental sampled raycasting and volume integration with actual shaders; these do not yet meet the full correctness/performance release gates.
- Self-contained `.molekel` preview files containing quantum data, binary numeric arrays, saved meshes, isovalues, grid geometry, scientific hashes, provenance, and material settings. Reopen displays saved geometry without recomputation.
- Native Save dialog with validated sibling-file replacement; browser file download. Native Open dialog and browser file selection.
- Conventional single-frame XYZ and PDB import with automatic coordinate-based bonds, PDB alternate/model handling, and retained explicit connections. [Import policy and algorithm](docs/structure-imports.md).
- Gaussian single-field bohr cube import, including single-orbital datasets,
  skewed/reflected affine axes, automatic bonds, and immediate volume/raycast
  previews. Generate signed meshes and save them with the original grid in
  `.molekel`. [Cube profile and workflow](docs/cube-import.md).
- Automatic display bonds across every supported import, including filling
  missing bonds in older native documents without losing cached surfaces.
- Molden import through a shared Rust library, Open and batch conversion controls,
  plus a standalone `molekel-convert` CLI. Canonical Gaussian shells through G
  and a tested ORCA S/P/spherical-D export subset are supported with explicit
  normalization and loss reports. [Molden profile and converter](docs/molden-import.md).

## Boundaries

The mesher is the MIT-licensed Rust `mcubes` 0.1.7 library, not MC33. Its classic table is not topology-certified. The numerical evaluator is a scalar Rust reference implementation, not an optimized collocation kernel. Neither substitution closes the relevant scientific/performance gates.

Raycasting uses bounded step sampling and bracket refinement. It can miss tangencies or multiple crossings between steps. Transparency uses ordinary mesh compositing and an experimental single-volume pass; opaque objects inside volumes and interpenetrating transparent layers are not yet correctly integrated. Do not treat these previews as publication-quality rendering.

The format's named preview profile is intentionally narrower than the proposed full format. It has whole-array ZIP entries and a 128 MiB file/inflated-byte cap; it is not a streaming/chunked large-data implementation. Saving may need several in-memory copies. Camera and transient volume state are not yet persisted. See [format profile](docs/preview-format.md).

Analytic sampling supports 12 through 256 points per axis in the API. Both UI
selectors offer 24, 32, 40, 48, 64, 80, 96, 128, 160, 192, 224, and 256; the
default remains 40. A transient sampled field may contain up to 256 cubed
values, separately from the unchanged 128 cubed total-sample limit for imported
and native authoritative grids. The two-million-vertex mesher limit now checks
the actual case counts before extraction, using the pinned `mcubes` behavior;
it is not a new meshing algorithm or topology guarantee.

Other limits remain: 256 AOs, 64 primitives per AO, 128 saved surfaces, 32
orbitals per batch, and 128 MiB geometry/native-container bounds, plus numerical
and domain validation. There is no estimated CPU-work cutoff or silent
resolution reduction for analytic sampling. Scalar sampling avoids unused
gradients and folds density-matrix pairs. The WASM worker keeps the f64 sampled
grid and transfers full-resolution f32 display values without serializing the
large sample array through JSON.

A 256 cubed field contains 16,777,216 samples: 128 MiB for f64 values plus
64 MiB for f32 display values, before meshes, additional copies, and GPU memory.
Larger jobs may take longer; phase progress is not a percentage or time estimate.
Cancel terminates the worker, retaining previous geometry, and recoverable
errors leave the previous document intact. Allocation checks cannot guarantee
recovery from every library, browser, GPU, or system out-of-memory failure;
these bounds do not guarantee latency or maximum process memory usage.

Imported cubes can be resampled for display without modifying their
authoritative values. All imports use published covalent radii and bounded
neighbor search for automatic bonds; these are display connectivity, not
inferred bond orders. PDB import selects the first geometry and one alternate
per residue. Van der Waals display-radius defaults outside the small explicit
element table remain provisional and are independent of the complete bond-radius
dataset.

Direct analytic shader raycasting, topology-certified MC33, shrinkwrap, full transfer-function editing, OBJ import, atom-color text import, metadata editing, undo, and release qualification remain to be implemented. Other external formats are deferred; no direct ORCA reader is planned for this increment. Linux and Windows are architectural targets, not tested supported releases.
