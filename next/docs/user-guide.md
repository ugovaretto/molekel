# Molekel Preview user guide

## What this program does

Molekel displays molecular structures and quantum-chemistry fields in an
interactive 3D scene. You can open Molden wavefunctions, PDB/XYZ structures, and
Gaussian cube scalar fields, inspect automatically computed bonds, display
supplied orbitals or density matrices, generate signed
isosurfaces, and save the molecular data and surface geometry together in a
portable `.molekel` document.

This is version 0.2.1, a development preview. Scientific calculations and
rendering have targeted tests, but the application is not yet a qualified
scientific release. It visualizes supplied data; it does not run a quantum
chemistry calculation to obtain a wavefunction from atom positions.

## Install and start on a Mac

The tester ZIP is for **Apple Silicon Macs** (M-series processors), not Intel
Macs. It targets macOS 13 or later; that complete OS range has not been tested.
The package's `BUILD-INFO.json` records the actual build system. Linux and
Windows packages are not available yet.

1. Unzip the package and move `Molekel Preview.app` to Applications.
2. Double-click the app. A mathematical hydrogen-pair example opens initially.
3. Keep the rest of the extracted package for this guide, examples, build
   information, license notices, and source code.

You do not need Rust, Node.js, a development server, or an internet connection
for the installed app's local molecular-file workflows.

