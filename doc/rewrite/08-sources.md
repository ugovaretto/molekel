# Research sources and evidence limits

Sources below were consulted on 3 October 2026. Links are primary documentation, source repositories, or original research except where explicitly labeled. Rolling documentation describes what was visible during this review; implementation must pin dependency revisions and recheck the capabilities it uses.

## Project evidence

| Source | Used for |
| --- | --- |
| [Project home](https://ugovaretto.github.io/molekel/) | Historical application scope |
| [Manual introduction](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/Introduction.html) | Feature and platform context |
| [Surface guide](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/Surfaces.html) | Field/surface categories |
| [Electron density](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/ElectronDensity.html) | Orbital, box, isovalue, and appearance workflows |
| [Grid data](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/GridData.html) | Grid surfaces, smoothing, multiple-isovalue workaround |
| [File formats](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/ReferenceGuide/FileFormats.html) | Historical format support and cube restriction |
| [Build guide](https://ugovaretto.github.io/molekel/wiki/pmwiki.php/Main/Build.html) | Historical dependencies |
| Local `gh-pages` DisplayStyle page | Representation and positional RGB semantics |
| [Source review](01-legacy-review.md) | File/line evidence and observed migration risks |
| [Legacy test specification](../../test/test_cases/test.txt) | Historical user workflows |

Local source and documentation commits are recorded in [the index](README.md). The current branch's active source list was checked to distinguish `.cpp` readers from nearby old `.cxx` copies.

## Scientific methods and formats

| Source | Used for |
| --- | --- |
| [Molden format](https://www.theochem.ru.nl/molden/molden_format.html) | Sections, units, shell/component distinctions, occupations |
| [PySCF Molden source](https://pyscf.org/_modules/pyscf/tools/molden.html) | Normalization/order handling and converter reference |
| [PySCF cube source](https://pyscf.org/_modules/pyscf/tools/cubegen.html) | Independent orbital/density evaluation and cube implementation |
| [gau2grid](https://github.com/psi4/gau2grid) and [API](https://gau2grid.readthedocs.io/en/latest/py_api.html) | Gaussian collocation and derivatives |
| [gau2grid component order](https://gau2grid.readthedocs.io/en/stable/order.html) | Explicit ordering conventions |
| [MC33 implementation](https://github.com/dvega68/MC33_c_library) and [associated paper](https://jcgt.org/published/0008/03/01/) | Candidate topology-aware mesher; repository documentation was readable |
| [scikit-image marching cubes](https://scikit-image.org/docs/stable/api/skimage.measure.html#skimage.measure.marching_cubes) | Independent Lewiner reference implementation |
| [VTK Flying Edges](https://vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html) | Native/server extraction candidate |
| [Original shrinkwrap paper](https://cspages.ucalgary.ca/~blob/ps/shrinkwrp.pdf) | Projection/adaptation method and topology limitation |
| [GPU Gems volume rendering](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-39-volume-rendering-techniques) | Transfer functions, sampling, and compositing foundations |
| [VMD cube plugin](https://www.ks.uiuc.edu/Research/vmd/plugins/molfile/cubeplugin.html) | Maintainer documentation for cube interpretation |
| [h5cube description](https://h5cube-spec.readthedocs.io/en/latest/cubeformat.html) | Explicitly nonofficial cube convention discussion; supplemented with implementation sources |
| [wwPDB coordinates](https://www.wwpdb.org/documentation/file-format-content/format33/sect9.html) | PDB coordinate and model semantics |
| [ORCA utilities](https://www.faccts.de/docs/orca/6.1/manual/contents/utilitiesvisualization/utilities.html) | Vendor-supported Molden export route |
| [ORCA JSON utility](https://www.faccts.de/docs/orca/6.1/manual/contents/utilitiesvisualization/orca_2json.html) | Structured wavefunction exchange and producer conventions |
| [cclib attributes](https://cclib.github.io/data.html) and [notes](https://cclib.github.io/data_notes.html) | Log-parser coverage, units, and interpretation limits |

The proposed interval-based analytic raycaster and MC-seeded shrinkwrap are engineering designs in this package. They are not claims of an already available Molekel implementation or a direct reproduction of the cited algorithms.

## Frameworks and reuse

| Source | Used for |
| --- | --- |
| [Tauri webview versions](https://v2.tauri.app/reference/webview-versions/) | macOS system-webview dependency |
| [Three.js WebGPURenderer](https://threejs.org/docs/pages/WebGPURenderer.html) | Documented WebGPU/WebGL2 backend behavior |
| [Three.js instancing](https://threejs.org/docs/pages/InstancedMesh.html), [OBJ](https://threejs.org/docs/pages/OBJLoader.html), [volume example](https://threejs.org/examples/webgl_texture3d.html) | Reusable renderer components; example inspected as a reference, not run |
| [Mol*](https://molstar.org/) | Existing molecular visualization toolkit |
| [Mol* orbital source](https://raw.githubusercontent.com/molstar/molstar/master/src/extensions/alpha-orbitals/orbitals.ts), [basis model](https://raw.githubusercontent.com/molstar/molstar/master/src/extensions/alpha-orbitals/data-model.ts), [density source](https://raw.githubusercontent.com/molstar/molstar/master/src/extensions/alpha-orbitals/density.ts) | Concrete quantum extension capabilities and limits |
| [3Dmol.js repository](https://github.com/3dmol/3Dmol.js) | Alternative molecular viewer |
| [vtk.js repository](https://github.com/Kitware/vtk-js) and [marching-cubes example](https://kitware.github.io/vtk-js/examples/ImageMarchingCubes.html) | Alternative scientific viewport |
| [wgpu documentation](https://docs.rs/wgpu/latest/wgpu/) | Native and browser backend choices |
| [Bevy WebGPU examples](https://bevy.org/examples-webgpu/) | Browser graphics target; no performance test performed |
| [Qt QRhi](https://doc.qt.io/qt-6/qrhi.html) | Qt graphics abstraction capabilities |
| [VTK WebGPU module](https://docs.vtk.org/en/latest/modules/vtk-modules/Rendering/WebGPU/README.html) | Current documented implementation/future-work distinction |
| [MoltenVK](https://github.com/KhronosGroup/MoltenVK) | Vulkan portability over Metal |
| [Apple Metal](https://developer.apple.com/metal/) | Native Apple graphics/compute option |
| [Safari 26 WebKit announcement](https://webkit.org/blog/17333/webkit-features-in-safari-26-0/) | WebGPU availability context, not packaged-app compatibility proof |

## Native storage

| Source | Used for |
| --- | --- |
| [ZIP specification](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT) | Standard container choice; no specification text reproduced |
| [HDF5 documentation](https://docs.hdfgroup.org/documentation/hdf5/latest/index.html) | Scientific container alternative |
| [Zarr core specification](https://zarr-specs.readthedocs.io/en/latest/v3/core/index.html) | Chunked-array alternative and versioned features |
| [QCSchema](https://molssi-qc-schema.readthedocs.io/en/latest/) and [wavefunction schema](https://molssi-qc-schema.readthedocs.io/en/latest/auto_wf.html) | Scientific interchange conventions and explicit mapping needs |

## Limitations of this research

Gaussian's official cubegen page could not be retrieved during this review; VMD and PySCF implementation sources were used instead. An attempted Mol* Molden parser path returned 404, so no claim of direct Molden-import support is made from that path. Some general sites returned little content; conclusions rely on the specific readable documentation and source files linked above.

The historical Flash tutorials were not run. No external viewer demo was benchmarked. No package was installed, no scientific reference calculation was executed, and no native-format file was generated. Library portability, browser memory, GPU feature support, and renderer fidelity remain implementation experiments. Local review findings are supported by code inspection and are not reported as reproduced crashes.
