# Molekel documentation and source review

The main migration cost is separating scientific data and calculations from three overlapping object models: OpenBabel molecules, legacy Molekel quantum chemistry structures, and the Coin/OpenMOIV/VTK rendering objects. A rewrite should preserve scientific meaning and useful workflows while replacing that ownership model.

Review baseline and scope are in the [index](README.md). Findings below are from static inspection, with no assertion that a current macOS build was attempted.

## Findings that affect the rewrite

### High priority

**The atom color loader writes past its array.** In `src/MolekelMolecule.cpp:3435`, `colors` has 104 entries, indexed 0 through 103. The loop increments `cnt` before writing and accepts 104 input lines, so the last write at line 3445 addresses entry 104. The supplied `data/dafault_atom_colors` has 104 lines. Replace the parser and retain the intended line-number-to-atomic-number convention. Reject invalid channels and avoid a fixed table size tied to this old implementation.

**Molden F-shell interpretation can change the wavefunction.** `src/old/readmolden.cpp:685` initializes ten Cartesian F components, but line 695 unconditionally changes this to seven. The detection at line 691 recognizes only a narrow flag case. This cannot correctly cover all of Molden's mixed Cartesian/spherical conventions. The [Molden specification](https://www.theochem.ru.nl/molden/molden_format.html) distinguishes these cases. Preserve explicit component ordering and normalization in the new data model and test each supported variant.

**Malformed Molden input can index outside allocated objects.** `src/old/readmolden.cpp:710` indexes atoms from the file without a preceding range check; `read_coefficients`, starting at line 836, also uses coefficient indices directly. Fixed character buffers and unchecked scans add further input risks. A converter needs bounded parsing and a complete validation pass before publishing a document.

**Fractional occupations are not respected by the coefficient-derived density path.** `src/old/calcdens.cpp:791`, especially the `USE_COEFFS` branch, builds density from integer `nAlpha`/`nBeta` loops and an implicit factor of two. It does not weight every orbital by its stored occupation. `src/old/readmolden.cpp` also converts summed occupations to integer counts. Store occupations as explicit real values and evaluate their actual weighted sum. This finding concerns that specific path, not every historical density source.

**Cube reading can append a value after failed extraction and still report success.** `src/utility/OBGaussianCubeFormat.cpp:137` checks the stream before extraction, appends at line 141 without checking extraction, and returns true at line 149. Depending on the failure, the appended value can be invalid. There is no exact scalar-count check. Negative atom-count dataset identifiers are skipped at lines 125-130, without a corresponding multi-channel data model. Validate the expected number of values and preserve channel identifiers.

### Medium priority

**General grid axes are lost.** `src/MolekelMolecule.cpp:1276`, `GridDataToVtkImageData`, keeps only `xAxis[0]`, `yAxis[1]`, and `zAxis[2]`. Rotated or skewed grids cannot be represented correctly by that conversion. The published file-format guide explicitly limits cube support to orthogonal bases. Use a complete index-to-world matrix in every new field and renderer path.

**Grid subsampling dimensions disagree with the loops.** The same conversion sets dimensions to `n / stepMultiplier`, while stepping from zero to less than `n` visits `ceil(n / stepMultiplier)` samples. For example, five samples with a stride of two require three output samples, not two. Make sample counts and cell counts separate concepts in the replacement.

**Cancellation and concurrent work cannot safely share the legacy calculator.** `src/old/calcdens.cpp:213` onward holds global `density`, `chi`, and `molOrb`, plus a non-atomic stop flag and shared range/type state. The `stopped` label remains inside the outer loop in `vtk_process_calc`, so it does not immediately exit the whole calculation. The local [threading notes](../developer/build/multithreading.txt) already document termination and ownership problems. Use job-local state and cooperative cancellation with no partially completed result marked valid.

**Scalar extrema start incorrectly for all-negative fields.** `src/old/calcdens.cpp:269` initializes the maximum with `numeric_limits<double>::min()`, a small positive value. An entirely negative field cannot update that maximum correctly. Initialize with negative infinity or the first valid sample.

**The graphics bridge depends on legacy OpenGL behavior.** `src/utility/vtkSoMapper.h` combines `SoGLRenderAction` with matrix-stack calls around lines 280-305; `src/utility/vtkGLSLShaderActor.cpp` derives behavior from VTK OpenGL actors. Changing the build's graphics library would not translate these render paths to Vulkan or Metal.

These are migration findings. Fixes to the legacy application were deliberately not made, following the request to produce documents before writing code.

## Feature map

