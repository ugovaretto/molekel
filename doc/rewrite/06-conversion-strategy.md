# Molekel file conversion strategy

The application should consume a validated Molekel document, whether loaded from `.molekel` or produced in memory by an importer. Converters own source-specific conventions. They can be built and released after the core viewer without spreading those conventions through rendering and numerical code.

The initial format design is in [the native format proposal](05-molekel-format.md). The conversion order below uses the flexibility granted in the user's follow-up while retaining the original v1 compatibility target.

## Delivery order

| Stage | Inputs | Purpose |
| --- | --- | --- |
| A | Small authored native fixtures and independent reference exports | Develop the document reader, evaluator, and renderer before broad import support |
| B | XYZ, PDB, cube, OBJ, atom-color text | Restore structure and precomputed-field workflows through simple adapters |
| C | Molden Gaussian-basis profile | Restore direct orbital/density evaluation from a commonly exchanged format |
| D | ORCA JSON and ORCA-produced Molden | Add producer-aware conversion with explicit convention tests |
| E | Gaussian formatted checkpoints and supported logs; other cclib formats | Extend coverage based on available scientific content |

Stages B and C remain part of full v1 compatibility. A native-only preview can precede them. ORCA and Gaussian adapters are later independent deliverables, not requirements for the first working viewer. No converter has been implemented in this research task.

## Converter contract

Each converter accepts source bytes or a bundle of related files plus explicit selection options. It returns a validated document and a structured report. GUI import, a future command-line converter, and a possible remote conversion service use this same logical contract.

The report records detected format/producer, selected job and geometry, preserved quantities, unit changes, AO reorderings and normalization transformations, source precision, unsupported sections, and unavailable features. It must distinguish "not present in source" from "converter does not support it" and "present but invalid".

Run detection, bounded parsing, source-model validation, convention conversion, canonical-model validation, and native writing as separate stages. Errors carry source locations when available. Do not publish a partially written native file as a completed conversion.

Use TypeScript/Rust adapters for small browser-friendly inputs and Python adapters for complex chemistry outputs where existing scientific readers provide value. Prefer PySCF or cclib over writing every log parser anew. A converter can be a separate tool; the viewer need not bundle its runtime or vendor utilities.

## Input policies

### PDB

