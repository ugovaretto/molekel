# Implementation status

Date: 4 October 2026. Branch: `2026`. All new implementation is under `next/`; legacy source, data, and historical research remain untouched. Root `AGENTS.md` is the user-requested development entry point. The previously requested root ignore rule excludes repository-local `tmp/`.

Start with the [documentation map](README.md), [user guide](user-guide.md),
[as-built architecture](architecture.md), or [agent handoff](handoff.md).

## Version 0.4.0

The user requested **EigenVista 0.4.0**, committing the rename below, an annotated
`v0.4.0` tag, and a push of the current `2026` branch to `origin`. The Rust
workspace and six first-party lock entries, npm package/lockfile, Tauri metadata,
and current user/architecture/handoff descriptions use 0.4.0. External
dependencies, icon/logo assets, scientific reference data, and the native
schema shape `[0, 1]` are unchanged. New documents use EigenVista identifiers;
the legacy Molekel reader compatibility remains as documented below.

The 0.4.0 release preflight passes **108 workspace Rust tests**, **114
Chromium/WebKit browser cases**, and **9 packaging tests** after the version
change. Production WASM/TypeScript/frontend build, Rust/frontend formatting,
workspace/all-target Clippy with warnings denied, structured version/dependency
checks, all 202 local documentation targets, and `git diff --check` pass.
Desktop/mobile branding screenshots were inspected; the converter reports
`eigenvista-convert 0.4.0`. The existing frontend chunk-size warning remains.
The running preview and private chat exports are preserved. Native interactive
and clean-machine installation acceptance remain separate from these checks;
tester archives are ad-hoc signed, not notarized public releases.

The repository remains at `ugovaretto/molekel`; no organization migration is
part of this release. Git records the completed commit/tag/push, and each
delivered tester ZIP's `BUILD-INFO.json` records its exact source and checks.

## EigenVista rename

The user requested renaming the active rewrite to **EigenVista**, keeping its
icon and logo. That initial rename retained version **0.3.2**; the repository,
`next/` source location, and current branch remained unchanged. Active packages, commands,
application metadata, generated WASM names, and documentation adopt EigenVista.
The native bundle is `EigenVista.app`; the CLI is `eigenvista-convert`; new
tester ZIP names start with `EigenVista-` and contain
`Source/eigenvista-source.tar.gz`.

The canonical native suffix is `.eigenvista`. Existing `.molekel` preview files
remain readable using the previous matched format/profile identifiers, with
unchanged `[0, 1]` document shape, scientific data, source hashes, and cached
meshes. Both native profiles report `ImportReport.format: "eigenvista"`.
Encoding always writes the new `eigenvista` format and
`eigenvista-preview-polynomial-v1` profile, even when native Save is explicitly
given the supported legacy `.molekel` suffix. New saves are not guaranteed
readable in older Molekel builds. No bulk file migration is performed.
See [native compatibility](preview-format.md).

Verification passes **108 workspace Rust tests** (including native destination
validation), **114 Chromium/WebKit browser cases**, and **9 packaging tests**.
Six new Rust regressions cover exact old/new native profiles, mixed-pair
rejection, scientific arrays/source hashes/cached meshes, import reporting,
bond repair, and CLI naming/overwrite protection. Four new browser cases cover
branding, unchanged atom logo, desktop/390/320-pixel header layout, renamed
downloads, and old-profile Open/save/reopen with full document equality.
Desktop/mobile screenshots in both engines were inspected; the full existing
canvas-pixel, camera, quantum import, rendering, high-resolution, and cancellation
suite also passes. An actual pre-rename water reference file additionally passes
the renamed CLI's validation.

Production WASM/TypeScript/frontend build, Rust/frontend formatting,
workspace/all-target Clippy with warnings denied, local documentation targets,
and `git diff --check` pass. The existing frontend chunk-size warning remains.
Structured comparisons confirm all eight icon files are byte-identical and all
external Rust/npm lock entries are unchanged. The numerical core is unchanged
apart from crate-name substitutions; frozen scientific data is not regenerated.