| Requested feature | Existing implementation evidence | Rewrite treatment |
| --- | --- | --- |
| Ball and stick | `MainWindow.cpp`, `QuickDisplayBStickSlot`; OpenMOIV `DISPLAY_BALLSTICK` | Preserve behavior; replace rendering objects |
| Liquorice | `MainWindow.cpp:3225`, `DISPLAY_STICK` with round-cap cylinders | Preserve rounded bonds and element coloring |
| Van der Waals spheres | `QuickDisplaySpacefillSlot`, `DISPLAY_CPK` | Use a named, versioned radius table |
| Orbitals from basis | `old/readmolden.cpp`, `old/calcdens.cpp:607`, `MolekelMolecule.cpp:983` | Use as comparison material; establish independent numerical reference |
| Density matrix surface | `calculate_density`, `generate_density_matrix`, `GenerateElectronDensityData` | Support explicit matrices and occupations with stated semantics |
| Cube surfaces | `utility/OBGaussianCubeFormat.cpp`, `GenerateGridDataSurface` | Replace parser and affine-grid handling |
| Adjustable isovalue | `GenerateIsoSurfaceActor` at `MolekelMolecule.cpp:927` and grid surface path around 1379 | Preserve and decouple from the sampled field cache |
| Marching cubes | `vtkMarchingCubes` calls around lines 930 and 1401 | Reuse an independently tested modern extractor |
| Color and transparency | VTK properties, molecule and surface widgets, depth-peeling support | Preserve per-object and per-sign settings |
| Atom color file | `SetAtomColors` at line 3427; sample text file | Preserve format compatibility, repair indexing and validation |
| PDB and XYZ | `MolekelMolecule::New` around lines 202-277 uses OpenBabel; PDB importer adds another path | Normalize through one importer boundary |
| Molden | `OBMoldenFormat` reads geometry; `old/readmolden.cpp` supplies quantum data | One validated scientific representation |
| OBJ plus metadata | No implementation found by targeted search | New requirement |
| Shrinkwrap | No implementation found by targeted search | New requirement |
| Direct volume rendering | No `vtkVolume` path found in reviewed source | New requirement |
| Shader field raycasting | No corresponding raycast/raymarch path found in reviewed source/shaders | New requirement; existing GLSL mostly shades geometry |
| Native scientific document | No matching `.molekel` contract found | New requirement from the user's follow-up |

Search absence is evidence about this checkout, not proof about every historical branch or external dependency. `src/molekel_sources.cmake` selects `old/readmolden.cpp`; the nearby `.cxx` file is not the active reader in that source list.

## Documentation reviewed

The hosted [manual introduction](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/Introduction.html), surfaces, electron density, grid data, file formats, and build pages were read. The local `gh-pages` tree was inspected, including the full Display Style page. Local build and multithreading notes and the test-case specification were also read.

The [electron-density guide](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/ElectronDensity.html) establishes workflows worth retaining: selecting an orbital, positive and negative surfaces, a configurable box and step size, cancellation/progress in the UI, and changing appearance after generation. The new viewer should expose the selected field and its units separately from appearance.

The [grid guide](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/GridData.html) documents per-grid styles, subsampling, and Laplacian smoothing. It describes loading multiple copies to display multiple cube isovalues. The replacement should attach several surface objects to one field instead.

The [build guide](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/Main/Build.html) and `src/CMakeLists.txt` describe Qt 4, VTK 5, OpenBabel 2, Coin/OpenInventor, OpenMOIV, GLEW, and Qwt. Platform instructions target much older operating systems. This establishes dependency age; it does not establish the outcome of a modern build.

The archived video material includes Flash files. Those videos were not executed. Their presence is not treated as verification of behavior.

## Assets to retain

| Asset | Use | Limitation |
| --- | --- | --- |
| `data/molden.input` | Existing atom/basis/orbital input; begins with atomic-unit coordinates | Inspect and validate its producer conventions before declaring numerical truth |
| `data/3POR.pdb`, `data/guanine.pdb` | Molecular geometry and representation checks | Add explicit alt-location and connectivity expectations |
| `data/alaninemulti.pdb` | Multiple-model handling | A frame selector is enough initially; animation is separate |
| `data/bik.xyz` | Basic coordinate import | Does not supply quantum data or reliable bond orders |
| `data/h2o-dens.cube` | Small 40 x 40 x 40 field | Validate its meaning and coordinate units from provenance |
| `all_data/Benzene.MO19-BOTH-SIGNS.cube` | ORCA-generated signed orbital field with negative atom count | Useful for channel metadata and signed lobes |
| `test/test_cases/test.txt` | User workflows and image scenarios | Mostly UI-oriented tests, not a numerical verification suite |
| `test/test_cases/reference_output/` | Historical visual references | Different rendering algorithms need semantic comparisons, not exact old pixels |
| `data/dafault_atom_colors` | Compatibility fixture for legacy RGB text | Preserve spelling as an existing filename; do not copy its parser |

Retain source and scientific attribution. File headers show GPL-2.0-or-later for substantial application code and LGPL-3.0-or-later for some utilities. These are observed notices, not a completed dependency-license audit. Record the origin and license of any copied code before redistribution; choosing a new language does not settle reuse rights.

## Review limits

The focus was the features in the brief. Trajectory animation, vibrational spectra, SAS/SES generation, printing, and every historical importer were not audited end to end. The old numerical code is useful to explain behavior, but modern independent fixtures must decide correctness when implementations disagree.
