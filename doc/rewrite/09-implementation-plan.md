# Molekel implementation plan

Date: 3 October 2026. Status: ready for implementation; no application code or experiments completed by this planning task.

## Goal and working agreement

Deliver an offline macOS molecular viewer with the full [v1 requirements](02-requirements.md), using the [selected architecture](03-architecture-decision.md). Keep the same scientific core and interface portable to Linux, Windows, and the browser. macOS is the first supported release, not an exclusive platform dependency.

Work directly on the existing `2026` branch, as requested. No new branch or worktree is needed. Preserve the legacy application, existing data, and the user's brief while developing the replacement alongside them. Do not modernize the legacy build as a prerequisite. This document plans future implementation; it does not begin it.

The implementation order below expands M0-M7 in the [validation and delivery plan](07-validation-and-delivery.md). That document remains the source for numerical tolerances, fixture coverage, and benchmark methodology. Record deviations and evidence in the relevant design document rather than quietly changing scope.

## Non-negotiable outcomes

- Molecular orbitals and density-matrix fields both support generated isosurfaces and shader raycasting. Sampled raycasting is the general path; direct analytic evaluation has an explicitly tested workload envelope for each field type.
- Generated meshes can be persisted in a self-contained `.molekel` document with the molecular structure, relevant quantum data, signed isovalues, generation parameters, and rendering settings. Copying that file to a clean installation must allow viewing the saved meshes without recomputation or the original input files.
- Saved meshes are durable document content when the user includes them in a save, even though they are mathematically derived data. Clearing a runtime cache must not remove them from a document. External mesh links or OBJ-plus-companion export are optional extensions, not prerequisites for portable saved surfaces.
- Marching cubes, projected shrinkwrap, direct volume rendering, and shader raycasting remain required modes. A preview with only mesh rendering is not the full v1 release.
- PDB, XYZ, Molden, cube, OBJ, and atom-color text compatibility are required before full v1. ORCA and Gaussian converters may follow independently.
- Missing quantum data remains missing. A geometry-only mesh or sampled cube cannot enable analytic evaluation by inference.

## Proposed repository layout

Place the replacement in `next/` to avoid collisions with legacy `src/`, `build/`, and `test/`. Create only the directories needed by the current milestone.

| Proposed path | Ownership |
| --- | --- |
| `next/app/` | Shared TypeScript/React interface and Three.js viewport |
| `next/app/src-tauri/` | Tauri shell, native commands, platform integration, packaging |
| `next/crates/molekel-core/` | Scientific model, units, validation, field identities, job contracts |
| `next/crates/molekel-format/` | Native container reader/writer and array integrity |
| `next/crates/molekel-compute/` | Gaussian evaluation, density contraction, meshing, C-library boundaries |
| `next/crates/molekel-wasm/` | Browser worker bindings to the shared core |
| `next/fixtures/` | Small licensed inputs, expected values, provenance and checksums |
| `next/tests/` | Cross-runtime, workflow, rendering, and portability tests |
| `next/tools/` | Reference generation and later independent conversion tools |
| `doc/rewrite/` | Decisions, format specification, validation results, release checklist |

Use a Rust workspace and a single frontend package initially. Keep renderer code independent of React updates, and keep scientific code independent of the UI and desktop shell. Pin toolchains and dependencies after the first compatibility checks. Do not create a generalized plugin system or remote service in the initial implementation.

## M0: Prove the difficult paths

Complete these bounded experiments before building the full interface. Prefer small reusable tests over throwaway prototypes, but do not treat experimental code as production-ready merely because it renders an image.

1. Establish a minimal shared web/Tauri application and automated macOS/Linux build checks. Confirm WebGL2 creation and a visible, interactive test scene in the packaged Mac application as well as a browser.
2. Compile the candidate Gaussian and MC33 C libraries behind narrow ownership-safe interfaces for native macOS, native Linux, and WASM. Verify licenses, allocation boundaries, cancellation strategy, and scalar builds before SIMD work.
3. Create independent small wavefunction fixtures: one closed-shell and one open-shell case, with explicit density matrices, orbital values, gradients, and sampled grids. Include an analytic Gaussian fixture whose answer does not depend on a chemistry package.
4. Exercise orbital and density evaluation through native and WASM paths. Check basis ordering, normalization, occupations, matrix orientation, and units against the independent references.
5. Render the difficult combined scene: atoms, both orbital signs, a density field, a translucent OBJ, and an overlapping volume. Test sampled raycasting and small analytic orbital/density cases, including tangencies and multiple intersections. Measure compositing correctness and resource use.
6. Roundtrip a tiny `.molekel` file containing authoritative quantum arrays, a grid, a persisted mesh, and material settings. Open the copied file with an empty cache and without access to its original source.

