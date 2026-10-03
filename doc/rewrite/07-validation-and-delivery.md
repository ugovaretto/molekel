# Molekel validation and development milestones

Scientific correctness, graphics correctness, and performance are separate release gates. This plan defines future work; none of the prototype tests or benchmarks below has been run as part of the research task.

## Reference policy

Use analytic Gaussian cases and independent PySCF-generated values as the main numerical references. Use legacy Molekel and Mol* for behavioral comparisons, not as the sole truth. A converter and the evaluator it feeds should not generate their own only reference dataset.

Each fixture records its origin, generation method/version, licensing/provenance, intended units, expected capabilities, dimensions, and source precision. Record hashes so future regressions compare against the same bytes. Generate reference grids outside the code path being tested.

## Required fixture matrix

| Area | Fixtures | Detects |
| --- | --- | --- |
| Primitive functions | Normalized s/p/d/f/g components and analytic derivatives | Normalization, phase, axis order, and gradient mistakes |
| Contractions | Multiple primitives, general contractions, diffuse/tight exponents, SP shells | Reuse and screening errors |
| Wavefunctions | Closed shell, open shell, separate alpha/beta, fractional occupations, partial MO sets | Double occupation factors and invalid density reconstruction |
| Matrices | Explicit total/alpha/beta/spin matrices, correlated density distinct from MO density, nonorthogonal overlap | Wrong matrix orientation and `trace(P)` misuse |
| Molden | Cartesian/spherical and mixed D/F/G variants, reordered basis centers, D exponents, missing/duplicate indices | The legacy reader's convention and indexing weaknesses |
| PDB | Existing `3POR.pdb`, alternate locations, atom-name/element ambiguity, models, explicit connectivity | Element inference and model merging |
| XYZ | Existing `bik.xyz`, multiple frames, malformed count, unknown elements | Basic import and diagnostics |
| Cube | Existing water and signed benzene files; synthetic asymmetric, rotated, skewed, reflected, multichannel, and truncated grids | Units, ordering, channel handling, and affine transforms |
| OBJ | Triangles, quads, concave faces, negative indices, missing normals, groups, invalid indices, nonmanifold mesh | Loader scope, metadata preservation, and robust errors |
| Native format | Structure-only, field-only, mesh-only, complete wavefunction, unknown optional/required features | Profile and version behavior |
| Container | Corrupt digest, duplicate path, excessive expansion, invalid shape product, missing/overlapping chunks | Bounded resource use and integrity |
| Surface topology | Sphere, two separated lobes, torus, saddle, thin connection, domain-clipped surface | Ambiguous cells and shrinkwrap topology assumptions |
| Raycasting | Multiple crossings in a cell, near-tangent hit, camera inside box, opaque atom inside volume | Missed surfaces and incorrect depth |
| Compositing | Intersecting translucent lobes, OBJ crossing a volume, two overlapping volumes | Sorting and integration mistakes |

Existing fixtures are seeds for the suite. New numerical references, edge cases, and expected values still need to be created during implementation.

## Numerical acceptance

Proposed initial tolerances below apply to well-conditioned, high-precision fixtures and should be tightened or relaxed only with a recorded numerical reason. They are not universal accuracy guarantees for every chemistry output.

| Check | Initial criterion |
| --- | --- |
| Native array roundtrip | Exact byte equality of canonical array values; optional caches may be absent |
| f64 orbital/density evaluation | `abs(error) <= 1e-10 + 1e-8 * abs(reference)` in atomic units on the reference suite |
| f32 rendering field | `abs(error) <= 1e-6 + 1e-4 * abs(reference)` on the declared visualization envelope |
| Gradients | Analytic derivative comparison plus central-difference convergence away from singular/ill-conditioned cases |
| Orbital normalization | `C S C^T` close to identity for orthonormal source orbitals; do not impose this on arbitrary supplied sets |
| Electron number | `trace(P S)` agrees with the represented occupation sum within 1e-6 for high-precision fixtures |
| Spatial density integral | Converges under box enlargement and grid refinement; initial target within 1e-3 electron for the small reference suite |
| Surface geometry | Residual and distance decrease under refinement; displacement error judged against grid spacing and local gradient |
| CPU/WASM/native agreement | Same f64 tolerance; exact integers/identities and consistent units |

