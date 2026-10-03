# Molekel v1 requirements

The complete v1 should open scientific documents on macOS, render structures and quantum fields, and preserve enough information to reproduce the view. The native document is the first development target; external importers can be delivered afterward through the same conversion contract.

Requirements below distinguish the user's requested capabilities from proposed implementation details. The latter can change after the validation milestones without silently dropping a requested feature.

## Required capabilities

| ID | Requirement | Acceptance evidence |
| --- | --- | --- |
| R01 | Ball-and-stick representation | Element colors, separately adjustable atom/bond sizes, picking, and stable geometry for zero-length or absent bonds |
| R02 | Liquorice representation | Rounded cylinders, consistent radius, isolated atoms visible, and coloring consistent with R01 |
| R03 | Van der Waals representation | Spheres use the selected radius dataset; displayed radii and units are inspectable |
| R04 | Molecular orbitals evaluated from basis data | Selected orbital values and gradients agree with independent reference data; both signs can be displayed |
| R05 | Density matrix surface | Explicit real density matrices and occupation-derived densities produce correct fields; density type is named |
| R06 | Cube scalar fields | Preserve origin, full axes, dimensions, channel identifiers, values, units, and positive/negative isovalues |
| R07 | OBJ geometry and information | Load supported polygon meshes; edit metadata, units, transform, molecule association, color, and opacity; save these in `.molekel` |
| R08 | PDB and XYZ compatibility | Import molecular coordinates with diagnostics; explicit policy for multiple models, unknown elements, and inferred bonds |
| R09 | Molden compatibility | Import supported geometry, basis, MO coefficients, energies, spins, and occupations; reject unsupported scientific conventions explicitly |
| R10 | Atom colors from text | Read legacy positional RGB data; validate finite values in [0,1]; provide defaults for elements without overrides |
| R11 | Per-surface color and transparency | Independent positive/negative materials; visibility and opacity changes do not recompute scientific data |
| R12 | Marching cubes isosurfaces | User can change isovalue, field, sampling domain, and resolution; meshes agree with the field within the declared discretization error |
| R13 | Shrinkwrap isosurfaces | User-selectable projected refinement with reported convergence and topology checks; failures remain visible and recoverable |
| R14 | Direct volume rendering | Editable signed transfer function, threshold/isovalue band, density scale, and sampling quality; opacity remains consistent when the ray step changes |
| R15 | Shader raycasting | Isosurface intersections computed in shaders for sampled volumes, plus direct analytic orbital evaluation within a tested workload envelope |
| R16 | Native `.molekel` documents | Structure and quantum data survive save/reopen without loss of canonical numeric precision; derived data and presentation state have explicit identities |
| R17 | macOS application | Installable, signed/notarized release candidate, local file workflows, offline opening, and responsive cancellation |
| R18 | Browser application | Same document semantics and essential viewing modes; tested capability detection and clear resource-limit messages |

R18 is promoted from the brief's "ideally" to a design target because the chosen architecture shares its interface. macOS remains the first release priority. A browser launch may follow the desktop release if browser validation is incomplete; it must not be described as already supported.

## Scientific meanings

An orbital is the signed amplitude `psi`, not `psi^2`. Electron density is an occupation-weighted quantity or the contraction of an AO density matrix with basis values. Spin density is distinct from total electron density. A general cube field may instead contain potential, ELF, or another scalar quantity, so the file must not be labeled electron density by extension alone.

The initial basis profile supports real contracted Gaussian functions through G shells, Cartesian and real spherical forms, shared SP shells converted to explicit components, and restricted or unrestricted orbitals. Occupations may be fractional. Unsupported higher angular momentum, Slater functions, complex coefficients, or periodic wavefunctions must not be silently truncated. Geometry and preserved source data may still be opened.

An explicit density matrix may represent a correlated or excited-state density. Record that state; do not replace it with a ground-state orbital reconstruction. Electron-number checks use represented electrons, including the effect of ECPs and partial orbital datasets.

## Interpretation decisions

| Ambiguous phrase | Proposed interpretation | Consequence |
| --- | --- | --- |
| Van der Waals radius | Space-filling spheres using a documented radius table | A fused envelope can be an additional later feature |
| Shrinkwrap | Mesh vertices projected onto an implicit level set with adaptive remeshing | Initialize components/topology from a validated coarse extraction; do not assume every orbital is a sphere |
| Volume rendering with isovalue | A threshold or a transfer-function band centered on the requested value | This changes optical classification, not the stored field |
| Shader raycasting of an F-Rep | Direct evaluation of an orbital expression along rays, as well as a texture-backed mode | Direct evaluation gets its own precision and performance checks |
| Information for an OBJ | Descriptive properties and scientific associations attached to a geometric object | Unknown units are resolved explicitly; no invented quantum data |
| Native format plus later converters | Application depends on `.molekel`; adapters produce that same model | Early milestones can omit broad file-format support without changing core APIs |

These interpretations are recorded decisions for planning. If shrinkwrap is intended to mean a specific historical implementation, or analytic raycasting must work at a particular system size, those details must be fixed before declaring the relevant milestone complete.

## User workflows

1. Open a native file, choose a structure representation, inspect an atom, change the color table, and save the document.
2. Select an orbital by stable identity, inspect energy/spin/occupation, set the domain and isovalue, and display both signs using any required rendering method.
3. Select an explicit or reconstructed density, inspect how it was obtained, generate a surface, and change its appearance independently.
4. Import a cube, choose its channel, inspect its units and axes, and overlay its surfaces or volume with the molecular structure.
5. Import an OBJ, resolve its scale and alignment, enter descriptive properties, and retain those settings on reopen.
6. Convert an external chemistry file, review any missing information, and open the resulting native document. Geometry-only results remain useful and clearly identified.
7. Cancel a long operation or change parameters while it runs. The last completed view stays usable; stale work cannot replace newer results.

## Supporting product requirements

These are proposed additions needed for a usable scientific tool: orbit/pan/zoom/reset, object selection, a scene list, editable material controls, a transfer-function editor, a computation panel, undo for view/metadata changes, local save/reopen, and image export with the field parameters recorded. A compact orbital table should support sorting without changing orbital identity.

Scientific arrays are immutable while a calculation is using them. Editing structure coordinates invalidates associated derived fields and must identify the wavefunction as belonging to the original geometry; it must not silently move atoms while treating the old coefficients as a recalculated wavefunction.

## Explicitly outside the first evaluation profile

Geometry optimization, running electronic-structure calculations, dynamics, vibration animation, spectra, SAS/SES, arbitrary user shader execution, volumetric path tracing, complex spinors, and periodic electronic structure are outside the requested first implementation. Preserve source metadata where useful so these can be added deliberately.

No requested rendering method is relegated to this list. A prototype with only marching cubes is an intermediate milestone, not completion of v1.

## Release criteria

Every required row must have a recorded outcome against the [validation plan](07-validation-and-delivery.md). Advertise importer and device support by tested profile. A native-only preview is acceptable as a staged release, but must not claim the original external-format compatibility target has been fulfilled.