**Exit:** a short evidence report lists passing checks, failures, dependency versions, tested hardware/runtimes, and supported experimental workload limits. Resolve any scientific discrepancy before proceeding. If a C/WASM boundary or the viewport fails its gate, apply the alternatives already documented in the architecture decision and repeat that gate. Do not drop a required mode to make an experiment pass.

## M1: Stabilize scientific data and persistence

1. Finalize the initial real-Gaussian profile, canonical units, explicit angular polynomials, coefficient/matrix ordering, and immutable scientific identities. Cover general contractions and Cartesian/spherical functions through G shells.
2. Write the manifest schema and implement bounded validation of versions, required features, dimensions, array types, chunk paths, sizes, and checksums. Add corrupt/truncated/adversarial container fixtures.
3. Implement native and browser loading/saving with the same document semantics. Use atomic replacement on native saves; failed writes must not destroy the previous valid document.
4. Separate authoritative inputs, disposable runtime caches, and persisted derived assets. A mesh record carries source hashes, algorithm/version, domain/resolution, precision, signed isovalue, and material/transform associations.
5. Preserve unknown optional metadata and clearly reject unsupported required features. Validate units and scientific dependencies before enabling calculations.

**Exit:** exact canonical-array roundtrips, schema/integrity tests, and native/WASM agreement pass. A copied document displays its saved meshes without invoking field evaluation or meshing. Changing scientific inputs marks affected saved results as belonging to the earlier input revision; they are never silently relabeled as current results.

## M2: Deliver a usable native-document viewer

1. Implement open/save, a scene list, compact object properties, orbital/density selection tables, camera controls, picking, and visible loading/error states.
2. Add ball-and-stick, liquorice, and van der Waals representations with an explicit radius dataset and documented bond-inference policy.
3. Add per-object and per-sign colors, opacity, visibility, atom-color overrides, units, and transforms. Add undo for presentation/metadata edits and basic image export with recorded field parameters.
4. Display persisted meshes and structure data from native documents. Keep large arrays outside reactive UI state and dispose of GPU resources when documents close.

**Exit:** an offline Mac preview opens, inspects, edits appearance, saves, and reopens native documents without losing scientific information. Repeated open/close, resizing, HiDPI, selection, and context-loss tests pass. This preview is explicitly native-format-only, not a full compatibility release.

## M3: Compute fields and persist isosurfaces

1. Implement cancellable, memory-budgeted field jobs with progress, immutable input revisions, and stale-result rejection. Share scientific semantics between native execution and browser workers.
2. Support signed orbitals, explicit total/alpha/beta/spin density matrices, and occupation-derived densities. Keep correlated or excited-state matrices distinct from reconstructed ground-state densities.
3. Sample full affine grids with explicit domain, resolution, scalar meaning, and units. Separate field and mesh cache keys so isovalue changes reuse sampled data and material changes trigger no computation.
4. Integrate MC33 extraction for positive and negative levels, gradients/normals, clipping diagnostics, and indexed meshes.
5. Save selected meshes and their scientific dependencies in `.molekel`. Test moving the file to another directory and a clean installation with the original inputs unavailable; restore geometry and appearance without recalculation.

**Exit:** R04-R06 and R12 numerical checks pass, including density electron-count/integral checks and surface refinement tests. Rapid parameter changes and cancellation do not publish stale or partial assets. Portable saved-surface tests pass for both orbitals and explicit density matrices.

## M4: Complete the rendering modes

1. Implement projected shrinkwrap seeded from validated marching-cubes topology, with adaptive refinement, convergence reporting, and recoverable failures.
2. Implement shader isosurface raycasting over sampled fields for orbitals, density matrices, and cube data. Use robust intersection handling, shared depth with molecular geometry, and separately controlled signed materials.
3. Implement direct analytic orbital and density-matrix evaluation in shaders within independently measured budgets. Oversized requests offer the sampled route explicitly; never silently pretend it is direct analytic rendering.
4. Implement direct volume rendering with a signed transfer-function editor, threshold/isovalue band, quality controls, and step-corrected opacity.
5. Finish mixed surface/volume compositing, clipping, gradients, lighting, and interactive/still-image quality settings. Benchmark cold/warm paths and establish documented defaults and limits.

