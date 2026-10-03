# As-built architecture

This describes the 0.1.0 development preview, not the complete
[proposed architecture](../../doc/rewrite/03-architecture-decision.md).
Read [status](status.md) for dated verification and uncompleted release gates.

## Boundaries and ownership

```text
Browser                          macOS application
file input / downloads           Tauri 2 / system WebView / native dialogs
         \                       /
          React document UI (App.tsx)
           |                   |
           |                   +--> Three.js / WebGL2 (Viewport.tsx, volume.ts)
           v
     request IDs + Web Worker (science.ts, science.worker.ts)
           |
     wasm-bindgen JSON/byte bridge (molekel-wasm)
           |
           +--> molekel-core: validation, import, bonds, evaluation, meshing
           +--> molekel-format: validated .molekel ZIP encode/decode

Desktop save only:
encoded bytes --> Tauri save_native --> format validation --> atomic file save
```

Both frontends currently perform scientific jobs in the **same Rust/WASM Web
Worker**. The desktop does not yet use native threaded evaluation. The Rust
core also builds natively for tests, reference generation, and the format used
by native saving. There is no calculation server, remote service, or database.

| Component | Responsibility and entry points |
| --- | --- |
| [Core model](../crates/molekel-core/src/model.rs) | `Document`, validation, explicit scientific units, affine grid geometry, resource limits |
| [Computation](../crates/molekel-core/src/compute.rs) | AO/field values and gradients, sampling/resampling, source hashes, classic marching cubes |
| [Imports](../crates/molekel-core/src/import.rs) and [PDB importer](../crates/molekel-core/src/import/pdb.rs) | Supported text profiles and recorded conversion losses |
| [Connectivity](../crates/molekel-core/src/bonds.rs) | Exact k-d-tree neighbors, explicit-plus-inferred display bonds |
| [Native format](../crates/molekel-format/src/lib.rs) and [filesystem adapter](../crates/molekel-format/src/native.rs) | Portable container, checksums/validation, atomic native writes |
| [WASM bridge](../crates/molekel-wasm/src/lib.rs) | `example`, `validate`, `import_text`, `encode`, `decode`, `sample`, `surfaces`, point/gradient probes |
| [Document UI](../app/src/App.tsx) and [types](../app/src/types.ts) | Document state, controls, dirty state, generation replacement, error recovery |
| [Worker client](../app/src/science.ts) and [worker](../app/src/science.worker.ts) | Request lifecycle, WASM initialization, single sampled-grid cache, cancellation |
| [Viewport](../app/src/Viewport.tsx) and [sampled renderer](../app/src/volume.ts) | Scene/materials, OrbitControls, picking, PNG download, sampled raycast/volume shaders |
| [File adapter](../app/src/files.ts) | Browser file/download versus native dialog/read/save |
| [Desktop entry point](../app/src-tauri/src/main.rs) | Tauri plugins and `save_native`/version commands |
| [Runner](../tools/run.mjs) and [packager](../tools/package-macos.mjs) | Repeatable builds/tests and verified tester artifacts |

The Rust workspace's default members are the three scientific/format/WASM
crates. The desktop shell is a workspace member but requires an explicit
workspace check or Tauri build. `cargo test` alone is not a desktop check.

## Scientific contract

`Document` owns one geometry, bonds, explicit basis functions, real orbitals,
real density matrices, authoritative sampled grids, saved surfaces, view
settings, and provenance strings. Scientific objects and surfaces have IDs;
the model validates uniqueness and relationships. TypeScript mirrors this
model manually, so shared changes require updates and roundtrip tests on both
sides. Neither the JSON bridge nor TypeScript types replace Rust validation.

- Positions, basis centers, grids, mesh positions, and the current scene are
  in **bohr**. Picked atom positions are displayed in angstroms. Energies, when
  supplied, are hartree; missing energies/occupations remain optional.
- A basis function is a sum of Cartesian polynomial terms multiplied by a
  contraction of Gaussian primitives about its explicit center. Radial
  coefficients already contain normalization. No hidden normalization, shell
  ordering, or spherical convention is applied by the evaluator.
- Polynomial degrees through G are supported. Orbitals are real linear AO
  combinations; density values contract the supplied full row-major matrix
  with AO values, including nonsymmetric transition matrices. Analytic gradients
  use the same explicit representation. Complex/spinor profiles are absent.
- A grid position is `origin + i*axes[0] + j*axes[1] + k*axes[2]`. Axes are
  step vectors, not total extents. Storage is x-fastest:
  `(k * ny + j) * nx + i`. Skewed and reflected nonsingular grids are valid.
- Scientific values and native arrays are f64. The current mesher interpolates
  using f32; its results live in f64 containers with the limitation recorded.
  Three.js geometry and GPU textures use f32. Do not infer f64 rendering precision.

[Independent PySCF fixtures](../fixtures/pyscf/README.md) establish the tested
normalization/convention subset. The application does not solve SCF, reconstruct
missing wavefunctions, or infer a density matrix from a PDB file.

## Import and open flow

The file adapter obtains bytes. `.molekel` goes through bounded ZIP decoding;
other supported extensions go through Rust text import. Successful loading
replaces the document, resets the camera/render mode, and clears transient
sampling state. An import failure leaves the previous document available and
shows an error. Opening another document cancels pending worker jobs.

