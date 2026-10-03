# Development handoff

Recorded 3 October 2026. Start with [root AGENTS.md](../../AGENTS.md), then this
document, [architecture](architecture.md), and [development commands](development.md).
The [documentation map](README.md) links the original brief and full M0-M7 plan.
The current application version is **0.2.1**, including cube rendering and
automatic bonds for all supported imports, alongside Molden and shared library/CLI
conversion. Native schema `[0, 1]` is unchanged. The user requested committing
this increment, tagging it `v0.2.1`, and pushing on `2026`; inspect Git for the
actual local and remote state rather than assuming completion from these notes.

## Establish the live state

The rewrite began on user-created branch `2026`. Commit `a75e8e2` and annotated
tag `molekel-next-start` mark its first working increment; both were pushed to
`origin`. They are **not** proof that subsequent packaging/documentation work
is already committed. Inspect current state before doing anything:

```sh
git status --short --branch
git log -5 --oneline --decorate
git diff --stat
git ls-files --others --exclude-standard
```

Read relevant staged/unstaged changes and new files. Preserve them. Never reset
to the tag to "restore" the project, change branches, publish, or assume the
working tree is clean. Do not repeat completed research or rebuild the legacy
C++ application. A tester source archive has no `.git`; use its source manifest
and `BUILD-INFO.json` for provenance instead.

## What is already implemented

- Shared Rust core, native format, import library/CLI, Rust/WASM bridge, React/Three.js viewer,
  and Tauri desktop shell, all under `next/`.
- Automatic exact-neighbor-search display bonds for every supported import:
  XYZ, PDB, Molden, cube, and native files. Native Open supplements only missing
  edges, preserves explicit edges/science/cached meshes, and reports an unsaved
  change when it adds bonds. Raw format decode remains exact.
- [Single-field cube import](cube-import.md), including standard negative-atom-
  count single-orbital cubes, affine original grids, immediate volume/raycast
  previews, and explicit signed-mesh generation. Full f64 source grids persist
  independently of bounded display samples.
- Molden Open and Convert Files workflows, canonical real Gaussian shells
  through G, explicit producer/normalization/loss reports, and a separately
  qualified ORCA S/P/5D Molden export subset. See [import contracts](molden-import.md).
- Real Gaussian-polynomial orbitals/density matrices through G, analytic
  gradients, sampled grids, positive/negative classic marching-cubes meshes.
- Native preview files containing scientific inputs and durable surface meshes
  with generation/material metadata; browser download and atomic native save.
- Ball-and-stick/liquorice/space-filling, picking, viewport image download,
  surface controls, experimental sampled raycast/volume modes, cancellation.
- Independent frozen PySCF references, native/WASM tests, browser pixel/layout
  checks, and automated ad-hoc-signed Apple Silicon tester ZIP packaging.
- User, architecture, setup/build, packaging, format, import, and continuation
  documentation. Documentation is not a substitute for remaining release tests.

## Evidence, not assumptions

The cube/all-format-bonds increment passed **83 workspace Rust tests**, including
the native destination-guard test, and **66 browser cases** (33 scenarios in
Chromium and WebKit). Packaging has **7 passing tests**. Production WASM/frontend,
macOS debug bundle, Rust formatting, and workspace Clippy were checked. The
verified release ZIP workflow checks extracted bytes, permissions, app/CLI
signatures, architecture, linked libraries, sample imports, and the actual
standalone converter's scientific agreement with WASM, including cube grids and
signed mesh generation. Each successful package's
`BUILD-INFO.json` records its artifact-specific checks.
See [status](status.md) for scope and provenance; rerun relevant tests for new
changes rather than treating these counts as permanent acceptance.

The user confirmed native PDB loading. Native Save dialog opening was observed;
complete interactive save/reopen, native close-warning behavior, and native PNG
download handling remain incompletely qualified. Filesystem roundtrip tests and
browser downloads are separate evidence. Clean-machine installation, the entire
macOS target range, Intel, Linux, and Windows are not verified releases.
New native Molden/batch/cube workflows have not been exercised interactively; browser
tests and the native destination-guard unit test are separate evidence. The
existing running app was not restarted during this change.

## Plan and next work

The [original implementation plan](../../doc/rewrite/09-implementation-plan.md)
remains the full target. [Status](status.md) is the authoritative progress table:
M0 is still open, M1-M5 are partial preview implementations, M6 release qualification
is incomplete, and M7 is not started. Later user decisions require a clean Rust
implementation and durable self-contained meshes. Optional external mesh links
or OBJ-plus-metadata export do not replace `.molekel` persistence.

