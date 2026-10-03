# Molden import and conversion

The shared Rust importer supports a bounded **real Gaussian Molden wavefunction
profile**, not every historical producer variant. The application and command-line
converter call the same library. Native `.molekel` remains the self-contained
format for saving scientific data, appearance, and generated surface meshes.

See the [user guide](user-guide.md) for opening and converting files, and the
[development guide](development.md) for building the converter. This document
records the scientific and API contract.

## Command-line conversion

The tester ZIP includes `Tools/molekel-convert`; source builds produce
`next/target/release/molekel-convert` with the release command in the development
guide. The examples below assume the executable is on your PATH. Output
directories must already exist.

```sh
molekel-convert calculation.molden
molekel-convert calculation.molden.input --output calculation.molekel
molekel-convert --output-dir converted water.molden hydroxyl.molden
molekel-convert --check --json calculation.molden
molekel-convert --help
```

A single input defaults to a sibling `.molekel` filename; `.molden.input`
becomes `.molekel`. Multiple inputs require `--output-dir`. `--output` (or `-o`)
selects one destination. `--check` validates and reports without writing.
`--json` emits an array on standard output containing each input/output, status,
format/warnings/`requires_save`, atom/orbital/density counts, and error. Without it, summaries
and warnings go to standard error. Use `--` before filenames starting with `-`.

Existing outputs require explicit `--force`; input paths are never valid
destinations, even with that option. The converter preflights destinations,
rejects output symlinks and colliding paths, validates the encoded native data,
and publishes complete files atomically. Initially absent outputs remain
no-clobber even with `--force`: a filesystem alias or concurrently created file
cannot replace an earlier batch result. Such late collisions report failure
and retain completed outputs. No-clobber publication requires hard-link support
on the destination filesystem; failure does not fall back to unsafe replacement.
Windows replacement behavior still requires platform qualification.

Exit codes are `0` for complete success, `1` for any per-file import/write
failure, and `2` for invalid options or destination/input-path preflight errors.
All paths must resolve at preflight; after that, batch import failures do not
discard successful outputs. At most 256 inputs are processed sequentially,
with bounded regular-file reads. No GUI, Python, ORCA installation, or subprocess
converter is needed. CLI output contains scientific data but no newly generated
surfaces; open it in the app to generate and save meshes. Existing native meshes
survive native-to-native conversion. The same commands accept supported XYZ,
PDB, and [Gaussian cube](cube-import.md) files, not only Molden.

## Shared boundary

[`molekel-import`](../crates/molekel-import/src/lib.rs) exposes:

```rust
pub const MAX_IMPORT_BYTES: usize = 128 * 1024 * 1024;

pub fn import_bytes(bytes: &[u8], name: &str)
    -> Result<ImportResult, String>;

pub struct ImportResult {
    pub document: molekel_core::Document,
    pub report: ImportReport,
}

pub struct ImportReport {
    pub format: String,
    pub warnings: Vec<String>,
    pub requires_save: bool,
}
```

The result/report implement Serde serialization/deserialization. `format` is
one of `molekel`, `molden`, `xyz`, `pdb`, or `cube`. Every successful document
passes core validation before returning. Errors contain a Molden line number
where applicable. There is no filesystem, subprocess, network, or external QC
program requirement in the library; it builds natively and as WASM.

ZIP magic routes through the existing bounded native decoder. A text
`[Molden Format]` header, case-insensitive and allowing a UTF-8 BOM/leading blank
lines, takes precedence over a misleading extension. This includes names such
as `molden.input` and `.molden.input`. `.molden`, `.mold`, and `.molden_input`
also select the Molden parser, which still requires its header. Existing
XYZ/PDB/ENT/Cube/CUB extensions use the core's existing import profiles. There
is no generic Gaussian-log, ORCA-binary, T41, or other content detector.

Native import supplements missing display bonds while retaining existing
connections, scientific data, and cached meshes. Additions are reported and
recorded in provenance; otherwise the decoded document is unchanged. This is
an import policy, not a change to exact native format decoding.
`requires_save` is true for external imports and native documents with added
bonds; the UI uses it for the unsaved indicator. It defaults to false when
deserializing older reports that omit the field. No input is rewritten on Open.
External import records the original byte SHA-256, input name, importer version,
scientific profile, connectivity method, and warnings in document provenance.
The import report is also returned separately for the UI/CLI. The original file
is never written or modified by parsing. Saving/CLI output is a separate step
using the validated native format and its filesystem adapter.