Use absolute error around orbital nodes, where relative error becomes meaningless. Compare a whole orbital up to one global sign when the producer allows phase freedom; do not independently flip individual AO components. Degenerate orbital subspaces may rotate between independent calculations, so compare overlaps/projectors or densities when individual orbital matching is not well-defined.

Text with fewer printed digits needs a source-precision error allowance. Record a separate tolerance for such fixtures rather than weakening every reference test. ECP systems compare valence/represented populations, not blindly all atomic numbers minus charge.

A surface residual alone is insufficient near a small gradient: a low scalar error can still mean a large positional error. Combine field residual, distance to the independent reference surface, normal consistency, and topology checks. Coarse grids need convergence evidence, not a fixed tiny residual threshold.

## Rendering and interaction checks

Test a browser build and the actual Tauri application on an Apple Silicon Mac. Select at least one additional device/runtime with lower capabilities. Fix the supported OS and browser matrix from those results, including oldest supported WKWebView and a current browser. Intel Mac support remains unclaimed until tested.

Use fixed cameras and image comparisons with tolerances, plus numeric checks for depth, picking IDs, bounds, and nonblank framebuffer coverage. Include resizing, HiDPI, context loss, reload, and repeated open/close. Validate positive and negative materials independently. A screenshot cannot establish orbital normalization, but it can expose missing lobes and incorrect compositing.

Check cancellation, rapid isovalue changes, deletion of an object with an active job, and reopening while old work finishes. Verify that progress reflects actual completed work and that cancelled/partial arrays never enter the completed cache. Material changes should cause no numerical job.

## Benchmark plan

Publish hardware, RAM, OS, browser/webview version, dependency revisions, build flags, viewport resolution, field dimensions, atom/AO/primitive counts, and whether data were cached. Measure cold and warm runs separately, with at least ten repeats for short operations; report median and p95 rather than only the fastest run.

| Workload | Measurements | Proposed target or purpose |
| --- | --- | --- |
| Existing PDB and a documented 10,000-atom scene | Parse time, bond inference, frame time, pick latency | 30 fps at 1280 x 800 for the declared representation; revise only with evidence |
| One orbital, 100 then 1,000 AOs | Evaluation at 128 and 256 cubed, peak memory, cancellation | Establish CPU/WASM/native crossover and whether GPU compute is worthwhile |
| Total density | Matrix and occupied-orbital paths on same inputs | Choose by measured shape/cost, not assumed complexity alone |
| 128/256/512 cubed scalar fields | MC33 extraction, triangle counts, grid upload, peak memory | Establish default budgets; 512 cubed is a stress case, not baseline browser support |
| Shrinkwrap | Seed cost, iteration time, residual, topology, triangle quality | Must improve geometric/mesh-quality objective without losing components |
| Analytic raycasting | Small reference molecules, increasing primitive counts | Define the supported direct-evaluation envelope and fallback threshold |
| Mixed transparent scene | Frame time, layer count, image error against reference | Decide interactive and still-image quality modes |
| Native file | Save/open, compression, chunk reads, copies, resident memory | Verify that chunking actually avoids full-file inflation |

Initial control feedback and cancellation target is 100 ms at the UI, with job stop at the next bounded block and a proposed 500 ms worst-case block budget on supported workloads. A kernel that cannot yield must run in an isolated worker/process with a safe discard mechanism. These numbers guide experiments and are not current performance claims.

## Milestones and exit gates