Use the user's next request to choose a bounded item; otherwise these are the
documented priorities, not permission for an unrelated rewrite:

| Work item | Entry points | Required evidence before calling it done |
| --- | --- | --- |
| Topology-aware Rust meshing | `compute.rs`, dependency record, scientific reference tests | MC33/Lewiner ambiguity/topology fixtures, signed/affine behavior, persisted algorithm identity, native/WASM parity |
| Robust sampled intersections and analytic shaders | `volume.ts`, field evaluator, browser tests | Tangency/multiple-root cases, quantified field/intersection errors, orbital and density envelopes, mixed-scene depth/transparency tests |
| Broader scientific qualification | Core reference tests, PySCF fixtures/generator | Spatial-integral convergence, diffuse/tight exponents, independent producer conventions; do not use implementation-under-test as sole oracle |
| Native lifecycle and save workflow | `files.ts`, Tauri entry point, `App.tsx` | Actual dialog save/reopen/cancel/replace/close and data-preservation observations, not only unit tests |
| Remaining external formats and richer UI | Shared import/model/format boundaries, UI | Broader Molden producer fixtures, future formats only when selected by user; metadata/material editing, undo, camera persistence without stale source associations |
| Public macOS release | Packaging, licensing, platform qualification | Supported-device/OS matrix, clean install, independent license review, Developer ID signing/notarization and distribution testing |

Shrinkwrap, full transfer functions, schema freezing/migrations, larger workloads,
native parallel jobs, and future user-selected converters also remain open.
Direct ORCA is deferred in favor of its existing Molden export tool; no direct
Gaussian-log/T41 reader was added; Gaussian cube is a separate supported scalar
profile. PDB is not a full metadata-preserving importer. Do not quietly downgrade
these requirements because the current preview can display a scene.

## Important traps

- Both desktop and browser scientific jobs currently run in WASM, not native
  threads. The worker has one transient grid cache, distinct from saved meshes.
- `mcubes` is classic marching cubes, not MC33. The scalar Rust evaluator is a
  reference implementation, not the proposed optimized scientific kernel.
- The as-built scene is in bohr, despite the early proposal's angstrom renderer.
- JSON transport folds negative zero; source hashes explicitly canonicalize it.
  Native binary arrays retain f64 bits. Preserve cross-runtime hash tests.
- Structure-only imports cannot enable quantum generation. Imported grids are
  authoritative scalar data, not recoverable basis/orbital information.
- `sample` and `generate` share one worker cache. Preparing a cube preview does
  not create meshes or replace original grid samples. A preview f32 overflow
  error must not prevent opening/saving a valid f64 document.
- `ImportReport.requires_save` drives the UI dirty state, including native bond
  backfill. Do not infer clean state solely from the `.molekel` extension.
- Generation replaces meshes for the selected field, not the whole document.
  Reopening saved meshes must not require regeneration or a source attachment.
- Tests use four unchanged legacy PDB fixtures, `data/molden.input`, and three
  cube fixtures outside `next/`. Their exact paths are listed by the packager
  and [cube guide](cube-import.md); keep them in supplied source archives.
  Do not copy legacy implementation into the rewrite.
- Preserve ORCA producer markers. Its qualified Molden profile differs from
  canonical normalization; F/G and other unqualified variants must not silently
  fall through to the canonical reader. Density labels must identify listed-
  occupation reconstruction, not imply recovered correlated matrices.
- The app calls the shared importer in WASM, not the CLI subprocess. Native
  conversion results use protected source paths; the CLI independently protects
  inputs and output aliases. Preserve no-clobber behavior even with `--force`.
- `npm test` checks default scientific workspace members, not the Tauri shell.
  Playwright may reuse a server on 5178; verify it belongs to this checkout.
  Do not regenerate WASM during browser tests: Vite can reload an active test
  back to the startup example. Finish frontend/native builds before Playwright.
- New tester packages snapshot current nonignored source, including uncommitted
  work. Do not edit source during packaging; its stability check will reject it.
- Tester signing deliberately clears Apple/Tauri credentials. Public signing
  needs a separate approved workflow; do not weaken the tester script's boundary.
- Current license is GPL-2.0-or-later, matching the repository's existing terms.
  Automated notices/source collection is not a completed legal compatibility audit.

## Finish each increment

Use repo-local temporary storage and the verification matrix in
[development](development.md). Update user-facing behavior docs, relevant
contracts, and dated evidence in status. Name unrun or blocked checks explicitly.
Preserve the user's running app and existing work. A normal handoff should state
what changed, what was verified, and which relevant risks remain; commit/push/tag
only on a direct user request.