## Canonical Gaussian profile

The input must contain one `[Atoms]`, one `[GTO]`, and at least one `[MO]` section.
Multiple MO sections append orbitals in source order; repeated atom/basis
sections are rejected as ambiguous. Geometry-only or vibration-only Molden is
not supported in this increment.

### Atoms and units

- `[Atoms] AU` and `[Atoms] Angs`, including parenthesized unit names, are
  accepted. Missing units are rejected. Coordinates become bohr in the model.
- Real elements 1 through 118 are accepted. Element symbols are checked
  case-insensitively against the atomic number; trailing numeric atom labels
  are allowed. Ghost atoms and effective nuclear-charge substitutions are not.
- Atom IDs must be unique and positive. GTO center references resolve these
  IDs explicitly. The atom label/ID itself is not retained as structured metadata.
- Automatic coordinate-based display bonds use the same exact neighbor-search
  implementation as the other supported formats. They are not inferred bond
  orders or aromaticity.

### Shells and component conventions

Gaussian S, P, D, F, G, and combined SP shells are supported. Each shell is a
single contraction; generalized contractions can be represented as separate
shells. SP uses distinct S/P coefficient columns. Extra generalized-coefficient
columns, STO, H and higher shells, nonpositive exponents, and nonfinite values
fail explicitly. Fortran `D`/`d` exponents are accepted.

The optional GTO center suffix must be zero; the optional shell scale must be
one. A non-unit shell scale is rejected because producers disagree about its
meaning; export explicit contraction coefficients instead.

Default D/F/G counts are Cartesian 6/10/15. `[5D]` changes D and the default F
mode to spherical; an explicit F flag takes precedence over that implied default.
`[7F]`, `[10F]`, `[6D]`, `[9G]`, `[15G]`, `[5D7F]`, `[5D10F]`, `[6D7F]`, and
`[6D10F]` are supported. Contradictory explicit flags and unknown numeric angular
flags are rejected. A convention flag is a standalone section without data.

AO order is the declared Molden order, including real spherical `m=0,+1,-1,...`
and the special Cartesian D/F/G component sequences. Spherical functions become
explicit real solid-harmonic polynomial terms through degree four; the evaluator
does not have to guess a producer convention at runtime.

Each primitive is normalized for its exact polynomial, then the contraction is
normalized analytically. The AO's radial coefficients absorb both factors, as
required by the core's explicit-polynomial model. For normalized same-angular-
degree primitives, the contraction norm uses
`sum(i,j) c[i] c[j] (2 sqrt(a[i] a[j]) / (a[i] + a[j]))^(l + 3/2)`.
Non-unit contraction norms beyond rounding tolerance generate a warning. Zero,
numerically unstable, overflowing, or underflowing normalization is rejected.
MO coefficients remain associated with those normalized AOs in source order.