The tester app is ad-hoc signed, not Developer ID-signed or Apple-notarized.
If macOS blocks it because the developer cannot be verified, only proceed if
you trust its origin and integrity. After trying to open it, use **System
Settings > Privacy & Security > Open Anyway**, then confirm Open for this app.
Do not disable Gatekeeper globally. Stop and report malware or damaged-app
warnings instead of bypassing them. Managed Macs may disallow exceptions.
See [Apple's security instructions](https://support.apple.com/en-us/102445).

## A first session

1. Click **Open** and select `Examples/water.pdb` from the extracted ZIP.
2. The Document panel should show **3 atoms** and **2 bonds**. Bonds are computed
   automatically, including when the file has no explicit connections.
3. Try the **Representation** choices: Ball and stick, Liquorice, and Van der
   Waals. The last uses space-filling spheres and does not draw bond cylinders.
4. Drag in the scene to rotate it; scroll to zoom. Use **Fit scene** in the
   viewport toolbar to restore the framing.
5. Click **Save**, choose a new `.molekel` filename, and reopen it with **Open**.
   Your structure, connectivity, and representation are retained.

`Examples/water.xyz` provides the same three-atom/two-bond check. Preserve your
original inputs: the preview does not retain every source-format annotation.
Use the app's Open button; Finder file associations and drag-and-drop loading
are not implemented.

## Finding your way around

| Area | Purpose |
| --- | --- |
| Top bar | Document title, unsaved-change indicator, Open, Convert files, and Save |
| Document panel | Atom/bond/basis counts, representation, example menu |
| Quantum fields | Select an orbital, supplied density matrix, or sampled field |
| Provenance | Source descriptions, import decisions, and loss notices |
| Central scene | Molecule and surfaces; selected atom/surface information below |
| Field rendering | Rendering mode, isovalue, grid resolution, Generate/Cancel |
| Appearance | Positive/negative colors and opacity |
| Saved surfaces | Current document's meshes, visibility, triangle counts, deletion |
| Footer | Busy state, completion information, or errors |

On a narrow browser window these areas rearrange; scrolling may be needed.
Toolbar icons show their names on hover.

## Moving and inspecting the scene

- Left-drag rotates. Right-drag pans; Ctrl-drag is another pan gesture.
- Scroll or middle-drag zooms. On a touchscreen, one finger rotates and two
  fingers pinch/pan.
- Click an atom to see its element/index and coordinates in angstroms. Click
  a mesh to see its label and triangle count. There is no measurement tool yet.
- **Fit scene** and **Reset view** currently do the same thing: fit all displayed
  objects and restore the default viewing direction.
- **Export image** requests a PNG download of the current viewport, named
  `molekel-view.png`. It uses the browser/WebView download mechanism, not the
  native document Save dialog; download handling varies by host. Browser export
  is the reference path, and native image-download handling still needs qualification.

The scene uses bohr internally. Picked atom positions are converted to angstroms.
Elements outside the small display-color table may appear as `Z` plus their
atomic number and use a fallback color/radius. Bond inference uses a separate,
complete covalent-radius table; display-radius fallbacks do not affect it.

## Supported files

| Extension | What is read | Important boundary |
| --- | --- | --- |
| `.molekel` | Current preview scientific document, appearance, and saved meshes | Missing display bonds are filled in on Open; not a general legacy Molekel project reader |
| `.molden`, `.molf`, `.molden.input` or recognized Molden header | Geometry, Gaussian basis, real orbitals, energies, occupations, and occupation-derived density matrices | Canonical S/P/D/F/G and SP; explicitly identified ORCA exports currently S/P/spherical D only |
| `.xyz` | One conventional atom-count/comment/coordinate frame, in angstroms | Geometry and automatically inferred bonds; multiple frames and extended XYZ are rejected |
| `.pdb` | Selected ATOM/HETATM geometry and explicit plus inferred bonds | First model/structure and one alternate conformer per residue |
| `.cube`, `.cub` | One scalar field, bohr geometry, and computed bonds; includes single-orbital cubes | Multi-field and alternative-unit cube profiles are not supported |

PDB selection keeps common atoms and chooses the alternate with the highest
mean occupancy per residue; zero-occupancy atoms are omitted. The preview does
not preserve chain/residue labels, occupancies, B-factors, formal charges,
isotope identity, cell/symmetry, or secondary-structure information. It does
not expand crystal images. mmCIF and compressed PDB files are not supported.

Bonds are a distance-and-coordination heuristic for display, not bond orders,
aromaticity, hydrogen-bond analysis, or proof of chemical validity. PDB `CONECT`
connections are retained and supplemented with coordinate-based connections.
All supported imports compute bonds automatically, including cube and Molden.
Opening an older `.molekel` file also fills in missing display bonds while
preserving existing connections, scientific data, and cached surfaces. If any
bonds are added, a report appears and the document is marked unsaved; Save
explicitly to retain them. Opening does not overwrite the source file.

OBJ, atom-color text files, direct ORCA output/GBW, and Gaussian logs are **not
implemented**. A PDB or XYZ
file correctly shows **No quantum fields**; coordinates alone cannot supply
orbitals or a density matrix.

## Importing and converting Molden

Use **Open** to select your Molden file, including `molden.input`. Import runs
locally, computes display bonds, and shows a report with any warnings. Select
an orbital or density and generate surfaces normally. Save creates a native
`.molekel` document; the source file is not converted in place. An imported
document is marked unsaved until saved. A failed import leaves the previous
document intact. **Import report** and Provenance retain decisions and losses.

The ZIP includes `Examples/water.molden`, a calculated RHF water wavefunction
with 24 basis functions, 24 spatial orbitals, and a total density. This is a
useful first check for orbital and density surfaces.

**Convert files** opens a separate batch window without replacing your current
scene. Add files, click **Convert**, review each result, then use its **Save**
button. Desktop Save asks for a destination; browser Save requests a download.
Conversion alone writes nothing. Failed rows show individual errors and do not
discard successful results. **Cancel** stops pending work and keeps completed
results. Closing the window preserves its results in memory; **Clear** discards
them. Saved/downloaded bytes are released. Unwritten results do not survive
closing the application. The queue allows 32 files and up to 128 MiB of retained
outputs. Save or clear completed results before processing more.

ORCA's `orca_2mkl` can export Molden, so a direct ORCA reader is not needed for
the supported subset. Preserve its producer title: ORCA uses different primitive
normalization, which the importer recognizes. This increment qualifies only
S/P/spherical D ORCA exports, not ORCA F/G, Cartesian D, SP, or all ORCA versions.
Unsupported cases fail explicitly. Other producer-specific conventions are
not automatically qualified just because a file is called Molden.

Density matrices are derived from listed orbitals and occupations, not recovered
correlated/transition density matrices. Truncated orbital sets can produce only
a partial density; inspect the warnings. Frequency, trajectory, and other
unsupported sections are omitted with notices. Keep original calculation files.
The full profile and command-line workflow are in `next/docs/molden-import.md`
(also `Molden-import.md` in the ZIP). No Python, ORCA, or other calculation
program is required to read a supported Molden file.

## Gaussian cube fields

Use **Open** with a `.cube` or `.cub` file. The atoms and inferred bonds appear
with its scalar field in **Volume preview**, without first generating a mesh.
You can immediately choose **Sampled raycast preview**, adjust the isovalue,
or click **Generate surfaces** to create positive/negative isosurface meshes.
No basis functions or orbital coefficients are needed for these sampled fields.

The ZIP includes `Examples/signed-affine.cube`: a mathematical signed field
with **2 atoms**, **1 bond**, and a skewed 7 x 7 x 7 grid. It is a rendering test,
not a quantum-chemistry calculation. Generate surfaces at its initial isovalue
to see both signed lobes, then Save and reopen the `.molekel` document.

The accepted profile includes standard single-orbital cubes with a negative
atom count and exactly one dataset ID. Multi-field cubes must be exported as
one file per field. Coordinates must use the supported bohr profile; scalar
meaning and units are not guessed from a filename. If the default isovalue is
outside a small field's magnitude range, import selects one tenth of its peak
magnitude as an initial display value. You can change it afterward.

**Grid resolution** bounds the preview grid and mesh calculation. It may
downsample a large input but does not add samples to a small one. The full
original scalar values and affine grid remain in the document and are saved
in `.molekel`, alongside any generated meshes. Convert files and the standalone
converter also accept cube inputs; conversion alone does not generate meshes.
The detailed profile is in `next/docs/cube-import.md` (`Cube-import.md` in the ZIP).

## Orbitals, densities, and isosurfaces

To try the workflow without a quantum-data file, open the flask-shaped example
menu beside Document and choose **Hydrogen pair** or **Fractional open shell**.
These are mathematical demonstrations, not calculated molecular reference
results. Opening an example replaces the current document after an unsaved-change
confirmation. Independently calculated water/OH `.molekel` reference documents
can also be generated from the source tree; see `next/fixtures/pyscf/README.md`.

1. Select a field under Orbitals, Density matrices, or Sampled fields.
2. Set a positive **Isovalue** magnitude. The generator attempts both positive
   and negative levels; only nonempty surfaces are added. A nonnegative density
   can therefore produce only a positive surface.
3. Choose **Grid resolution**, from 24 x 24 x 24 to 48 x 48 x 48. Larger values
   cost more memory and computation and are not an accuracy guarantee.
4. Click **Generate surfaces**. The footer reports completion and triangle count.
   During computation the same button becomes **Cancel**; cancelling retains
   the previous completed meshes.
5. Adjust colors or opacity, then Save to make the result durable on disk.

Selecting a field or changing isovalue/resolution does not regenerate existing
meshes automatically. For imported grids, selecting the field or changing
resolution does prepare its sampled preview. Generate surfaces replaces the meshes for that field;
meshes belonging to other fields remain. Save separate documents to preserve
multiple isovalues of the same field. The isovalue recorded on each saved
surface is its actual generation value, not a later edited control value.

Orbital values use `bohr^-3/2`; density controls show electrons per `bohr^3`.
Imported scalar grids have unspecified scalar units: consult their source.
An isovalue outside the sampled range can yield no surfaces. Reduce the
magnitude or try another field, without assuming that an empty mesh is an error.

## Rendering and appearance

**Isosurface mesh** displays saved triangles and works immediately after
reopening a `.molekel` file, without recalculation.

**Sampled raycast preview** and **Volume preview** use an in-memory display
grid, not direct analytic shader evaluation of a wavefunction. Cube fields and
other grids stored in `.molekel` prepare it automatically on Open or field
selection. A reopened document with cached meshes starts in mesh mode, with
sampled modes also ready. For orbital/density fields without a stored grid,
Generate surfaces prepares it; generate again after switching away or reopening.
The raycast isovalue control updates the sampled preview directly, but previously
generated meshes still require regeneration.

The original cube data is f64, while rendering and current meshing use f32.
If a finite grid overflows the preview's numeric range, an error is shown; its
document, bonds, and existing meshes still open and remain savable. Saving
does not reduce the precision of the authoritative grid to the preview's precision.

In sampled modes, meshes of the selected field are replaced visually by the
sampled display. Meshes from other fields may remain visible. The eye and trash
controls manage saved meshes, not the transient volume; switch to mesh mode
to inspect their effect independently.

Positive/negative colors and opacity currently apply across the document's
surfaces, not just the selected field. Eye icons toggle individual mesh
visibility; trash icons remove meshes from the document. There is no undo.

Raycasting can miss crossings, marching cubes is not topology-certified, and
transparent/volume overlaps have known limitations. These modes are experimental,
not publication-quality guarantees. Shrinkwrap and a transfer-function editor
are not available yet.

## Saving, reopening, and sharing

Save writes a self-contained `.molekel` preview file. It includes structure,
bonds, supplied basis/orbitals/density matrices or imported grids, provenance,
saved mesh geometry and generation metadata, visibility, colors, opacity, and
representation. Copy that file to another matching preview installation to
view its saved meshes without the original input or recomputation. There is
no separate OBJ-plus-metadata export or external mesh linking yet.

On desktop, Save opens a native destination dialog; cancelling leaves the
document unsaved. In a browser, Save requests a download to the browser's
configured location; confirm that the download completed. Save does not export
back to Molden/PDB/XYZ/cube and does not perform automatic background saving.
Use a new `.molekel` filename; replacing an existing native file is an explicit
Save-dialog choice, not an automatic result of Open or Convert.

The **Saved surfaces** heading means surfaces stored in the current document's
memory. They reach disk only when you Save. Camera position, active render
mode, and transient sampling cache are not saved. Each mesh keeps its generation
grid metadata, but the grid-resolution control itself resets on a new app session.

Save before closing or replacing a document. The preview asks before replacing
modified content and registers a browser close warning, but native window-close
protection has not been fully verified. There is no autosave or crash recovery.
Keep original data and backups because the native preview schema is not frozen.

## Problems and limits

| Symptom | What to check |
| --- | --- |
| Generate surfaces is disabled | PDB/XYZ contain no quantum fields; open Molden, a native field document, cube, or example |
| Raycast/volume choices are disabled | For orbitals/densities, Generate surfaces; imported grids prepare automatically unless the footer reports a preview error |
| Cube opens but no field is visible | Check the isovalue and scalar range, Fit scene, and any preview error; an all-zero grid has no nonzero surface |
| Structure seems missing | Use Fit scene; try Ball and stick; check atom counts and error messages |
| Unexpected PDB atom/bond count | Read Provenance; first-model/alternate/occupancy selection and heuristic bonds affect counts |
| Too much work or mesh allocation error | Lower grid resolution; the preview deliberately refuses oversized calculations |
| File rejected | Check format/profile and exact message; do not strip validation metadata or rename unsupported data |
| WebGL2 unavailable | Check graphics acceleration or try another compatible device/browser |
| Graphics context lost | Save if possible, then restart/reload; unsaved data may be lost |

Current guards include a 128 MiB file/inflated-array budget, a 32 MiB PDB text
budget, 100,000 atoms, 600,000 bonds, 256 basis functions, 64 primitives per
basis function, and 128 cubed total imported grid samples. These are rejection
limits, not recommended workloads; large structures can be slow well below them.

For a problem report, include the package filename or `BUILD-INFO.json`, Mac
model/processor, OS version, exact error, steps, and screenshot. Include a small
input only with permission to share it; avoid confidential research data in
public issues. Forward the complete ZIP, including license, notices, and Source,
when sharing the application with another tester.

## Running from source instead

Developer prerequisites are Rust, Node.js/npm, wasm-bindgen CLI 0.2.108, and
platform build tools. After setup in `next/docs/development.md`, run
`npm --prefix next/app run desktop` from the repository root for the native
development app, or `npm --prefix next/app run dev` for the browser viewer at
`http://127.0.0.1:5178`. These are alternatives: the native development command
starts its own server on that port. The installed tester app needs neither.

Other developer documentation is in `next/docs/README.md`; the tester ZIP's
`Source/molekel-source.tar.gz` contains those files with their original paths.
