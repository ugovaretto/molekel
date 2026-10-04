# As-built architecture

This describes the 0.3.0 development preview, not the complete
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
           +--> molekel-import: format detection, Molden, reports, provenance
           |      +--> molekel-core: model, structure/grid imports, scientific operations
           +--> molekel-format: validated .molekel ZIP encode/decode

Command line (no GUI/WebView):
molekel-convert --> same molekel-import --> molekel-format --> atomic file save

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
| [Import library](../crates/molekel-import/src/lib.rs) | Shared byte detection/dispatch, Molden conventions, import reports, source-byte digest |
| [Converter CLI](../crates/molekel-convert/src/main.rs) | Bounded file reads, batch/JSON/check mode, destination preflight and no-clobber saves |
| [Connectivity](../crates/molekel-core/src/bonds.rs) | Exact k-d-tree neighbors, explicit-plus-inferred display bonds |
| [Native format](../crates/molekel-format/src/lib.rs) and [filesystem adapter](../crates/molekel-format/src/native.rs) | Portable container, checksums/validation, atomic native writes |
| [WASM bridge](../crates/molekel-wasm/src/lib.rs) | `example`, `validate`, `import_document`, compatibility `import_text`, `encode`, `decode`, `sample`, `surfaces`, point/gradient probes |
| [Document UI](../app/src/App.tsx) and [types](../app/src/types.ts) | Document state, controls, dirty state, generation replacement, error recovery |
| [Orbital browser](../app/src/OrbitalBrowser.tsx) | Full orbital metadata table, filtering, transient checkbox selection, bounded multi-orbital generation settings |
| [Worker client](../app/src/science.ts) and [worker](../app/src/science.worker.ts) | Request lifecycle, WASM initialization, single sampled-grid cache, cancellation |
| [Viewport](../app/src/Viewport.tsx) and [sampled renderer](../app/src/volume.ts) | Scene/materials, OrbitControls, picking, PNG download, sampled raycast/volume shaders |
| [File adapter](../app/src/files.ts) | Browser file/download versus native dialog/read/save |
| [Desktop entry point](../app/src-tauri/src/main.rs) | Tauri plugins and `save_native`/version commands |
| [Runner](../tools/run.mjs) and [packager](../tools/package-macos.mjs) | Repeatable builds/tests and verified tester artifacts |

The Rust workspace's five default members are core, format, import, converter,
and WASM. The desktop shell is a workspace member but requires an explicit
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

The file adapter obtains bytes. The shared `molekel-import::import_bytes`
returns a validated document and structured format/warning/`requires_save` report. Native ZIPs
go through bounded decoding; the Molden header takes precedence over a text
filename. Other supported extensions use existing Rust importers. Successful loading
replaces the document and resets the camera/render mode. If the selected field
is an authoritative grid, Open prepares its bounded display sample immediately;
without saved meshes the initial mode is volume. A sampling failure still opens
the validated document in mesh mode with an error, so its full data can be saved.
An import failure leaves the previous document available and shows an error.
Opening another document cancels pending worker jobs. Generation tokens and a
live unsaved-edit check protect the entire read/import/sample sequence.
External imports start dirty. Native imports supplement missing display bonds
while preserving explicit bonds and their order, scientific inputs, and saved
meshes. Additions produce a warning and `requires_save: true`; otherwise the
decoded document is unchanged and starts clean. Raw format decoding remains
exact and does not infer bonds. Original
Molden sections are not embedded wholesale: preserve inputs and read the report.

Molden shells become explicit normalized polynomial contractions. Spin blocks,
occupations, and energies are retained; complete listed occupations generate
explicitly labeled occupation-derived matrices. This is not a recovered correlated
density. Producer and spherical/Cartesian conventions are checked at the import
boundary, not hidden in the evaluator. See [the exact profile](molden-import.md)
and [independent producer fixtures](../fixtures/molden/README.md).