**Exit:** R13-R15 pass reference scenes for both orbital and density fields. Check multiple roots, near-tangencies, camera-inside cases, disconnected components, and intersecting translucent objects. Visual tests, numeric depth/field checks, and performance evidence are all required; screenshots alone are insufficient.

## M5: Restore external-file workflows

Implement the shared conversion/report contract before exposing each importer in the interface. Follow the [conversion strategy](06-conversion-strategy.md):

1. XYZ and PDB structure adapters, including model selection, element handling, diagnostics, and explicit versus inferred connectivity.
2. Cube, OBJ with metadata/companion handling, and atom-color text adapters. Preserve full affine grids, declared units, channel identity, and original provenance.
3. Molden real-Gaussian support through G shells, including mixed shell conventions, SP/general contractions, spin, occupations, and independent producer-specific reference checks.
4. Present completeness/loss reports and enable only operations supported by the imported data. Conversion results use the same validated model as native files.

**Exit:** R07-R10 pass the fixture matrix. Every imported complete wavefunction survives native save/reopen and produces the same fields within the declared source precision. Malformed inputs yield useful diagnostics without crashing or fabricating scientific data.

## M6: Qualify and release macOS v1

1. Resolve minimum macOS version, Apple Silicon coverage, and whether Intel support has sufficient tested evidence. Document representative supported workload sizes.
2. Run the complete numerical, parser, saved-surface, rendering, cancellation, memory, and offline workflow suites in release builds.
3. Package the application, audit dependency licenses/notices, sign and notarize with available release credentials, and test installation/open/save on a clean machine. Missing credentials block distribution certification, not local development.
4. Supply example documents, capability/error guidance, supported-format profiles, and concise user documentation. Include known limitations and benchmark conditions.
5. Release the browser build only after its own capability, worker, memory, and file-workflow checks pass. It may follow the Mac release without changing document semantics.

**Exit:** R01-R17 have recorded passing evidence and no unresolved scientific-correctness defects. R18 is labeled supported only for tested browser profiles. All requested rendering modes are present; a partial preview does not count as this milestone.

## Portability throughout development

| Target | Early checks | Release policy |
| --- | --- | --- |
| macOS | Native numerical tests, Tauri build, actual WKWebView rendering, offline workflows | First supported desktop release |
| Linux | Native core/C-library tests and Tauri compilation from M0; actual WebKitGTK/GPU smoke tests as soon as a suitable runner is available | Do not claim support from compilation alone; qualify a specific distribution/runtime before packaging a supported release |
| Windows | Keep platform-specific paths and integration behind the desktop adapter; add build checks after M1 where infrastructure allows | Separate WebView2/driver and installer qualification before support claims |
| Browser | WASM numerical tests and browser rendering checks from M0 | Separate resource budgets and capability gates; no native-runtime assumptions |

Keep macOS-only packaging and APIs in the shell. Do not make Metal, Accelerate, or Apple-specific filesystem behavior mandatory for shared scientific functions. Check case-sensitive paths, binary portability, and document copying across platforms. A Linux build cannot establish graphics correctness on every distribution or driver; record untested targets explicitly.

## M7: Follow-on work

After the first supported release, qualify Linux desktop packaging, broaden Windows testing, add ORCA/Gaussian and other producer-aware converters, and evaluate WebGPU acceleration or optional remote computation against measured needs. Portable OBJ-plus-metadata export can be added without changing the self-contained native-document requirement. These are separate deliverables, not reasons to delay the core workflow indefinitely.

## Execution and completion tracking

- Start implementation with M0, in the current branch. Keep changes bounded by one work item and leave the branch in a testable state between items.
- For each item, record implemented behavior, tests run, results, remaining limitations, and any changed decision. A build success, a rendered frame, and numerical correctness are different kinds of evidence.
- Reuse the acceptance fixtures across modes and platforms. Never use the converter/evaluator under test as its sole reference.
- Update this plan and the requirements checklist as milestones actually pass. Nothing above is marked complete by writing this document.
- Estimate later work only after M0 establishes toolchain, graphics, and scientific-kernel costs. No calendar delivery date is assumed.

The immediate next implementation step is the M0 shared application/toolchain skeleton and its smallest independent scientific fixtures. Full interface work follows the risk checks, not before them.
