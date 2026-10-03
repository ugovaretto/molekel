# Development handoff

Recorded 3 October 2026. Start with [root AGENTS.md](../../AGENTS.md), then this
document, [architecture](architecture.md), and [development commands](development.md).
The [documentation map](README.md) links the original brief and full M0-M7 plan.

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

- Shared Rust core, Rust native format, Rust/WASM bridge, React/Three.js viewer,
  and Tauri desktop shell, all under `next/`.
- XYZ/PDB structures with automatic exact-neighbor-search display bonds, plus
  a bounded single-channel cube importer.
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

The last recorded full application suite passed **39 Rust tests** and **30
browser cases** (15 scenarios in Chromium and WebKit). Packaging has **7 tests**.
Production WASM/frontend, macOS debug/release bundles, Rust formatting, and
workspace Clippy were checked. The verified ZIP workflow checks extracted bytes,
permissions, signature, architecture, linked libraries, and sample imports.
See [status](status.md) for scope and provenance; rerun relevant tests for new
changes rather than treating these counts as permanent acceptance.

The user confirmed native PDB loading. Native Save dialog opening was observed;
complete interactive save/reopen, native close-warning behavior, and native PNG
download handling remain incompletely qualified. Filesystem roundtrip tests and
browser downloads are separate evidence. Clean-machine installation, the entire
macOS target range, Intel, Linux, and Windows are not verified releases.

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
| Remaining external formats and richer UI | Import/model/format boundaries, UI | Molden/OBJ/color inputs with explicit loss reports and fixtures; metadata/material editing, undo, camera persistence without stale source associations |
| Public macOS release | Packaging, licensing, platform qualification | Supported-device/OS matrix, clean install, independent license review, Developer ID signing/notarization and distribution testing |

Shrinkwrap, full transfer functions, schema freezing/migrations, larger workloads,
native parallel jobs, and future ORCA/Gaussian converters also remain in the
plan. PDB is not a full metadata-preserving importer. Do not quietly downgrade
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
- Generation replaces meshes for the selected field, not the whole document.
  Reopening saved meshes must not require regeneration or a source attachment.
- Tests use four unchanged legacy PDB fixtures outside `next/`. Keep them in
  supplied source archives. Do not copy legacy implementation into the rewrite.
- `npm test` checks default scientific workspace members, not the Tauri shell.
  Playwright may reuse a server on 5178; verify it belongs to this checkout.
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