The packager checks the EigenVista bundle identity, CLI/default filenames,
source snapshots with uncommitted crate renames, extracted bytes and permissions,
ad-hoc signatures, and native/WASM example agreement. Exact source identity
and completed archive checks are recorded in each delivered `BUILD-INFO.json`.
The tester is not notarized; native interactive and clean-machine installation
acceptance are not implied. Earlier counts and release evidence below belong
to the pre-rename Molekel implementation and remain historical evidence.
The initial rename did not request a version bump, commit, tag, or push. Historical
Molekel references, Git tags, repository URLs, original research filenames,
third-party notices, and private chat exports are not renamed.

## Version 0.3.2

The user confirmed the zoom fix in the web preview, then requested version
**0.3.2**, a commit, an annotated `v0.3.2` tag, a push of the current `2026`
branch to `origin`, and a fresh redistributable Apple Silicon tester ZIP.
This release includes the camera redraw fix, Zoom in/out buttons, camera
regressions, and updated documentation described below. The Rust workspace,
six first-party lock entries, frontend package/lockfile, Tauri metadata, and
current version descriptions use 0.3.2. External dependencies, scientific
references, and native schema `[0, 1]` are unchanged.

After the version bump, release preflight passes **102 workspace Rust tests**,
**110 Chromium/WebKit browser cases**, and **7 packaging tests**. Production
WASM/TypeScript/frontend build, Rust/frontend formatting, workspace/all-target
Clippy with warnings denied, all 195 local documentation targets, and
`git diff --check` pass. Camera screenshots were inspected on desktop and mobile.
Structured manifest checks confirm consistent versions and unchanged external
dependencies. The existing frontend chunk-size warning remains.

The existing preview and private chat export are preserved. Chat exports remain
ignored and excluded from source snapshots and commits. Inspect Git for actual
commit/tag/push state and the delivered ZIP's `BUILD-INFO.json` for its source
identity and completed packaging checks. Tester ZIPs remain ad-hoc signed, not
notarized public releases; native interactive workflows and clean-machine
installation remain separate acceptance gates.

## Camera zoom redraw

The user reported that wheel zoom in the web preview only appeared after a
later click. This was reproduced before the fix: after a wheel event and eight
animation frames, canvas pixels remained unchanged. OrbitControls updates the
camera synchronously inside its wheel handler, so the next animation-loop
`update()` can return false even though a new camera view has not been drawn.

The viewport now listens for OrbitControls change events and marks the next
frame for rendering, retaining damping and avoiding unconditional idle redraws.
The listener is removed on cleanup. Zoom in/out toolbar buttons call the
library's public dolly methods; middle-button vertical drag and touchscreen
pinch remain available. Camera-only actions do not modify saved science or
regenerate geometry. The user guide describes all zoom alternatives.

Verification passes **102 workspace Rust tests** and the full **110
Chromium/WebKit browser cases** (55 scenarios per engine). The eight new camera
cases count graphics draw calls, wait for idle frames, and verify wheel-only
pixel changes before any canvas click in both directions and all rendering
modes. They also cover Enter/Space button activation, middle-drag, and toolbar
fit/activation at 390- and 320-pixel widths. Desktop/mobile screenshots were
visually inspected. Production WASM/TypeScript/frontend build, Rust/frontend
formatting, workspace/all-target Clippy, local documentation links, and
`git diff --check` pass. The existing frontend chunk-size warning is unchanged.

At the end of the camera implementation step, version remained 0.3.1 and the
previously distributed ZIP was unchanged. Native interaction and packaging had
not been rerun. The subsequent 0.3.2 release request is recorded above.

## Version 0.3.1

The user requested version **0.3.1**, committing the high-resolution work below,
an annotated `v0.3.1` tag, pushing the current `2026` branch to `origin`, and a
fresh redistributable Apple Silicon tester ZIP. The Rust workspace and its six
first-party lock entries, frontend package/lockfile, Tauri metadata, and current
version descriptions use 0.3.1. External dependencies, frozen scientific
references, and native schema `[0, 1]` are unchanged.

The user reports calculations completing within seconds at higher resolutions;
exact dimensions and timings were not supplied. This is encouraging local
feedback, not a reproducible benchmark or a platform-wide performance promise.
Personal `next/chat-*.md` exports are now ignored so packaging cannot include
them as untracked source. The existing chat export and running preview are
preserved. Inspect Git for commit/tag/push state and each delivered ZIP's
`BUILD-INFO.json` for its source identity and completed packaging checks.