XYZ/PDB convert angstrom positions to bohr and compute bonds before returning
the document. PDB uses `pdbtbx` plus bounded compatibility preflight and explicit
connection handling. `kiddo` supplies an exact immutable k-d tree; covalent
cutoffs and deterministic coordination filtering supplement explicit bonds.
See [structure imports](structure-imports.md) for the scientific tradeoffs,
selection policy, loss notices, and exhaustive-neighbor comparisons.

Cube imports preserve the authoritative affine scalar grid. Display sampling
may downsample it but does not mutate the original array. Unsupported profiles
fail explicitly. General converter infrastructure remains planned, not hidden
inside the current import dispatcher.

## Generation, cache, and rendering

1. The UI requests generation for the selected field, resolution, and isovalue,
   omitting existing meshes from the computation request.
2. The worker validates the document. Its cache key includes basis, orbital,
   density, grid data, selected field, and resolution. Appearance and isovalue
   are excluded, allowing the sampled grid to be reused when only those change.
3. Rust evaluates an analytic field on a bounded domain or resamples an imported
   grid. Analytic domain sizing currently uses the most diffuse Gaussian
   exponent; it is a preview heuristic, not a proven surface-containment bound.
4. `mcubes` extracts positive and negative isosurfaces. Empty meshes are omitted.
   Affine mesh positions/normals are converted to model coordinates.
5. The UI replaces surfaces of the selected field, retains other fields'
   surfaces, stores the transient grid separately, and marks the document dirty.

Request IDs resolve worker promises. Cancellation terminates the worker,
rejects outstanding work, and clears the worker cache. The next request starts
a new worker. A UI generation token prevents stale completion from overwriting
new state; cancellation is not a resumable checkpoint or cooperative progress API.

Mesh mode renders durable surface records. Sampled raycast and volume modes
use a 3D texture and replace the selected field's mesh display. Other fields'
meshes may remain. These shaders are bounded sampled previews, not direct
analytic evaluation or robust root isolation. Ordinary mesh transparency and
single-volume compositing do not correctly solve all mixed-scene overlaps.

The viewport owns its Three.js engine and disposes rebuilt geometry/materials.
Camera damping drives an animation loop with rendering on movement or changed
scene state. Scene objects are rebuilt on document/grid/mode changes; atoms
and bonds are individual meshes, not a qualified large-structure instancing path.

## Persistence versus transient state

The [native preview format](preview-format.md) is a ZIP with `manifest.json`
and checksummed, little-endian f64/u32 arrays. It is **not** the full proposed
schema, a streaming container, or a backward-compatible reader for every
historical Molekel file. Arrays are whole entries and saves can make multiple
in-memory copies.

Saved meshes include field ID, source hash, signed isovalue, mesher/version,
grid geometry, precision, vertices/normals/indices, and appearance. The associated
quantum inputs remain in the same document. Hashes exclude appearance, sort
JSON keys, and canonicalize negative zero because the JavaScript JSON bridge
folds its sign. A source mismatch is rejected; revision-aware historical assets
are not yet implemented. Never discard saved meshes as disposable cache data.

Persistent view settings include representation, field selection, colors,
opacity, and isovalue. Camera, active render mode, grid-resolution control,
worker cache, transient display grid, and dirty state are not persisted. A
loaded document can show saved geometry without recomputation while sampled
rendering remains disabled until regeneration.

Browser saving encodes through WASM and requests a download. Desktop saving
encodes through WASM, revalidates in native Rust, asks for a destination, writes
and syncs a new sibling file, then renames it into place. Cancellation or failed
validation does not replace the existing file. Windows replacement semantics
need separate qualification. File-format tests are not proof that native
window close, Save dialog, or download interactions all work end to end.

## Safety and distribution

Importers, model validation, computation, and ZIP parsing each enforce bounds.
These include file/inflated byte limits, finite numbers, checked dimensions,
indices, supported profiles, checksums, entry coverage, and safe archive paths.
They are defensive limits, not performance promises. The desktop read adapter
currently reads a selected file before the UI byte-budget check; avoid claiming
streaming or universal pre-allocation protection at the UI boundary.

The [Tauri config](../app/src-tauri/tauri.conf.json) defines a local-content CSP
and embedded frontend; [capabilities](../app/src-tauri/capabilities/default.json)
enable the local dialog/read workflow. Native saving owns the destination
dialog inside Rust. No arbitrary shell-execution command or update service is
exposed to the frontend.

The [tester packager](tester-packaging.md) builds an optimized arm64 app,
embeds notices, ad-hoc signs the staged bundle, includes documentation/source,
and verifies the extracted ZIP before publishing it locally. It does not
notarize or upload. macOS/Apple Silicon is the exercised platform; portable
components make Linux/Windows feasible but do not establish support.

## Decisions still open

The clean-Rust requirement supersedes the early scientific C-library proposal.
The scalar Rust evaluator, classic marching cubes, bohr scene, JSON bridge,
single worker/cache, preview schema, and sampled renderers are current bounded
choices, not all final architecture decisions. Native parallel jobs, optimized
collocation, topology-certified meshing, analytic shaders, shrinkwrap, full
converters, formal schema evolution, camera persistence, and qualified release
platforms remain open. Track them in [status](status.md) and [handoff](handoff.md).
