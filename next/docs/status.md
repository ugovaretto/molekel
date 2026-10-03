# Implementation status

Date: 3 October 2026. Branch: `2026`. All new implementation is under `next/`; legacy source, data, and historical research remain untouched. Root `AGENTS.md` is the user-requested development entry point. The previously requested root ignore rule excludes repository-local `tmp/`.

Start with the [documentation map](README.md), [user guide](user-guide.md),
[as-built architecture](architecture.md), or [agent handoff](handoff.md).

## Working increment

The new Rust core builds both natively and as WebAssembly. A shared browser/Tauri viewer opens mathematical reference documents, evaluates orbitals and explicit density matrices, samples affine fields, extracts signed meshes, and persists the science plus geometry/materials in a self-contained native preview file. XYZ, PDB and single-channel standard cube workflows are available. XYZ/PDB imports automatically infer bonds from atom coordinates; PDB explicit connections are retained and supplemented.

Generated geometry can be copied/reopened without recomputation. Scientific hashes exclude appearance and link each saved surface to its basis/coefficients/matrix or authoritative grid. A changed source is rejected rather than silently associated with an old mesh. The current model does not yet provide full revision history.

The visible modes are mesh rendering, sampled raycasting preview, and volume preview. Their limitations are described in the [README](../README.md). Native and browser interfaces currently both run numerical work in the same Rust/WASM worker; native threaded field jobs are not implemented. Native saving uses the Rust format library directly.

## Verified evidence

- Rust/f64 tests cover normalized Gaussian values and derivatives, polynomial gradients at nodes, fractional occupations, explicit density contraction, electron count with overlap, grid integration, skewed/reflected grid meshing, affine resampling, malformed inputs, native roundtrips, saved-source mismatch, checksums, unknown profiles, and unsafe ZIP paths.
- Independent PySCF 2.14.0 fixtures now cover water RHF/cc-pVDZ, OH UHF/STO-3G, and general Cartesian/spherical S/P/D/F/G contractions. Native AO/field values and gradients, SCF orbital normalization, total/alpha/beta/spin populations, affine grids, and finite-difference convergence pass. The largest observed native value/gradient discrepancy on these samples was `1.1369e-13` in atomic units. [Fixture provenance, conventions, and remaining limits](../fixtures/pyscf/README.md).
- Browser tests pass in Chromium and Playwright WebKit on this Apple Silicon Mac. They exercise a nonblank scene, camera movement verified through changed canvas pixels, sampled raycast and volume pixels, density generation, save/download/reopen without a field cache, invalid-import recovery, cancellation and worker restart, material persistence, mobile layout, and independent analytic/PySCF values and gradients through WASM. The PySCF tests compare 3,868 field-value/gradient components per browser engine using the documented f64 tolerance.
- Native save regression tests verify complete replacement and preservation of the original on invalid input/write failure. Real closed/open-shell documents retain signed orbital meshes, total-density meshes, visibility/materials, and quantum arrays through native format roundtrips.
- XYZ/PDB regression tests verify automatic connectivity, exact k-d-tree results against exhaustive distance checks, explicit PDB connections, element alignment, coherent alternate/model selection, legacy repository files, malformed inputs, and dense-input limits. Browser tests cover actual XYZ/PDB loading, both bond representations, save/reopen, and mobile canvas sizing, foreground pixels, unclipped framing, and panel separation. [Algorithm decision, import profile, and measured workloads](structure-imports.md).
- Screenshots at 1440 x 960 and 390 x 844 were inspected. The scene is visible and framed; controls fit the mobile layout without horizontal overflow.
- TypeScript/production web build and the Rust/WASM release build pass. Rust formatting and Clippy with warnings denied pass for the scientific crates and desktop shell.
- A debug `Molekel Preview.app`, not Developer ID-signed or notarized, was built on macOS 27.0 / Apple Silicon. The actual packaged `tauri://localhost` viewport was observed rendering mesh/volume content, and the native Save dialog opened. The final native-dialog save/reopen workflow was not completed during the interactive check because the user was using the window; filesystem-level save regression tests are separate evidence. Native window-close protection and viewport PNG-download handling also remain to be qualified independently of browser behavior.

The current suite passes 39 Rust tests and 30 browser test cases (15 scenarios in each of two engines). Production web/WASM and the macOS debug bundle were rebuilt with PDB import and automatic XYZ/PDB bonds. At the user's request, the rebuilt native app was subsequently launched; the user confirmed that loading a PDB file works. This manual confirmation is separate from the automated browser coverage and does not complete the native save/reopen checks above. These checks do not establish the full M0 scientific/graphics acceptance matrix or a supported OS range.

The independent browser roundtrip tests exposed JavaScript's folding of negative zero to positive zero. Scientific hashes now explicitly canonicalize zero signs and sort keys, so a saved surface does not acquire a false source mismatch after a browser material edit. A native-to-WASM regression covers a supplied transition matrix with negative-zero entries. Native binary arrays preserve f64 bits; the JSON bridge's zero-sign limitation is documented in the [preview format](preview-format.md).

## Tester packaging