The [Convert Files dialog](../app/src/ConvertFiles.tsx) uses the same worker and
library, serializing imports and retaining bounded encoded outputs separately
from the open document. It requires explicit per-result Save, supports cancellation,
and does not replace the scene. The modal excludes concurrent scene jobs because
cancellation terminates the shared worker. Its outputs are transient until saved.
The UI links the Rust library through WASM; it does not spawn the CLI or invoke
vendor programs. The CLI is a separate headless frontend for scripts/batches.

All supported imports compute display bonds from positions, including cube and
native documents. XYZ/PDB convert angstrom positions to bohr. PDB uses `pdbtbx`
plus bounded compatibility preflight and explicit
connection handling. `kiddo` supplies an exact immutable k-d tree; covalent
cutoffs and deterministic coordination filtering supplement explicit bonds.
See [structure imports](structure-imports.md) for the scientific tradeoffs,
selection policy, loss notices, and exhaustive-neighbor comparisons.

Cube imports preserve the authoritative affine scalar grid, converting source
z-fastest ordering into model x-fastest ordering. Single-field density cubes and
negative-atom-count cubes containing exactly one dataset are supported in the
standard bohr profile. A dataset identifier is metadata, not reconstructed
orbital coefficients. For low-amplitude fields the initial display isovalue is
adjusted below the original peak; samples are not normalized or changed.
Display sampling may downsample the grid but does not mutate the original array.
Values that overflow f32 fail preview preparation, not import or persistence.
See [cube import](cube-import.md) for the exact limits. Unsupported profiles
fail explicitly. Other new formats are deferred; a direct ORCA/GBW reader is
not part of this increment. Qualified ORCA Molden exports use the shared library.

## Generation, cache, and rendering

1. The UI requests `sample` or `generate` for the selected field and resolution,
   with an isovalue for generation, omitting existing meshes from the request.
2. The worker validates the document. Its cache key includes basis, orbital,
   density, grid data, selected field, and resolution. Appearance and isovalue
   are excluded, allowing the sampled grid to be reused when only those change.
3. Rust evaluates an analytic field on a bounded domain or resamples an imported
   grid. Analytic domain sizing currently uses the most diffuse Gaussian
   exponent; it is a preview heuristic, not a proven surface-containment bound.
4. A `sample` request returns only the grid. Open, selecting a stored grid, or
   changing its display resolution uses this path without creating surfaces.
   For `generate`, `mcubes` extracts positive and negative isosurfaces. Empty meshes are omitted.
   Affine mesh positions/normals are converted to model coordinates.
5. The UI stages the generated meshes, preflights native encoding of the combined
   document through Rust, then replaces surfaces of the selected field, retains other fields'
   surfaces, stores the transient grid separately, and marks the document dirty.

Analytic grid sampling computes scalar AO values only, resolving the field once
and reusing an AO buffer. The separate value-and-gradient reference evaluator
is unchanged. Real density sampling folds each off-diagonal pair into
`Pij + Pji`, retaining diagonal terms and both halves of nonsymmetric matrices;
it does not reconstruct arbitrary matrices from orbital occupations or discard
small coefficients. If a folded coefficient overflows, sampling uses the
original full matrix. Extreme intermediate products use an alternate
multiplication order rather than overflowing an otherwise representable value.
There is no estimated CPU-work cutoff for analytic sampling. The previous
150-million-work-unit rejection has been removed, not increased or bypassed
for selected files. Sampling retains its API range of 12 through 80 per axis,
and the UI continues to offer 24, 32, 40, and 48. It does not silently lower
the requested resolution; the 125-AO density in `data/molden.input` is no longer
restricted to 24 cubed by a work estimate. Finite-value and domain checks,
imported-grid bounds, the two-million-vertex worst-case mesher allocation bound,
and document/container budgets still apply. Removing the CPU estimate does
not establish a maximum runtime or remove memory-related rejections.

