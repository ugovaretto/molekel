# Native preview profile

This implementation is an experimental subset of the research proposal, not
its frozen version-1 schema. New files use `format: eigenvista`,
`version: [0, 1]`, `required_features: [eigenvista-preview-polynomial-v1]`, and
`coordinate_unit: bohr`. The exact model is defined in the
[Rust core](../crates/eigenvista-core/src/model.rs).

## Naming and compatibility

The canonical filename suffix is `.eigenvista`. The decoder also accepts the
previous matched identifiers, `format: molekel` with
`required_features: [molekel-preview-polynomial-v1]`, for existing `.molekel`
preview documents. Mixing old and new identifiers is not a supported profile.
This compatibility does not extend to arbitrary files from legacy C++ Molekel.

The rename leaves the `[0, 1]` document shape, scientific arrays, field/source
hashes, and cached mesh semantics unchanged. Decoding an old file does not
rewrite it or require regeneration of its surfaces. Open still applies the
existing, separately reported missing-bond supplementation policy below.
The shared importer reports `ImportReport.format = "eigenvista"` for both
old and new native profiles.

Encoding always writes the new EigenVista identifiers. Native Save accepts
both `.eigenvista` and the legacy `.molekel` suffix, defaulting to `.eigenvista`,
but choosing the old suffix does not produce the old profile. Newly saved
files therefore are not guaranteed readable by previous Molekel builds.
Keep the original old-profile file when compatibility with those builds is
required. No bulk file migration is performed by the rename.

Molden conversion uses this same profile without a schema change. Explicit
normalized basis functions, orbitals/occupations/energies, occupation-derived
density matrices, inferred bonds, source-byte hash, and loss notices become
ordinary document data. Original Molden text and omitted sections are not
embedded. Subsequent saved meshes retain the usual field/source associations;
they do not require the original Molden file to reopen. See the
[Molden import contract](molden-import.md) for normalization and completeness limits.

Cube conversion also uses this unchanged profile. The complete affine grid,
original finite f64 samples (reordered x-fastest), atoms, inferred bonds, source
digest, and dataset/provenance metadata are stored. Display downsampling never
replaces that authoritative grid. Generated signed meshes include their sampled
grid geometry and source hash as usual, so the cube source need not accompany a
saved `.eigenvista`. Preview f32 overflow can prevent rendering without preventing
import/save of valid f64 data. See [cube import](cube-import.md).

The ZIP contains `manifest.json` first and `arrays/aN/0.bin` entries. Numeric array leaves in the document are replaced by `{ "$array": "aN" }` references. Descriptors record shape, entry, byte count, SHA-256, and either `f64` or `u32`. Values are little-endian and row-major; grids are x-fastest. Authoritative Rust float arrays remain f64. Mesh positions are f64 containers for the f32 interpolated result, with that precision limitation recorded explicitly.

Small scalar metadata is JSON. Basis radial weights already include normalization; no automatic normalization is applied. Orbital energies are optional hartree values, not fabricated zeros. Occupations are explicit. A full row-major density matrix identifies its kind independently of orbitals. Current object relationships use stable IDs within a single geometry/basis document.

Saved surface records contain source field ID and scientific hash, signed isovalue, algorithm/version, grid origin/step vectors/resolution, precision, geometry/normals/indices, sRGB color, opacity, and visibility. Quantum inputs remain in the same document. A source mismatch is rejected in this preview; the later revision-aware model should preserve historical assets with their original inputs.

Loading verifies ZIP paths, duplicate entries, supported compression, declared inflated size, reference coverage, shape products, byte lengths, digests, finite arrays, IDs, indices, and scientific dimensions. Entry count, file size, inflated size, grid size, and mesh size are bounded. The preview refuses unreferenced archive entries and unsupported required profiles. Original source attachments, arbitrary extensions, multiple geometry revisions, chunk streaming, and migrations are not implemented.

`eigenvista-format::decode` preserves the validated document exactly. The shared
import layer used by Open and conversion subsequently supplements missing
display bonds from atom positions, retaining every existing edge and its order.
This is reported as an unsaved change, never silently written back to the
source. Scientific arrays, grids, and cached surface records remain unchanged;
bonds are not part of a field's scientific hash.

Native saves validate the encoded document, write and sync a newly created sibling file, then rename it into place. Failure leaves the original file intact. Windows overwrite semantics still need a dedicated implementation/test before a Windows release. Browser saves produce the same container through a download.

Source identity uses SHA-256 over serde_json's explicitly sorted-key encoding of the basis and selected source record. Floating-point negative zero is canonicalized to positive zero for this hash because JavaScript JSON transport folds their signs; this does not change the represented real field. The native binary format retains f64 bit patterns, but the current browser JSON bridge does not retain the sign of zero when saving. Numeric tests and browser roundtrips compare against the JSON-canonical input. This is a local preview contract, not a published cross-language canonical-JSON standard. Pinning and documenting a formal canonical representation is an M1 gate.
