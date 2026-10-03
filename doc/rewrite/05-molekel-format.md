# Molekel native scientific document format

Recommendation: define `.molekel` as a versioned ZIP container with a UTF-8 JSON manifest and typed binary array entries. Its purpose is to preserve a scientific document independently of the program that generated it and independently of the renderer. This document is a **proposed format 0.1 design**, not a frozen schema or an implemented reader.

A schema-first approach lets the viewer and numerical core proceed using small native fixtures while external converters are developed later. The extension must not be confused with ORCA's older `.mkl` exchange format.

## Container decision

| Option | Benefits | Costs for this project | Decision |
| --- | --- | --- | --- |
| JSON only | Inspectable and easy to prototype | Large numeric arrays, parsing overhead, and loss of explicit binary layout | Manifest and small metadata only |
| ZIP with JSON and binary arrays | One portable file; ordinary libraries; explicit chunks and optional compression | Molekel must specify array semantics and validate container limits | Selected for initial native files |
| HDF5 | Mature hierarchical scientific datasets and native tooling | Browser integration needs an additional runtime/library and careful memory behavior | Possible interchange/backend format |
| Zarr | Standard chunked typed arrays suited to distributed storage | Full codec/store profile and single-file packaging add early integration choices | Strong candidate for later large remote fields |
| SQLite | Metadata queries and transactions | A database adds little to the first immutable scientific arrays | Consider for local catalog/cache, not native interchange |