The orbital browser lists `document.orbitals` in original document order,
using one-based row numbers and imported label/spin/occupation/energy metadata.
Missing numerical metadata displays as `--`; unknown spin is labeled Unknown.
Search matches number, ID, label, or spin. Filtered select-all affects only
listed rows; hidden selections remain selected. Saved counts derive from
surface field IDs, including hidden meshes. The browser owns checkbox selection
and draft isovalue/resolution locally, with the active orbital initially checked.
Closing without generating does not change the document.

Multi-orbital generation uses the same `generate` operation serially for up to
32 distinct orbital IDs in document order. The parent rechecks the selection
and stages all results without publishing partial meshes. The retained plus
generated geometry is bounded by 128 surfaces and an estimated 128 MiB of
f64 positions/normals and u32 indices. The worker's `validate` operation also
encodes the complete combined document through the Rust native format and
discards the output, checking model and native-container budgets with all
retained scientific inputs before commit. This persistence preflight has an
additional encoding/memory cost but writes no file. These are preview rejection
budgets, not guarantees of acceptable memory use or latency. Cancellation or
any batch error leaves previous surfaces untouched. A successful batch replaces
only selected fields' meshes, keeps unrelated saved meshes, and enters mesh mode.
The active field stays unchanged if its orbital was selected, otherwise it
becomes the first selected orbital. Only that field's display grid is retained
in the UI for sampled rendering; the worker still has a single-entry cache.

Request IDs resolve worker promises and associate progress events with the
request that emitted them. Sampling, meshing, and validation stage events update
the footer while the UI remains separate from synchronous Rust/WASM work.
They indicate phases, not completed voxel counts, percentages, or estimated
remaining time. Stale progress is ignored through request and generation-token
checks, just like stale completion. Cancellation terminates the same Web Worker,
rejects outstanding work, and clears its cache; it does not cooperatively stop
inside a Rust loop. The next request starts a new worker. Previous completed
geometry remains intact, and cancellation is not a resumable checkpoint.

Mesh mode renders durable surface records. Sampled raycast and volume modes
use a 3D texture and replace the selected field's mesh display. Other fields'
meshes may remain. These shaders are bounded sampled previews, not direct
analytic evaluation or robust root isolation. Ordinary mesh transparency and
single-volume compositing do not correctly solve all mixed-scene overlaps.
Skewed and reflected affine grids use the full grid transform; reflected box
winding is corrected for back-face ray-entry rendering, including inside views.

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

Persistent view settings include representation, active field selection, colors,
opacity, and isovalue. Camera, active render mode, grid-resolution control,
orbital-browser checkbox selection, worker cache, transient display grid, and
dirty state are not persisted. Multi-orbital selection produces independent
saved meshes, not a new combined orbital/density or multiple active volumes. A
loaded document can show saved geometry without recomputation. A selected
authoritative grid also gets a new transient display sample on Open, making
sampled modes available immediately. Analytic orbital/density fields still need
Generate to prepare their transient display grid after reopening. No sampled
preview is a substitute for the complete authoritative grid saved in the file.

Browser saving encodes through WASM and requests a download. Desktop saving
encodes through WASM, revalidates in native Rust, asks for a destination, writes
and syncs a new sibling file, then renames it into place. Cancellation or failed
validation does not replace the existing file. Windows replacement semantics
need separate qualification. File-format tests are not proof that native
window close, Save dialog, or download interactions all work end to end.
Desktop destinations must end in `.molekel`; imported external source paths
are protected against replacement. Batch conversion protects all selected sources.
CLI saves protect inputs and output aliases, require `--force` for existing
destinations, and atomically publish initially absent files without replacement
even with `--force`. This uses same-directory hard links; filesystems that do
not support them fail explicitly instead of falling back to an unsafe overwrite.

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
embeds notices, ad-hoc signs the staged bundle and standalone converter, includes documentation/source,
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