| Milestone | Deliverables | Exit gate |
| --- | --- | --- |
| M0: Architecture experiments | Native-format tiny reader/writer, C-kernel native/WASM build, representative Three.js scene in Tauri/browser | Correct small orbital/density; MC33 fixture; cube+atoms+transparent OBJ; native file roundtrip; documented runtime limits |
| M1: Scientific document | Frozen profile semantics, JSON Schema, validator, immutable model, provenance and format tests | Supported documents roundtrip; malformed documents fail predictably |
| M2: Molecular viewer | Native open/save, scene tree, R01-R03, R10-R11, camera, picking, object metadata | Usable offline Mac viewer; appearance edits preserve scientific data |
| M3: Quantum fields and meshes | R04-R06, R12; field jobs, cancellation, cache, both orbital signs | Independent values, units, density, and meshing suite passes |
| M4: Remaining rendering modes | R13-R15; transfer function editor; sampled/analytic raycasting; compositing | All required modes pass the same reference scenes and have documented budgets |
| M5: Compatibility adapters | R07-R09 through stages B/C of conversion plan | Original input-format target works via shared conversion/report path |
| M6: Release | Packaging, signing/notarization, compatibility matrix, examples, user documentation | R01-R17 met; browser R18 released only after its own validation |
| M7: Extensions | ORCA/Gaussian converters, optional compute server, justified WebGPU acceleration | Each extension has fixtures and a measured benefit |

M0 is a future experiment plan, not authorization already exercised to write code in this task. Implement these milestones only in the next development phase. No estimate of total calendar duration is defensible until the graphics and scientific-library experiments are measured and team capacity is known.

## Initial work items

1. Freeze basis normalization and explicit angular-polynomial examples for one Cartesian and one spherical F shell.
2. Build two independent canonical wavefunction fixtures and their point/grid reference data.
3. Test gau2grid and MC33 compilation, allocation, and result ownership in native and WASM runtimes.
4. Test the hard graphics scene before expanding the interface: signed orbital, atoms, translucent OBJ, and a volume.
5. Specify the format's machine schema and implement validation before adding broad converters.
6. Implement the simple adapters and inspect loss reports before adding producer-specific quantum adapters.

These work items are ordered dependencies, not simultaneous tasks requiring multiple agents.

## Risks and decision triggers

| Risk | Mitigation | Trigger for revisiting the decision |
| --- | --- | --- |
| Wrong AO convention produces plausible images | Explicit basis functions, independent values, producer fixtures | Any unexplained field discrepancy blocks that importer |
| C/WASM toolchain or unsafe kernel assumptions | Scalar build, audited boundary, bounded allocations, independent checks | Use a separate C/WASM module if Rust linking dominates effort |
| Three.js compositing takes excessive custom work | Test overlapping volumes/surfaces in M0 | Evaluate Mol*/vtk.js viewport adapter before UI expansion |
| Shrinkwrap misses topology | MC33 component seeds, refinement checks, visible failure | Keep mode unreleased until its acceptance profile passes |
| Analytic raycasting is too costly | Conservative interval rejection, narrow supported envelope, sampled mode | Reassess scope/performance expectations using measured scenes |
| Browser memory limits | Chunking, transfer ownership, budgets, optional remote/native jobs | Decline oversized jobs before allocation; never promise native-sized workloads |
| Native format grows without discipline | Required-feature registry and versioned profiles | Add a new profile when old meanings would otherwise change |
| Converters lack required data | Structured completeness/loss report | Request better source export, not fabricated coefficients |
| Dependency licensing is unclear | Inventory exact versions/notices and copied files | Resolve packaging before distribution |

## Decisions still needed during implementation

Minimum macOS version, Intel support, realistic maximum molecule/basis sizes, preferred van der Waals radius dataset, standalone shrinkwrap expectations, and image-export quality targets remain open. They do not block the research recommendation. They should be resolved with representative user files and measured M0 results before committing to release promises.
