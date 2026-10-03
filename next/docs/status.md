# Implementation status

Date: 3 October 2026. Branch: `2026`. All new implementation is under `next/`; legacy source, data, and historical research remain untouched. Root `AGENTS.md` is the user-requested development entry point. The previously requested root ignore rule excludes repository-local `tmp/`.

Start with the [documentation map](README.md), [user guide](user-guide.md),
[as-built architecture](architecture.md), or [agent handoff](handoff.md).

## Version 0.2.1

The cube-rendering/all-format-bonds increment advances the application to
**0.2.1**. Rust workspace crates and their lock entries, frontend package and
lockfile, Tauri bundle metadata, and current guides share this version. External
dependency versions and native schema `[0, 1]` are unchanged. The user confirmed
that the cube examples render correctly and requested committing all changes,
creating a version tag, and pushing the current `2026` branch. The chosen
annotated tag is `v0.2.1`; verify its actual commit and remote state with Git.

After the version bump, version/dependency-consistency checks, all 83 workspace
Rust tests, Rust formatting, workspace/all-target Clippy with warnings denied,
and the production WASM/frontend build passed. The 66 browser cases and full
verified tester-package run below cover the same cube implementation before the
version-only follow-up; they were not rerun solely for metadata changes.

The `elf_pic.cube` investigation confirmed a display-level issue rather than
lost scalar data: almost every value is above the default isovalue `0.08`.
Increasing the level reveals the field. The user asked to stop that investigation;
no proposed percentile-based default selection or ELF-specific behavior was
applied. The prior low-amplitude default remains unchanged.

## Version 0.2.0

The Molden/shared-conversion increment advances the application from 0.1.0 to
0.2.0. Rust workspace crates, CLI, frontend package/lockfile, and Tauri bundle
metadata share this version. The desktop version command derives it from Cargo
instead of a separate literal. The native file schema remains `[0, 1]` with the
same required preview profile; an application version bump does not invalidate
existing `.molekel` documents. Dependency versions and frozen references are
unchanged. The user requested committing and pushing this increment on `2026`;
inspect Git for its actual commit and remote status.
After the bump, version-consistency checks, all 73 workspace Rust tests, Rust
formatting, workspace/all-target Clippy, and the production WASM/frontend build
passed. External Cargo/npm dependency records remain unchanged. The 54 browser
cases below were verified before this version-only follow-up; they were not
rerun solely for the metadata change.

## Cube rendering and all-format bonds

The follow-up to commit `d27aae7` makes cube scalar fields immediately available
in volume and sampled raycast modes on Open, without first extracting meshes.
Generate surfaces creates the signed, persistable geometry explicitly. The
complete original affine f64 grid remains authoritative through preview
resolution changes, conversion, and save/reopen. Standard single-orbital cubes
with negative atom counts and one dataset ID are now supported, alongside
single-channel density cubes. See [the cube profile](cube-import.md).

Every supported format now computes display connectivity: XYZ, PDB, Molden,
cube, and native `.molekel`. Native import fills only missing bonds, preserves
explicit edges/order, scientific records, and cached meshes, and reports the
change through `requires_save`. Raw native format decoding remains exact.
Older native documents are not silently rewritten. Reflected-grid volume box
winding is corrected; grid values overflowing f32 fail preview preparation
without discarding the imported f64 document or cached geometry.

Verification on 3 October 2026: **83 workspace Rust tests** (including the one
desktop test), **66 Chromium/WebKit browser cases**, and **7 packaging tests**
pass. Rust formatting, workspace/all-target Clippy with warnings denied,
TypeScript/production WASM/frontend build, and the macOS debug bundle pass.
The browser cases include both field signs, desktop/mobile pixels and camera
interaction, inside/reflected grids, source-grid preservation, saved meshes,
native missing-bond backfill, conversion, invalid-input recovery, and f32
preview failure with successful f64 persistence. Desktop/mobile screenshots
were inspected. One initial full-suite run was interrupted by a concurrent
WASM rebuild triggering Vite reload; the complete rerun after builds finished
passed all 66 cases. Build and browser checks must run sequentially.

Core tests also read the user's existing water density and two single-orbital
cube fixtures, verifying atoms/bonds, grids, and nonempty signed meshes where
appropriate. The new first-party signed/skewed scalar fixture independently
specifies every expected sample. No legacy data or code was changed.

Tester packaging now includes `Examples/signed-affine.cube`, `Cube-import.md`,
and the three legacy cube fixtures needed by the source tests. Its staged and
extracted checks exercise cube bonds, signed meshes, and the standalone
converter's exact agreement with WASM, in addition to the prior checks. Each
successful ZIP's `BUILD-INFO.json` records its artifact-specific verification;
earlier ZIPs are unchanged. Native cube dialog interaction, full native lifecycle,
clean-machine installation, and other OSs remain unqualified. The running app
was not restarted. The implementation was initially uncommitted at version
0.2.0; the later user-requested 0.2.1 version/commit/tag/push follow-up is recorded
above. No public release upload or notarization is part of this workflow.