`npm --prefix next/app run package:macos` now produces an optimized Apple Silicon
ZIP with ad-hoc bundle signing, instructions, synthetic PDB/XYZ samples, the
exact source snapshot, dependency sources/notices, build identity, and a SHA-256
companion file. Seven packaging regressions and the 39 Rust tests pass. A full
packaging run verified the extracted app's signature, every archived file's
contents/permissions, system-only dynamic library links, and both sample imports.
Rust formatting and workspace Clippy also pass. All temporary staging remains
in repository-local `tmp/`; output remains ignored under `next/artifacts/`.
The package does not upload, notarize, or require Apple credentials.

The deployment target is macOS 13, but only the recorded development host has
been exercised; the full OS range and clean-machine Gatekeeper installation are
not qualified. Tester distribution is not a supported public release. The
existing GPL-2.0-or-later terms are now explicit for the new source. License
inventory and bundled source are not a legal compatibility opinion. See
[tester packaging](tester-packaging.md) for the repeatable command and boundaries.

Reproducible native example files are generated under ignored `artifacts/references/`: water and OH contain both signs of a HOMO surface plus a hidden total-density surface; a spherical S-G example contains a synthetic transition-density surface. The browser workflow test opens the natively generated water file and restores all three surfaces without recomputation. The water screenshot was also inspected.

## Plan alignment

| Milestone | Status | Remaining gate |
| --- | --- | --- |
| M0: Difficult-path experiments | In progress | Broader scientific qualification, topology-certified meshing, analytic shader evaluation, rigorous mixed-scene transparency, measured limits, and Linux execution |
| M1: Scientific model/persistence | Preview subset implemented | Frozen schema, explicit feature registry, chunk streaming, multiple geometry revisions, extension preservation, formal canonical hashing |
| M2: Native viewer | Preview implemented | Full metadata controls, complete radius policy, per-surface material editing, undo, camera persistence, lifecycle coverage |
| M3: Fields and saved meshes | Preview implemented | Optimized/block evaluator, native jobs, topology-correct extraction, clipping diagnostics, larger workload validation |
| M4: Rendering modes | Experimental sampled modes | Direct analytic raycasting, robust root isolation, shrinkwrap, transfer-function editor, depth-aware compositing |
| M5: External formats | XYZ, PDB structure profile, standard cube subset | Interactive PDB model selection/full metadata retention, Molden, OBJ, atom-color files, broader producer fixtures and loss reports |
| M6: Release qualification | Not complete; preview user/developer guides and tester notices/source packaging available | Supported-device matrix, clean-install tests, native lifecycle checks, signed/notarized release, completed license audit, v1-complete documentation |
| M7: Extensions | Not started | Linux/Windows release qualification, ORCA/Gaussian converters, justified acceleration or remote jobs |

## Decisions after the research plan

The user's clean-Rust requirement supersedes the planned C-library integrations for this increment. `mcubes` 0.1.7 is an existing MIT Rust implementation used to test the pipeline, not a claim of MC33 topology guarantees. Gaussian evaluation is a new, transparent scalar Rust reference implementation. No hand-written replacement meshing engine, C++ implementation, or legacy source reuse was introduced.

The render coordinates currently remain in bohr throughout the scene; angstroms are shown for picked atom positions. This avoids an extra scene conversion during the initial tests and is explicit in the interface. The research proposal's renderer-in-angstrom convention is not yet adopted.

Active GitHub CI was not installed outside `next/`; reproducible platform commands are in [ci/README.md](../ci/README.md). Linux and Windows remain unverified, not supported release targets. No public release or upload was performed.

## Documentation handoff

Root/scoped `AGENTS.md` now lead to the current handoff, as-built architecture,
fresh-checkout setup, build/test instructions, packaging, and a standalone user
guide. The documentation map links the complete original research and M0-M7
plan while explicitly distinguishing proposals from implemented behavior.
The current docs cover persistence, worker cancellation/cache semantics, import
losses, limitations, and outstanding native/release verification. The fresh-run
instructions create repository `tmp/` before resolving it.

Future tester ZIPs include `User-guide.md`; source snapshots also include both
agent guides, the original brief, historical plans, and current developer docs.
The prior ZIPs remain unchanged; use a newly generated package for these additions.

The documentation pass checked 124 local Markdown links, cross-checked controls
and commands against source, exercised the documented production-preview server
and its linked assets, and passed the seven packaging tests. Application UI and
scientific behavior were not changed; the browser suite was not rerun for this
documentation pass. Historical runtime evidence above remains explicitly dated.

## Next priorities

1. Resolve the topology-aware Rust mesher choice against MC33/Lewiner reference fixtures; classic marching cubes must not become an accidental final decision.
2. Implement robust sampled ray intersections and direct analytic orbital/density shaders, then quantify errors and workload limits.
3. Extend the independent suite with spatial-integral convergence, broader exponent ranges, and producer-specific conventions before importing real wavefunction files. The first closed/open-shell and S-G reference gates now have passing evidence.
4. Finish native save/open interaction checks and document lifecycle/state preservation, then expand the interface and remaining importers.

The application is a usable development preview, not completion of the full plan. No scientific accuracy or performance claim beyond the named tests is implied.