After the version bump, release preflight passes **102 workspace Rust tests**,
**102 Chromium/WebKit browser cases**, and **7 packaging tests**. Production
WASM/TypeScript/frontend build, Rust/frontend formatting, workspace/all-target
Clippy with warnings denied, local documentation links, and `git diff --check`
also pass. Desktop/mobile high-resolution screenshots were inspected. Structured
manifest checks confirm consistent versions and unchanged external dependencies;
Git source discovery excludes the private chat export. The existing large
frontend-chunk build warning remains.

The ZIP workflow builds an optimized native application and standalone converter,
uses ad-hoc signing, and includes documentation, source, examples, and dependency
notices. It is not a notarized public release. Native interactive workflows and
clean-machine installation remain separate acceptance gates.

## High-resolution grids through 256

The user requested substantially larger grids and will test real-molecule
performance themselves. Both selectors now offer 24, 32, 40, 48, 64, 80, 96,
128, 160, 192, 224, and 256 samples per axis, with default 40 unchanged. The
core accepts 12 through 256. Requests are not silently downgraded and remain
background worker jobs with cancellation and phase reports.

`Grid::validate_transient()` permits up to 256-cubed calculation samples,
separately from the unchanged 128-cubed authoritative source-grid bound.
The mesher counts actual per-cell output using all 256 case counts obtained
from the pinned library's public API. It rejects output above two million
vertices before extraction, replacing the former all-cells worst-case rejection
without changing triangulation or mesher identity. Rust-owned large buffers
use fallible reservations. Numerical/domain validation, document surface counts,
geometry budgets, and the 128 MiB native-container cap remain intact.

The WASM `SampledField` owns f64 samples, rechecks scientific identity on meshing,
and exports only small metadata plus an independent full-resolution f32 array
for display. The worker transfers that buffer instead of cloning a large grid
through JSON; the renderer uses it without another CPU-side conversion copy.
Older JSON probe APIs remain for compatibility/reference checks. Saved meshes
record the requested resolution, while authoritative scientific data and native
schema remain unchanged. No transient display array is saved as source data.

A 256-cubed field needs 128 MiB of f64 samples and another 64 MiB of display
values before geometry, temporary copies, GPU storage, and runtime overhead.
Trapped/failed workers are retired, and the next request starts fresh. Scene
construction is staged so a recoverable allocation error leaves the old scene
visible; render/image-export exceptions are reported without unmounting the UI.
Fallible allocations are not a guarantee against browser, library, GPU, or
OS-level OOM. WASM linear memory retains its high-water size until worker
termination. Existing geometry and file guards are deliberately retained.

Verification: **102 workspace Rust tests** and **102 Chromium/WebKit browser
cases** (51 scenarios in each engine), production WASM/frontend build,
formatting, and workspace/all-target Clippy pass. Native tests exercise actual
256-cubed analytic sampling, signed plane meshes, exact case-count/threshold
parity, pathological output rejection, separate source/transient limits, and
deterministic capacity-overflow handling without exhausting physical memory.
Ten new high-resolution browser cases cover a full-resolution two-AO
density, cache reuse, save/reopen, all rendering modes and mobile framing,
real 256-cubed Molden job cancellation, and injected worker/scene failure paths.
Four new direct owned-field API cases verify orbital/density/signed-affine-cube
parity against the legacy API, independent display buffers, unchanged native
sources, source-hash rejection, and reuse after a rejected request. Desktop and
mobile screenshots were visually inspected; all 169 local Markdown targets
resolve and `git diff --check` passes.

At the end of this implementation step, application version remained 0.3.0;
no commit, tag, push, native rebuild, or tester ZIP had yet been requested.
The subsequent 0.3.1 release request is recorded above. Completed high-resolution
benchmarks for the user's large Molden density are not claimed. The user's
running server and chat export were preserved.

## Version 0.3.0

The user requested version **0.3.0**, a commit of the accumulated application
changes, an annotated `v0.3.0` tag, and a push of the current `2026` branch to
`origin`. This increment includes the orbital browser and atomic multi-orbital
meshes, optimized scalar density sampling, and cancellable larger background
calculations documented below. The user also confirmed that the density in
`data/molden.input` generates effectively instantaneously at 48 cubed on their
machine; this is a local observation, not a performance guarantee.