## Working increment

The new Rust core builds both natively and as WebAssembly. A shared browser/Tauri viewer opens mathematical reference documents, evaluates orbitals and explicit density matrices, samples affine fields, extracts signed meshes, and persists the science plus geometry/materials in a self-contained native preview file. Molden, XYZ, PDB and single-channel standard cube workflows are available. Every supported import, including native documents, automatically fills missing display bonds from atom coordinates; explicit connections are retained and supplemented.

Generated geometry can be copied/reopened without recomputation. Scientific hashes exclude appearance and link each saved surface to its basis/coefficients/matrix or authoritative grid. A changed source is rejected rather than silently associated with an old mesh. The current model does not yet provide full revision history.

The visible modes are mesh rendering, sampled raycasting preview, and volume preview. Their limitations are described in the [README](../README.md). Native and browser interfaces currently both run numerical work in the same Rust/WASM worker; native threaded field jobs are not implemented. Native saving uses the Rust format library directly.

## Molden and shared conversion increment

The shared Rust `molekel-import` library owns byte detection, Molden parsing,
producer conventions, reports, and provenance. Open, the separate Convert Files
queue, and the standalone `molekel-convert` CLI use that same library. The app
links it through WASM; it does not launch the CLI or vendor executables. No
external runtime dependency, C++, or copied legacy implementation was added.

Canonical real Gaussian Molden S/P/SP/D/F/G shells, spherical/Cartesian order,
normalization, spin, occupations, energies, and occupation-derived densities
are implemented. Identified `orca_2mkl` exports have a separately qualified
S/P/spherical-D normalization profile; its unqualified F/G, Cartesian D, and SP
variants fail explicitly. Direct ORCA/GBW, Gaussian logs, and T41 readers are
deferred. Other formats will be selected later by the user. The exact profile,
limits, losses, CLI behavior, and API are in [Molden import](molden-import.md).

Open marks foreign imports unsaved, reports warnings, and preserves the prior
document on failure. Convert Files retains up to 32 queue entries and 128 MiB
of encoded results, does not replace the scene, and saves only on explicit
request. CLI destination preflight and no-clobber publication protect inputs
and previous outputs, including macOS Unicode filename aliases under `--force`.
Native Save enforces `.molekel` and rejects protected imported source paths.
Browser downloads cannot attest that the user's download ultimately reached disk.

Five [independent actual-import fixtures](../fixtures/molden/README.md) cover
PySCF RHF water, UHF OH, multi-primitive spherical/Cartesian S-G contractions,
and a real upstream ORCA NH3 export. Native and WASM tests compare AO/MO/density
values and analytic gradients, normalization/overlap populations, and native
scientific/mesh persistence. Largest observed canonical sample discrepancy was
`3.13e-12`; the independently corrected ORCA fixture gives `6.64e-9`, within
the stated relative tolerance for its finite-precision text. These are tested
profiles, not a blanket producer or scientific-accuracy guarantee.

The untouched legacy `data/molden.input` also imports successfully: 17 atoms,
125 AOs, 118 spatial orbitals, and one occupation-derived density. Its frequencies,
normal modes, convergence history, and geometry trajectory are explicitly
reported as omitted. This smoke test is separate from independent numerical
qualification. A converted convenience copy is generated under ignored
`next/artifacts/conversions/molden.molekel`, not beside the legacy input.

Verification for the earlier Molden increment: 72 default-workspace Rust tests, one separate native
destination-guard test, and seven packaging tests pass. Workspace/all-target
Clippy with warnings denied, Rust formatting, the production WASM/frontend build,
and the macOS debug bundle pass. All 54 browser cases pass (27 scenarios in
Chromium and WebKit), including workflow, numerical, pixel, camera, responsive,
and delayed-read success/failure/unsaved-edit regressions. The latter check that
an earlier Open cannot overwrite a later document or bypass newly made edits.
Desktop/mobile Molden and conversion screenshots were inspected. Native file
dialogs and a clean-machine installation were not exercised for this increment.

The tester packager now includes a separately signed `Tools/molekel-convert`,
water Molden example, updated guides, and source/test-data notices. It verifies
the actual extracted converter's native roundtrip against the app's WASM import,
in addition to signature/architecture/library and archive-content checks. Each
successful package's `BUILD-INFO.json` records those artifact-specific checks;
earlier ZIPs remain unchanged. Optional IOData reference-generation/test-data
licensing is isolated and documented in the [dependency record](dependencies.md).
The current documentation pass checked local Markdown links with no missing
targets. Legacy source/data and original plans are unchanged. At the end of
implementation, these changes were uncommitted and the tester ZIP remained local;
the later version/commit/push request is recorded above. No release upload or
additional tag is part of that request.

## Earlier preview evidence