These rules follow the [Molden format specification](https://www.theochem.ru.nl/molden/molden_format.html).
The [PySCF writer](https://pyscf.org/_modules/pyscf/tools/molden.html) supplies an
independent, exercised producer whose Cartesian output rescales MO coefficients
for normalized Cartesian AOs. A standard-looking file from an unqualified
producer is not automatically proof of compatible normalization.

### Orbitals and densities

Real AO coefficients, symmetry labels, energies, spin labels, and occupations
are read in file order. Coefficient indices are one-based, checked against the
expanded basis, and may be sparse: omitted entries mean zero. Duplicate indices
and all-zero or empty orbitals are rejected. Symmetry is included in the display
label; energies use hartree. Missing energies and occupations remain missing.
Explicit `Spin=Alpha` or `Spin=Beta` is required.

A single Alpha set is represented as spatial orbitals with occupations up to
two. This is an explicit, reported interpretation of conventional restricted
Molden output, not a deduction of separate spin populations. Mixed Alpha/Beta
sets retain their spin labels and occupations must be at most one. Fractional
occupations are accepted. A beta-only set remains beta-only.

If **all listed orbitals have occupations**, the importer constructs
`P[i,j] = sum(k) occupation[k] C[i,k] C[j,k]`. Mixed-spin inputs additionally
produce alpha, beta, total, and alpha-minus-beta spin matrices. A single spatial
set produces one `total`-kind matrix; beta-only produces one `beta` matrix.
If any occupation is missing, no derived density is made.

These fields are labelled **Occupation-derived ... (listed orbitals)** and
carry a warning. They are not independently supplied correlated, difference,
or transition density matrices. Incomplete orbital exports, ECP core electrons,
and missing off-diagonal density information cannot be recovered. A matrix's
`total` kind describes its contraction use, not a completeness certification.
The importer does not solve SCF or infer missing electrons from atomic numbers.

The initial selected field is the last source-order orbital with positive
occupation, or the first orbital when none qualifies. That is not necessarily
the highest-energy orbital for an unusual export. IDs are `molden-mo-N` and
`molden-density-total/alpha/beta/spin`.

## Identified ORCA export subset

No native ORCA parser or invocation of ORCA is added. An existing `orca_2mkl`
Molden export can be opened directly **within the tested subset below**.

A title/preamble containing `orca_2mkl` selects a separate producer profile.
It supports segmented S, P, and spherical 5D shells. These exports include
primitive normalization in their GTO coefficients: the importer removes the
appropriate Cartesian primitive normalization (`s`, `x`, or `xy`, respectively)
before canonical AO construction and reports the conversion.

SP, Cartesian D, F, and G are deliberately rejected for this producer profile.
ORCA-specific F/G phase changes are documented by
[IOData's importer](https://iodata.readthedocs.io/en/latest/_modules/iodata/formats/molden.html),
but they are not enabled without independent real-export fixtures. An
ORCA-labelled title/preamble without the recognized tool marker is rejected.
Do not remove a producer marker to bypass this safety boundary.

Qualification uses one actual ORCA NH3 S/P/5D export from IOData, compared with
IOData-corrected canonical output evaluated independently by PySCF. This is not
a blanket compatibility claim for every ORCA version or export option. Another
producer with the same title syntax but different conventions requires a new
qualified profile, not guessed normalization or silent MO rescaling.

## Preserved and omitted data

The converted document retains atoms, computed bonds, normalized explicit basis,
real orbitals, optional energies/occupations, occupation-derived densities,
profile/provenance and source checksum. No source molecular calculation file
is embedded; `.molekel` instead contains the parsed scientific arrays needed by
the viewer. Subsequent generated meshes and their metadata persist through
normal Save.

The first title section is used when nonempty, bounded to 1024 characters. Other sections,
including frequencies, normal modes, optimization trajectories, charges, and
core/ECP metadata, are not preserved as structured data. Each ignored section
is named in the report/provenance, with its starting line. An ignored `[CORE]`
section does not recover missing core density. Duplicate sections whose values
would be scientifically ambiguous are rejected instead of picking a geometry.

The existing repository file `data/molden.input` was imported without editing it:
17 atoms, 118 orbitals, and one occupation-derived matrix. Its frequency,
normal-coordinate, convergence, and geometry-trajectory sections are reported
as omitted. This is a real-file smoke test, separate from independent numerical
qualification.

## Limits and verification

The shared byte limit is 128 MiB, with stricter existing XYZ/PDB/Cube limits
still enforced by their parsers. Molden additionally bounds 500,000 lines,
16,384 bytes per line, 128 sections, 100,000 atoms, 256 expanded AOs, 64
primitives per AO, 512 MOs, and 256 bytes per MO header value. Core resource,
relationship, finite-number, and format checks apply afterward. Limits are not
interactive-performance promises. Input is in-memory, not streaming.

The [parser unit tests](../crates/molekel-import/src/tests.rs) exercise content
detection, units, references, SP, sparse coefficients, mixed angular flags,
fractional/spin densities, missing data, reported losses, resource bounds,
malformed input, native preservation, and profile rejection.

The [independent tests](../crates/molekel-import/tests/independent_molden.rs)
and [fixture guide](../fixtures/molden/README.md) cover RHF water, UHF hydroxyl,
Cartesian and spherical multiple contractions through G, and the qualified
real ORCA NH3 export. They compare point values and gradients, use independently
generated overlaps to check MO norms/electron counts, verify fixture checksums,
and exercise exact `.molekel` scientific/mesh roundtrips. Parser validation
does not recompute a full overlap matrix or certify every incoming MO is
orthonormal; explicit input flags and the selected profile remain authoritative.

From the repository root, with repository-local temporary variables set:

```sh
cargo test --manifest-path next/Cargo.toml --locked -p molekel-import
cargo clippy --manifest-path next/Cargo.toml --locked -p molekel-import --all-targets -- -D warnings
```

See [status](status.md) for the broader WASM, browser, native, CLI, and packaging
checks run for the implementation increment. A native parser passing its tests
is not by itself proof of a correct GUI workflow or release installation.