The Rust workspace and its six first-party lock entries, frontend package and
lockfile, Tauri metadata, and current version descriptions now use 0.3.0.
External dependencies, frozen scientific reference values, and native schema
`[0, 1]` are unchanged. Earlier entries describe work before this release request;
their statements that no version bump or commit was requested are historical.
Inspect Git to establish the actual commit, tag, and remote state.

After the version update, **98 workspace Rust tests** and the complete
**88 Chromium/WebKit browser cases** pass. Production WASM/TypeScript/frontend
build, Rust/frontend formatting, workspace/all-target Clippy with warnings
denied, and all 165 local Markdown targets pass validation. Structured checks
confirm that all six crate versions and frontend/Tauri metadata agree, and
that manifest/lockfile contents changed only in first-party versions. The
existing large frontend-chunk build warning remains; no dependencies changed.
No native bundle, tester ZIP, public release upload, or notarization was requested;
native interaction and packaging tests were not rerun.
The separate user chat export is left untracked and excluded from this commit.

## Larger background calculations

The user authorized removing the estimated CPU-work restriction while preserving a
responsive interface, cancellation, and memory/mesh safeguards. Analytic
sampling no longer rejects requests based on the former 150-million estimated
work-unit cap. This applies to all analytic fields, not only `molden.input`;
the actual requested resolution is used without an automatic downgrade.
The existing UI options remain 24, 32, 40, and 48 samples per axis.

Calculations continue in the existing dedicated Rust/WASM Web Worker. The
worker sends request-associated sampling, meshing, and validation phase events
without resolving the pending result. The footer displays the current phase;
there is no percentage or remaining-time estimate. Cancel terminates the
worker and retains the prior document, meshes, and active sampled grid. A
subsequent request starts a fresh worker. Retired-worker events are ignored,
and the existing atomic publication/native-encoding preflight remains intact.

Retained guards include model validation, finite-value and diffuse-domain
checks, API sampling resolution 12 through 80, 128-cubed imported sample count,
the two-million worst-case mesher vertex bound, 128 saved surfaces, and 128 MiB
geometry/native-container budgets. The mesher rejects excessive worst-case
output before copying its sampled input. These guards bound admitted data and
individual allocations, not total browser memory or calculation latency.

Verification on 4 October 2026: **98 workspace Rust tests**, **88 browser cases**
(44 scenarios in Chromium and WebKit), production WASM/TypeScript/frontend
build, Rust/frontend formatting, and workspace/all-target Clippy pass. Actual-file
native tests generate finer density meshes at 32, 40, and 48 than at 24, compare
sampled values with the unchanged point evaluator, and preserve input science.
Additional rejection tests cover sampling resolution, imported-grid size,
dimension overflow, and the exact worst-case mesher boundary.

Browser tests verify real 48-cubed Molden density generation, sampled/reference
agreement, all three rendering modes, exact science/mesh save-reopen, and mobile
framing. A real in-flight sampling job reports its phase while page timers,
animation frames, and camera interaction continue. Cancel terminates that worker
before its result, preserves the exact saved document, and allows a fresh-worker
retry. That check uses the unmodified source file and no delayed or mocked
calculation. The six focused cases passed twice; the final run and complete
88-case suite include the visible sampling-phase assertion. Desktop/mobile
screenshots were inspected and all 165 local Markdown targets checked exist.
WASM builds finished before browser tests ran.

This work preserves the prior uncommitted orbital-browser and scalar-sampling
increments. No version bump, commit, tag, push, native bundle, or tester ZIP was
requested. Native desktop interaction and packaging tests were not rerun, and
the user's running server was not restarted.

## Earlier density sampling work-budget fix

This records the preceding optimization. Its retained CPU cap and 32-cubed
rejection were superseded by the larger-background-calculation follow-up above.

The user reported that `data/molden.input` could not generate its density even
at the UI minimum of 24 cubed. Reproduction confirmed 125 basis functions and
229,340,160 estimated work units, exceeding the existing 150-million cap.
The old sampling loop called the full value-and-gradient evaluator, discarded
the gradients, and contracted all `n*n` matrix entries at every grid point.