- Rust/f64 tests cover normalized Gaussian values and derivatives, polynomial gradients at nodes, fractional occupations, explicit density contraction, electron count with overlap, grid integration, skewed/reflected grid meshing, affine resampling, malformed inputs, native roundtrips, saved-source mismatch, checksums, unknown profiles, and unsafe ZIP paths.
- Independent PySCF 2.14.0 fixtures now cover water RHF/cc-pVDZ, OH UHF/STO-3G, and general Cartesian/spherical S/P/D/F/G contractions. Native AO/field values and gradients, SCF orbital normalization, total/alpha/beta/spin populations, affine grids, and finite-difference convergence pass. The largest observed native value/gradient discrepancy on these samples was `1.1369e-13` in atomic units. [Fixture provenance, conventions, and remaining limits](../fixtures/pyscf/README.md).
- Browser tests pass in Chromium and Playwright WebKit on this Apple Silicon Mac. They exercise a nonblank scene, camera movement verified through changed canvas pixels, sampled raycast and volume pixels, density generation, save/download/reopen without a field cache, invalid-import recovery, cancellation and worker restart, material persistence, mobile layout, and independent analytic/PySCF values and gradients through WASM. The PySCF tests compare 3,868 field-value/gradient components per browser engine using the documented f64 tolerance.
- Native save regression tests verify complete replacement and preservation of the original on invalid input/write failure. Real closed/open-shell documents retain signed orbital meshes, total-density meshes, visibility/materials, and quantum arrays through native format roundtrips.
- XYZ/PDB regression tests verify automatic connectivity, exact k-d-tree results against exhaustive distance checks, explicit PDB connections, element alignment, coherent alternate/model selection, legacy repository files, malformed inputs, and dense-input limits. Browser tests cover actual XYZ/PDB loading, both bond representations, save/reopen, and mobile canvas sizing, foreground pixels, unclipped framing, and panel separation. [Algorithm decision, import profile, and measured workloads](structure-imports.md).
- Screenshots at 1440 x 960 and 390 x 844 were inspected. The scene is visible and framed; controls fit the mobile layout without horizontal overflow.
- TypeScript/production web build and the Rust/WASM release build pass. Rust formatting and Clippy with warnings denied pass for the scientific crates and desktop shell.
- A debug `Molekel Preview.app`, not Developer ID-signed or notarized, was built on macOS 27.0 / Apple Silicon. The actual packaged `tauri://localhost` viewport was observed rendering mesh/volume content, and the native Save dialog opened. The final native-dialog save/reopen workflow was not completed during the interactive check because the user was using the window; filesystem-level save regression tests are separate evidence. Native window-close protection and viewport PNG-download handling also remain to be qualified independently of browser behavior.

Before the Molden increment, the suite passed 39 Rust tests and 30 browser test cases (15 scenarios in each of two engines). Production web/WASM and the macOS debug bundle were rebuilt with PDB import and automatic XYZ/PDB bonds. At the user's request, the rebuilt native app was subsequently launched; the user confirmed that loading a PDB file works. This manual confirmation is separate from the automated browser coverage and does not complete the native save/reopen checks above. These checks do not establish the full M0 scientific/graphics acceptance matrix or a supported OS range.

The independent browser roundtrip tests exposed JavaScript's folding of negative zero to positive zero. Scientific hashes now explicitly canonicalize zero signs and sort keys, so a saved surface does not acquire a false source mismatch after a browser material edit. A native-to-WASM regression covers a supplied transition matrix with negative-zero entries. Native binary arrays preserve f64 bits; the JSON bridge's zero-sign limitation is documented in the [preview format](preview-format.md).

## Tester packaging

`npm --prefix next/app run package:macos` now produces an optimized Apple Silicon
ZIP with ad-hoc bundle signing, instructions, molecular samples, the
exact source snapshot, dependency sources/notices, build identity, and a SHA-256
companion file. The earlier packaging increment passed seven packaging regressions
and 39 Rust tests. Its full run verified the extracted app's signature, every archived file's
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
| M5: External formats | Shared importer/CLI/UI; Molden profile, XYZ, PDB structure profile, standard cube subset | Broader Molden producer conventions, PDB metadata/model selection, future user-selected formats including planned OBJ/color workflows |
| M6: Release qualification | Not complete; preview user/developer guides and tester notices/source packaging available | Supported-device matrix, clean-install tests, native lifecycle checks, signed/notarized release, completed license audit, v1-complete documentation |
| M7: Extensions | Not started | Linux/Windows release qualification, future user-selected converters, justified acceleration or remote jobs |

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
3. Extend independent qualification with spatial-integral convergence, broader exponent ranges, and more producer-specific Molden conventions. Closed/open-shell, canonical S-G, and one real ORCA S/P/5D import have passing evidence; unsupported producer variants must remain explicit errors.
4. Finish native save/open interaction checks and document lifecycle/state preservation, then expand the interface and remaining importers.

The application is a usable development preview, not completion of the full plan. No scientific accuracy or performance claim beyond the named tests is implied.