See the primary [ZIP specification](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT), [HDF5 documentation](https://docs.hdfgroup.org/documentation/hdf5/latest/index.html), and [Zarr specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html). The selection is an engineering tradeoff, not a benchmark. It does not claim ZIP is the best store for terabyte volumes.

## Logical layout

| Entry | Content |
| --- | --- |
| `manifest.json` | Format identity, version, object identities, array descriptors, provenance, and required features |
| `arrays/<id>/<chunk>.bin` | Canonical scientific arrays and optional derived arrays |
| `attachments/<id>` | Optional original input files or supporting records |
| `preview.png` | Optional small preview, never required for scientific interpretation |

The manifest is authoritative. A filename alone does not imply meaning. Every referenced entry has an uncompressed byte length and SHA-256 digest. An optional preview is not proof that the scientific arrays are complete.

Use ZIP Store or Deflate only in the initial profile; writers place the manifest first, and readers also use the central directory. Each independently compressed chunk is an entry. ZIP64 may be used, but a reader is allowed to refuse a file exceeding its declared runtime budget. Do not require the entire archive to be inflated at once.

## Manifest entities

| Entity | Required meaning |
| --- | --- |
| Document | Format name, major/minor version, unique document ID, creator/version, creation time, required features, object lists |
| Structure | Stable atom IDs, atomic numbers, positions, coordinates unit, optional bonds/residues/models, charge and multiplicity when known |
| Basis | Stable center and shell IDs, explicit function definitions, AO order, normalization semantics, source conventions |
| Orbital set | Basis ID, geometry revision, coefficients, energies if known, occupations if known, spin interpretation, state/method metadata |
| Density matrix | Basis ID, geometry revision, array, matrix kind, state, spin, represented electron count if known, provenance |
| Field | Analytic definition or sampled grid, quantity, units, source/state identity, domain, precision and error information |
| Mesh | Vertex/index arrays, coordinate unit, optional normals, object groups, materials, metadata, and source-field linkage if derived |
| View | Camera, visible objects, representation/radius settings, per-sign materials, transfer functions, clipping, selected orbital |
| Provenance | Source hashes, producer/converter versions, conversion operations, missing-data warnings, derivation records |

Support structure-only, field-only, and mesh-only documents as valid profiles. A wavefunction requires an explicit geometry/basis relationship, but an imported grid or generic OBJ need not contain atoms. Support multiple structures and several calculation states without relying on array position as identity.

## Binary array contract

- Initial element types: unsigned 8/16/32-bit integers, signed 32-bit integers, IEEE float32, and IEEE float64. Multi-byte values are little-endian.
- Every descriptor gives `dtype`, `shape`, named axes, element units where applicable, and chunk descriptors. Each chunk specifies its origin, actual shape, entry path, uncompressed length, and digest.
- Arrays use C order: the last dimension varies fastest. Chunk interiors have the same order; there are no hidden headers or padding bytes.
- Positions have shape `[atom, xyz]`. Orbital coefficients have shape `[orbital, ao]`. Density and overlap matrices have shape `[ao, ao]`. Mesh indices have shape `[triangle, corner]`.
- Scalar grid shape is `[z, y, x]`, so x is fastest. Store distinct channels as distinct arrays/field records with shared geometry metadata. This makes channel identity and scalar units unambiguous.
- Chunk coverage must exactly fill the array without overlap or missing regions. Edge chunks declare their actual shape. Optional cache absence is represented by an absent cache record, not missing array chunks.
- Authoritative coordinates, basis exponents/coefficients, MO coefficients, and density matrices use float64. Derived rendering grids and meshes may use float32 with recorded precision.
- Reject NaN/infinity in required scientific arrays. Unknown information is explicit metadata absence, not a numeric sentinel.
- Dimension products and byte lengths use checked arithmetic. Metadata integers must stay within the exact interoperable JSON-integer range; larger arrays require a future profile rather than imprecise counts.

Proposed initial writer policy: one entry for small arrays, grid bricks of at most 64 x 64 x 64 samples, and contiguous orbital-row blocks for large coefficient arrays. These are storage choices and must not alter array semantics. Readers should accept other valid chunk shapes within their resource budget.

## Molecular structure

Positions and basis centers use bohr in the canonical scientific profile. The interface can show angstroms. Preserve original units and the conversion constant/version in provenance. Atom identity stays stable if a converter reorders data. Nuclear charge, atomic number, effective core electron count, isotope, and partial charge are separate fields.

Bonds reference stable atom IDs and record order plus origin: explicit input, library-perceived, heuristic, or user-edited. Unknown order is allowed. Preserve PDB chain/residue/alternate-location identifiers as metadata. Different conformations use geometry revisions or model records; an orbital set refers to the particular geometry for which it was computed.

Changing a display transform moves the whole scientific object visually. Changing atomic coordinates creates a new structure revision and does not recalculate its electronic structure.

## Basis semantics

The initial profile is `real-gaussian-polynomial-v1`. Its evaluation equation is given in [scientific methods](04-scientific-methods.md). Each shell identifies a center and shared primitive exponents. Each AO component has an explicit angular polynomial and a radial coefficient vector over those exponents. AO order is the stored component order referenced by the orbital matrix.

Angular terms carry nonnegative integer powers and finite weights. Primitive exponents are positive. The combined stored weights already encode primitive and contraction normalization and spherical-harmonic phases. **Readers must not renormalize or guess an AO convention.** A component can represent a Cartesian monomial or a real spherical combination. SP and general contractions expand into explicit components sharing exponent arrays.

This slightly more explicit representation is intentional: it removes dependence on whether a producer means one particular ordering or sign convention by a label such as "F". Original shell labels and coefficient conventions remain in provenance. Optimized evaluation may regroup equivalent components, but the explicit definitions remain the authority.

Support through degree four in the first required evaluator profile. Higher-degree functions or Slater/complex data must be tagged with a required feature and preserved or rejected explicitly, never reduced to a lower degree. A future compact standardized basis encoding can coexist if it defines an exact conversion to these functions.

## Orbitals and density matrices

An orbital set references the AO basis and records each orbital's stable ID, index in its source, spin, occupation, energy and energy unit when present. Preserve source ordering; UI sorting does not rewrite coefficients. Missing energy is not zero. Missing occupation permits orbital rendering but does not authorize reconstruction of total electron density.

Use two alternatives: spatial orbitals with explicit occupations in [0,2], or alpha/beta sets with occupations in [0,1]. Record which convention is used. Fractional values are valid. A restricted open-shell state needs explicit occupations/spin assignments for spin-density evaluation.

A real density matrix is a full row-major `[ao,ao]` array in the declared AO basis. Store whether it is total, alpha, beta, spin, difference, or transition density and identify the calculation state. Symmetric storage compression is deferred to avoid a second initial matrix layout. An optional overlap matrix enables `trace(P S)` checks.

If both coefficients and a matrix are supplied, record whether they represent the same density. Preserve independent correlated densities even if they differ from a density reconstructed from canonical orbitals. Never overwrite a supplied matrix merely because another one can be computed.

## Fields and meshes

An analytic field names its evaluator, basis, orbital or density source, and exact source revision. It contains no executable expression or arbitrary shader program. A sampled field names origin, all three step vectors, sample counts, scalar units, channel/state identity, and boundary policy. Unknown quantities are permitted; unknown coordinate units must be resolved before spatial overlay.

Native scalar-field coordinates use bohr. OBJ mesh coordinates can retain declared source units but must have a resolved conversion to the scene. Mesh records support names, descriptions, typed custom properties with units, group IDs, molecule association, and a rigid/scale transform. An arbitrary mesh is not automatically an isosurface.

A derived grid or mesh records all input hashes, algorithm/version, isovalue if applicable, precision, domain, resolution, tolerances, and any smoothing. It is dispensable only if its authoritative inputs remain present. An imported cube is authoritative sampled data, not a disposable cache unless there is also a complete independent source definition.

## Presentation state

Store user-selected atom color overrides, radius dataset ID/version, representation sizes, orbital sign colors, opacity, clipping, transfer-function control points and their units, camera, and object visibility. Save numerical isovalues exactly as entered in the field's units.

Use sRGB for stored display colors and document conversion to the renderer's linear working space. Transfer-function opacity/extinction semantics include the reference length unit so changing the renderer's sampling step does not change the view unintentionally.

## Provenance and incomplete conversion

Converters record source format and producer version when known, SHA-256 of source bytes, selected job/geometry/state, converter version, units and AO transformations, numeric precision of the source, and warnings. Original attachments are optional and subject to the user's file-size preference.

Use feature availability states such as present, absent-in-source, unsupported, and failed-validation. For example, geometry and MO energies without basis coefficients form a useful structure document, but not an evaluable wavefunction. A converter must never fill missing coefficients with zeros and mark conversion successful.

## Validation and evolution

1. Validate container structure and manifest identity before allocating scientific arrays.
2. Check versions, required features, sizes, chunk coverage, byte lengths, and digests.
3. Validate all IDs/references and dimensional relationships, including `nAO` and `nMO`.
4. Validate units, basis definitions, numeric values, matrix semantics, and geometry associations.
5. Derive available application operations from validated entities.

Unknown required features prevent evaluation of affected entities. Unknown optional metadata can be preserved. A newer major version is refused for ordinary editing; a read-only preview may be offered only if its supported profile is unambiguous. Minor versions add optional features without changing old meanings. Migrations write a new document and record source/target versions.

Archives must reject duplicate paths, traversal paths, symbolic links, encrypted entries in the initial profile, and unreasonable decompression or allocation requests. Process entries as data; do not extract arbitrary contents into user directories. Atomic native save writes and validates a temporary sibling file before replacement. Browser save uses the same validated output model.

## Relationship to QCSchema

Use [QCSchema](https://molssi-qc-schema.readthedocs.io/en/latest/) as an interoperability guide and later converter target. Its [wavefunction schema](https://molssi-qc-schema.readthedocs.io/en/latest/auto_wf.html) documents AO conventions and column-major matrices. The proposed `.molekel` arrays are row-major and carry explicit AO functions, so the formats are not byte-compatible and `.molekel` must not claim QCSchema conformance.

An adapter must convert matrix orientation and AO conventions explicitly, and map scientific provenance separately from visualization state. The native format also serves grid-only and mesh-only documents that need different application semantics.

## Conditions before freezing version 1

Demonstrate a small molecule, unrestricted fractional occupations, Cartesian/spherical D/F/G shells, a supplied density matrix, a skewed grid, and an OBJ scene through save/reopen. Compare independent converters producing the same physical wavefunction. Measure browser memory for large chunked arrays. Then publish JSON Schema, binary fixtures, a validator, migration rules, and an extension registry as implementation deliverables.

Those executable/schema artifacts are intentionally future work under the instruction not to write code yet. The proposed container, field meanings, and compatibility rules are concrete enough to guide that implementation without claiming the draft has been standardized.