Scalar sampling now resolves the field once, reuses a value-only AO buffer,
and folds real density pairs as `Pij + Pji` with separate diagonal terms.
It preserves nonsymmetric and signed matrices without dropping coefficients or
substituting occupation-derived orbitals for arbitrary matrices. Nonfinite
folded coefficients use the original full matrix; alternate multiplication
order protects representable extreme products. The original point/gradient
reference evaluator, scientific inputs/hashes, native schema, dependencies,
application version, and 150-million work cap remain unchanged. Budget errors
now report the maximum admissible integer resolution.

For this file, 24 cubed now costs 122,204,160 estimated work units and produces
13,824 samples plus a 460-triangle density mesh at isovalue 0.08. Higher UI
settings still exceed the cap (the calculated integer maximum is 25); no
automatic resolution reduction or blanket large-workload support is implied.

Verification on 4 October 2026: **94 workspace Rust tests**, **84 browser cases**
(42 scenarios in Chromium and WebKit), production WASM/TypeScript/frontend
build, Rust formatting, and workspace/all-target Clippy pass. New native
tests compare sampling against the unchanged reference evaluator on four
frozen PySCF documents and nonsymmetric/antisymmetric/signed/extreme matrices.
Actual-file regressions cover 24-cubed generation, retained rejection at 32,
source hashes and native persistence. Browser checks also compare sampled
values with analytic points and exercise mesh/raycast/volume pixels, camera
movement, exact science/mesh save-reopen, and mobile framing. Desktop/mobile
screenshots were inspected, and 162 local Markdown targets checked successfully.

This follow-up preserves the uncommitted orbital-browser work. No commit, tag,
push, version bump, native bundle, or tester ZIP was requested. Native desktop
interaction and packaging tests were not rerun; the running server was not
restarted. The rebuilt WASM core is available in the existing web preview.

## Orbital browser and batch meshes

The orbital-browser increment follows `v0.2.1` without changing application
version, native schema, dependencies, import profiles, or scientific algorithms.
The [legacy Electron Density interface](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/ElectronDensity.html)
and its dialog image informed the metadata table. This increment implements
orbital listing and selection, not the legacy dialog's full bounding-box, nodal,
potential-mapping, or density controls.

The searchable table lists every imported orbital in source order, with label,
spin, occupation, energy in hartree, and saved-mesh count. Missing energy or
occupation remains unknown, not zero. Filtering preserves hidden selections;
selection does not change the scene until Generate selected. A batch accepts
up to 32 orbitals, a positive isovalue, and a bounded grid resolution. Larger
selections show an error and disable generation; nothing is silently truncated.

Jobs run sequentially through the existing Rust/WASM worker. The UI accumulates
results without changing the live document, checks aggregate geometry bounds,
and uses Rust native encoding as a persistence preflight before publishing.
Success replaces only selected fields' meshes; all other field meshes remain.
Cancel or any failure retains the prior document, active grid, and generation
settings. The Cancel control also works for native documents with no initially
active field. Only the chosen active orbital's sampled grid is retained for
volume/raycast modes; those modes do not combine multiple fields. Checkbox
selection is transient, while generated meshes persist in `.molekel` files.

Verification on 4 October 2026: **83 workspace Rust tests** and **82 browser
cases** (41 scenarios in Chromium and WebKit) pass, along with Rust formatting,
workspace/all-target Clippy with warnings denied, Prettier, and the production
WASM/TypeScript/frontend build. The 16 new browser cases cover the full
118-orbital legacy Molden list, metadata/nulls, filtering and the 32-orbital
limit, mobile horizontal scrolling, both mesh signs, exact save/reopen,
preserved density surfaces, active-grid pixel parity, camera movement, and
atomic recovery from cancellation, worker failure, invalid source hashes, and
native-format budget rejection. Desktop/mobile table, mesh, and viewport
screenshots were inspected. All 159 relative Markdown targets checked in the
rewrite README and current guides exist.

The initial full browser pass exposed an immediate-pixel-read timing race in
the existing WebKit mobile test. It now polls for the same nonblank-pixel
threshold, allowing the renderer's next animation frame; rendering code and
the threshold are unchanged. Five focused repetitions and the complete
82-case rerun pass. No WASM build ran concurrently with browser tests.

No native shell or packaging behavior changed. No tester ZIP, version bump,
commit, tag, or push was requested for this increment. Full native-dialog and
clean-machine release gates remain open. The native bundle and packaging tests
were not rerun for this UI increment, and the native app was not restarted.

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