Use the [wwPDB coordinate specification](https://www.wwpdb.org/documentation/file-format-content/format33/sect9.html) and a tested existing parser. Preserve fixed-column atom identity, residues, alternate locations, occupancy, elements, and model boundaries. Do not infer calcium from a protein atom named CA when its element is carbon.

Proposed initial behavior: open the first model and provide model selection; choose a consistent alternate conformation by a documented occupancy/tie policy while preserving alternatives. Respect explicit connectivity and mark inferred bonds separately. PDB provides structure, not orbital coefficients. mmCIF is a sensible later extension, but does not replace the explicitly requested PDB support.

### XYZ

Support conventional atom-count/comment/coordinate blocks, element symbols or a documented atomic-number variant, and concatenated frames. Default conventional XYZ coordinates to angstroms and record that assumption. Extended XYZ properties require a named adapter profile; never guess lattice or quantum semantics from arbitrary extra columns.

Use a tested geometry parser where available. Apply the shared bond-inference policy only if the user requests bonds and explicit data are absent. Preserve atom order across compatible frames.

### Molden

The [Molden specification](https://www.theochem.ru.nl/molden/molden_format.html) defines geometry, GTO/STO sections, orbitals, occupations, and component conventions. The first converter supports real Gaussian data through G shells. Preserve Cartesian/spherical distinctions and mixed shell flags; split SP contractions into explicit components. Handle Fortran exponent notation and validate AO indices/counts.

Producer identification matters because files can differ in normalization and conventions. [PySCF's Molden implementation](https://pyscf.org/_modules/pyscf/tools/molden.html) is a useful reader/writer reference, but any high-angular-momentum filtering or normalization behavior must be accounted for. Neither filename nor successful parsing establishes correct wavefunction interpretation.

If basis data or occupations are absent, offer only the operations supported by what was read. STO data can be preserved with an unsupported-evaluator marker; it must not be interpreted as GTO data. Verify both source order and final explicit AO definitions against independently generated values.

### Gaussian cube

Use primary implementation documentation from [VMD's cube plugin](https://www.ks.uiuc.edu/Research/vmd/plugins/molfile/cubeplugin.html) and [PySCF's cube reader/writer](https://pyscf.org/_modules/pyscf/tools/cubegen.html), supplemented by the clearly nonofficial [h5cube description](https://h5cube-spec.readthedocs.io/en/latest/cubeformat.html).

Parse comments, origin, full axis vectors, atoms, channel count/identifiers, and the expected scalar count. Support negative atom-count multi-orbital files and documented optional per-voxel value counts. Convert source order to native x-fastest arrays. Check nonfinite values, truncated data, inconsistent counts, singular grids, and dimension overflow.

Unit conventions in cube-related tools are not perfectly uniform. Use atomic units for the standard profile; support alternate signed-count/angstrom conventions only through a named, tested producer profile or explicit user choice. Do not apply a universal "negative means angstrom" rule to all files or confuse cubegen input controls with output-file semantics. Preserve the original header and conversion decision.

Imported cube values are authoritative samples. The converter cannot reconstruct basis functions, density matrices, or orbital coefficients from them. Unknown scalar meaning remains unknown even when a title resembles "density".

### OBJ and metadata

Use Three.js [OBJLoader](https://threejs.org/docs/pages/OBJLoader.html) for the supported polygon-mesh subset and validate indices, finite positions, group boundaries, and normals. Triangulate supported faces using the loader's behavior; verify concave polygons in fixtures. Unsupported freeform curves/surfaces require conversion or an explicit diagnostic.

OBJ has no dependable physical-unit convention. Ask for or import declared units before overlaying it with a molecule. Preserve object/group names, optional MTL material assignments, and a scene transform. Add user-editable description, source, scalar-property associations, and molecule identity in the native mesh record.

References to MTL or texture resources are resolved only from user-selected companion files or explicitly permitted locations. An OBJ import must not trigger arbitrary network fetches. Its geometry alone does not authorize a scientific field label.

### Atom-color text

Support the existing convention of one `R G B` line per atomic number with channels in [0,1]. Validate each line before applying any change. Treat absent later elements as defaults, and reject ambiguous malformed rows with line numbers. Add an optional explicit symbol-based format as a separately detected profile; do not reinterpret positional rows when comments or blank lines appear.

Store the resolved override table in the native document so reopening does not depend on the original text file. Retain the file hash/name as provenance.

## ORCA conversion

Prefer ORCA's documented [orca_2json export](https://www.faccts.de/docs/orca/6.1/manual/contents/utilitiesvisualization/orca_2json.html) when available. The manual describes geometry, basis, and molecular-orbital exchange, and explains that density export requires the density file and appropriate configuration. Its real-solid-harmonic conventions need an explicit adapter.

The alternative [orca_2mkl route](https://www.faccts.de/docs/orca/6.1/manual/contents/utilitiesvisualization/utilities.html) can produce Molden from `.gbw` and related orbital files. Test that route by ORCA version and orbital type. Keep selected state and spin identity through conversion.

Do not parse proprietary binary `.gbw` by guessed offsets. Treat vendor utilities as user-supplied tools or accept their exported data. Their availability, invocation, and redistribution conditions need separate packaging decisions. A text `.out` file with missing coefficients cannot provide the same result as a complete wavefunction export.

## Gaussian and general log conversion

Prefer a complete structured wavefunction export, such as a formatted checkpoint, over text intended for human reading. cclib is a candidate for supported Gaussian and other outputs. Its [attribute table](https://cclib.github.io/data.html) includes basis and MO-related quantities, but support depends on the parser and what the source printed. Its [data notes](https://cclib.github.io/data_notes.html) also show that AO labels are not a universal ordering contract.

For logs, select a specific job, geometry, and electronic state. Verify that coordinates, basis, occupations, and coefficients belong together. Concatenated optimization and frequency jobs must not combine the last coordinates with an unrelated earlier coefficient table.

A conversion with only coordinates and energies creates a structure document and reports missing wavefunction information. Do not invent coefficients or recover them from orbital eigenvalues. If a cube is available, attach it as a sampled field with independent provenance.

## Loss and success policy

| Result | Behavior |
| --- | --- |
| Complete supported wavefunction | Enable supported orbital and density operations after validation |
| Geometry plus incomplete quantum metadata | Show geometry and metadata; disable unavailable computations with the specific reason |
| Valid sampled field | Enable grid surfaces, volume rendering, and sampled raycasting; no claim of analytic basis evaluation |
| Unsupported basis convention | Preserve source information; withhold scientific evaluation |
| Invalid or contradictory scientific arrays | Reject affected entity; do not silently repair coefficients |
| Cosmetic metadata not mapped | Preserve it in a namespaced extension or report it as omitted |

Native save/reopen must preserve canonical arrays exactly. Conversion from a low-precision text file cannot improve its information content; store that limitation so numerical tolerances are appropriate. Independent converters should agree on physical fields after allowed whole-orbital sign alignment, not necessarily on byte-identical source labels.
