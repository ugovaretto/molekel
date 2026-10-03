# Molekel rewrite research and development plan

Research date: 3 October 2026. Status: recommendation for development; no application implementation has been started.

## Recommendation

Build a web application that works locally and package the same interface as a macOS application with Tauri 2. Use TypeScript, React, and Three.js for the interface and rendering, and a Rust numerical core compiled both to WebAssembly and native code. Reuse scientific C libraries behind narrow interfaces where they pass the portability and correctness checks below. Start with WebGL2 rendering; evaluate WebGPU acceleration against measured workloads before adopting it.

Adopt **`.molekel` as the native scientific document format**. Store molecular structure, basis functions, orbitals, density matrices, sampled fields, meshes, provenance, and presentation settings in one versioned file. Develop the application against this contract first. Add external format converters as independent adapters, so ORCA and Gaussian output changes do not require changes to the renderer.

This is an engineering recommendation based on the requested combination of macOS, possible browser use, quantum chemistry, and several custom rendering methods. It is not a measured performance ranking. The [architecture decision](03-architecture-decision.md) compares all four options in the brief, plus existing molecular viewers and a simpler Rust graphics alternative.

## Main findings

1. The existing orbital and density algorithms are valuable reference material, but their implementation uses shared mutable state and is coupled to VTK and the application UI. Reusing the whole application would retain substantial migration work.
2. Scientific correctness needs its own acceptance tests. Inspection found a forced Molden F-shell convention, integer occupation assumptions, and cube parsing and grid-transform weaknesses. The legacy output must not be the only reference.
3. Marching cubes, direct volume rendering, and shader isosurface raycasting are related but distinct user modes. Shrinkwrap requires an explicit topology policy because molecular orbital surfaces can have disconnected lobes and holes.
4. The native format is more than a cache. Basis functions and coefficients are authoritative scientific data; grids and meshes are optional derivatives. A cube or OBJ file alone cannot restore missing quantum chemistry information.
5. A server is useful for unusually large jobs and optional converters. It need not be a dependency for opening a local file or viewing an orbital.

## Documents

| Document | Purpose |
| --- | --- |
| [Legacy review](01-legacy-review.md) | Documentation inventory, code paths, migration risks, and reusable assets |
| [Requirements](02-requirements.md) | Traceable v1 scope, interpretations, and acceptance criteria |
| [Architecture decision](03-architecture-decision.md) | Language and framework choice, alternatives, deployment, and component boundaries |
| [Scientific and rendering methods](04-scientific-methods.md) | Equations, precision, meshing, shrinkwrap, volume rendering, raycasting, and memory |
| [Native file format](05-molekel-format.md) | Proposed `.molekel` container, scientific semantics, schema evolution, and validation |
| [Conversion strategy](06-conversion-strategy.md) | Input formats, converter stages, missing-data policy, and ORCA/Gaussian routes |
| [Validation and delivery](07-validation-and-delivery.md) | Numerical fixtures, rendering checks, benchmarks, milestones, and risks |
| [Sources](08-sources.md) | Primary documentation, source code, papers, and research limitations |
| [Implementation plan](09-implementation-plan.md) | Ordered work on branch `2026`, milestone exit gates, portable saved surfaces, and early Linux checks |

## Scope and assumptions

- macOS is the first supported desktop platform. Apple Silicon is the first proposed validation target; minimum macOS version and Intel support are decisions for the first compatibility milestone.
- Development stays on the existing `2026` branch. Linux build checks start early; Linux and Windows release support require their own runtime and packaging validation.
- The browser application should support the same scientific document and essential viewing operations. Dataset limits may differ by runtime.
- The initial scientific scope is finite molecules with real Gaussian orbitals through G shells, explicit alpha/beta or spatial occupations, and real density matrices. Slater bases, complex spinors, and periodic wavefunctions remain identifiable in the format but require later evaluation profiles.
- "Van der Waals radius" means space-filling atomic spheres. A fused van der Waals envelope is an additional interpretation to confirm, not silently substituted for the requested representation.
- "OBJ information" means editable name, units, transform, association with a molecule, material, provenance, and descriptive properties. OBJ does not imply atoms, basis functions, or a scalar field.
- The user explicitly allowed a native format and later converters. The proposed first technical milestone therefore accepts `.molekel`; PDB, XYZ, Molden, cube, and OBJ remain the compatibility target for the full v1 release. ORCA and Gaussian adapters can follow without blocking the core.

## What was completed

The Actions in [the brief](../../prompt-and-info.md) are covered: documentation review, focused source review, implementation research, and a framework decision, with supporting development documents. The later native-format requirement is incorporated throughout.

Reviewed source baseline: branch `2026`, commit `bd5fae841a73ee362cc7be87ca21fccdfb753a17`. Local documentation branch: `gh-pages`, commit `3e35935e4deacfc409f3dc9f032cb1b23ad0bb5f`. The existing untracked brief was left untouched.

This was a static review and research task. No legacy build, dependency installation, renderer prototype, numerical experiment, or performance benchmark was run. The future checks in these documents are acceptance gates, not completed test results.
