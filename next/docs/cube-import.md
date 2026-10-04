# Gaussian cube import

The shared Rust importer reads one bounded scalar field from `.cube` or `.cub`.
Open, Convert files, and the standalone `eigenvista-convert` CLI use this same
profile. A cube supplies an already sampled field: no basis, orbital coefficients,
or density matrix is needed for volume rendering or isosurface extraction.
This is not a Gaussian-log reader or a wavefunction reconstruction tool.

## Application workflow

1. Open the cube. Atom positions produce automatic display bonds, and the scalar
   grid prepares a bounded display sample without extracting any meshes.
2. The initial mode is **Volume preview**. **Sampled raycast preview** is also
   ready; change the isovalue or appearance as needed.
3. Choose a grid resolution, then **Generate surfaces** for positive and negative
   signed meshes. Empty levels are omitted; a nonnegative field may produce
   only a positive mesh.
4. Save a `.eigenvista` document to retain the original grid, atoms, bonds, generated
   geometry, isovalues, colors, and provenance together. The cube is not overwritten.

When the default isovalue `0.08` is at least the field's peak absolute magnitude,
import chooses `0.1 * peak` if that gives a positive level below the peak.
This is an initial display choice recorded in provenance, not normalization
of the source values. All-zero fields retain the default and have no nonzero
isosurface. Scalar units remain unspecified; consult the producer's calculation.

Opening a native document whose selected field is a stored grid prepares the
preview again automatically. Existing cached surfaces start in mesh mode and
remain available without regeneration. Selecting a different stored grid also
prepares it. Orbital/density fields without stored grids still require Generate
surfaces to create a transient display grid.

The tester ZIP includes `Examples/signed-affine.cube`, a first-party mathematical
fixture with 2 hydrogen atoms, 1 inferred bond, and a signed skewed 7 x 7 x 7 grid.
It is not a calculated molecular orbital. Its formula and provenance are in the
[fixture guide](../fixtures/cube/README.md).

## Accepted text profile

- Two comment lines, followed by atom count and a three-component origin. The
  optional `NVAL` must be one.
- Three positive axis counts and their full three-component step vectors.
  Coordinates and steps use **bohr**. Skewed and reflected nonsingular grids
  are valid; axes need not align with Cartesian directions.
- Atom rows contain atomic number, finite charge value, and three coordinates.
  Elements 1 through 118 are accepted. The charge column is validated but not
  stored as a separate charge property. Zero-atom scalar grids are allowed.
- A negative atom count denotes the standard orbital/dataset header, not a unit
  change. The absolute value determines the number of atom rows. The following
  dataset count must be exactly one, followed by one integer ID. That ID is
  retained in the grid label and provenance, not interpreted as orbital coefficients.
- Exactly one finite scalar per lattice point; ordinary and Fortran `D`/`d`
  exponents are accepted. Missing or additional samples are errors.

Multiple datasets, non-unit `NVAL`, negative axis counts/alternative coordinate
units, ghost atoms, and singular grids are unsupported. Export one field per
cube with the supported units instead of removing format markers. The profile
follows the conventional organization described by the
[VMD cube reader documentation](https://www.ks.uiuc.edu/Research/vmd/plugins/molfile/cubeplugin.html),
with the explicit restrictions above.

The origin and axes retain their affine meaning:
`position(i,j,k) = origin + i*axis[0] + j*axis[1] + k*axis[2]`.
Cube text is read z-fastest and reordered to the model's x-fastest array
`(k * ny + j) * nx + i`. Neither dimensions nor coordinates are inferred from
the field's appearance. Scalar meaning remains `unknown scalar`, even for an
orbital-labelled dataset.

## Preservation and precision

The authoritative imported grid uses f64 values and is retained in `.eigenvista`.
Display sampling uses at most the selected resolution per axis, preserving
the affine extent and applying trilinear downsampling where needed. Small
grids are not upsampled. Changing preview resolution never rewrites the original
array or regenerates existing saved meshes automatically.

Current GPU textures and meshing use f32. Values whose magnitude overflows the
finite f32 range cause a preview/meshing error instead of being silently clamped. Open
still retains the valid imported document, its bonds, and any cached meshes;
it remains savable with the original f64 samples. This is distinct from a
malformed-file error, which leaves the previous document open.

Generated meshes carry field/source identity, signed isovalue, mesher version,
generation grid geometry, precision, and material settings. They survive save,
copy, and reopen without access to the cube source. Full original grid data
also survives, so sampled previews can be rebuilt. Camera, active render mode,
display resolution, and transient display samples are not persisted.
See the [native format](preview-format.md) and [architecture](architecture.md).

The sampled raycaster may miss tangencies or crossings between steps; volume
integration and mixed-scene transparency have known limitations. Classic
marching cubes is not topology-certified. A finer display grid cannot recover
detail missing from the original cube. These remain preview renderers, not
publication-quality or quantified-accuracy guarantees.

## Bonds and conversion

Cube atoms use the same exact-neighbor-search display-connectivity algorithm
as the other supported imports. Bonds are inferred from positions and covalent
radii, not from scalar-field values. See [structure imports](structure-imports.md)
for the heuristic and bounds.

The CLI accepts cube inputs using the options in [Molden import](molden-import.md):

```sh
eigenvista-convert calculation.cube --output calculation.eigenvista
eigenvista-convert --check --json calculation.cube
```

Conversion preserves the scalar grid and inferred bonds but does not generate
meshes. The same shared native-import path fills missing bonds in older
`.eigenvista` and legacy `.molekel` preview files while retaining explicit bonds
and cached scientific data. The [native profile](preview-format.md) describes
the accepted old identifiers and the EigenVista identifiers written by new saves.
Additions are reported as requiring Save; the original file is never changed
by parsing. Raw native decoding remains exact and separate from this import policy.

## Limits and regression evidence

Cube text is limited to 64 MiB, absolute atom count to 100,000, and total grid
samples to `128^3`. Every dimension must be at least two. Finite-number,
checked-allocation, affine-grid, model, and bond-search limits also apply.
These are rejection bounds, not interactive-performance promises.

[Cube core tests](../crates/eigenvista-core/tests/cube_import.rs) exercise ordering,
affine grids, signed fields, bonds, single-orbital headers, malformed profiles,
sampling, and meshing. They read the existing
`data/h2o-dens.cube`, `all_data/Benzene.MO19-BOTH-SIGNS.cube`, and
`all_data/molden_test/test_homo.cube` without changing them. Shared-import/CLI
tests cover exact native grid persistence, conversion, and missing-bond backfill. The analytic fixture
provides independent expected samples for browser rendering and persistence
checks. See [status](status.md) for tests actually run, separately from native
dialog and clean-install qualification.
