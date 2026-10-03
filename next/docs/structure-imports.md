# Structure imports and automatic bonds

XYZ and PDB imports automatically compute display connectivity in the shared Rust core before the document reaches the viewer. This applies in both the native application and the browser. There is no separate bond-generation action. The atom/bond counts are shown in the document summary, and inferred bonds are saved in `.molekel` files alongside explicit bonds.

XYZ accepts one conventional atom-count/comment/coordinate frame in angstroms.
Each atom row contains an element symbol and three coordinates. Extra nonempty
records, including additional frames and extended XYZ columns, are rejected;
unlike PDB, XYZ does not select the first of several frames.

## Algorithm decision

The legacy implementation was inspected, not copied. `src/utility/MolekelChemPDBImporter.cpp` uses `UniformGrid`, 27 neighboring buckets, a 1.2-times-covalent-radii cutoff, and angle/valence pruning. Other legacy paths delegate to Open Babel. The located PDB implementation is a uniform grid, not a k-d tree.

All-pairs distance testing is quadratic and unsuitable for large structures. Cell lists are an excellent choice for roughly uniform particle systems with bounded cutoffs; ASE documents a linearly scaling neighbor-list implementation and covalent-radius-based cutoffs. A sparse cell list avoids the empty-volume allocation cost of a dense grid. A k-d tree is also a good fit for a static, nonperiodic point cloud, including sparse structures and element-dependent query radii. No claim is made that a k-d tree always beats a cell list. [ASE neighbor lists](https://docs.ase-lib.org/ase/neighborlist.html).

The preview uses the existing Rust `kiddo` 5.2.4 immutable k-d tree with exact squared-Euclidean radius queries. It was selected for reusable, tested spatial indexing, successful native/WASM builds, and the measured workloads below. It avoids an application-specific spatial-tree implementation. Approximate-nearest-neighbor search is not used. [Kiddo documentation](https://docs.rs/crate/kiddo/5.2.4).

The distance test uses the Pyykko-Atsumi single-bond covalent radii for elements 1-118, as supplied by pinned `pdbtbx` 0.12.0. These are distinct from the viewer's provisional van der Waals display radii. [Original radii publication](https://chemistry-europe.onlinelibrary.wiley.com/doi/10.1002/chem.200800987), [PDB Toolbox](https://docs.rs/pdbtbx/0.12.0/pdbtbx/).

1. Convert the model's bohr coordinates to angstroms for the geometric test.
2. Preserve and deduplicate explicit connections first, including bonds longer than the automatic cutoff.
3. Query each atom with radius `r_i + max(r_j) + 0.45 angstrom`, then retain pairs with `0.4 <= distance <= r_i + r_j + 0.45 angstrom`. Evaluate each unordered pair once. The search radius has a small numerical guard; acceptance uses the actual pair cutoff.
4. Sort candidates by increasing `distance / (r_i + r_j)`, with deterministic index tie-breaks. Limit inferred coordination to one for H/F, four for B/C/N, and three for O. Existing explicit bonds consume coordination slots but are never removed. Other elements have no chemistry-specific cap, avoiding a blanket organic-valence rule for metals/hypervalent species.
5. Return sorted, unique endpoint pairs with no self-bonds. Report the method and any coordination rejections in provenance.

This is a geometric display-connectivity heuristic, not electronic bond-order perception. The minimum-distance and light-element coordination filters avoid common false connections and overlaps. They do not establish aromaticity, oxidation state, metal coordination, hydrogen bonds, or chemical validity in arbitrary geometries. Open Babel likewise separates proximity-based connections from subsequent bond-order handling and applies minimum-distance/valence checks. The Rust implementation does not link to Open Babel. [Open Babel `ConnectTheDots`](https://openbabel.org/api/3.0/classOpenBabel_1_1OBMol.shtml).

Bounds are explicit: 100,000 atoms, 600,000 output connections, 2,000,000 candidates, and 10,000,000 visited neighbors. Excessively dense or invalid inputs fail rather than silently returning a partial bond list. Each radius-query result is at most the input atom count. Coordinates must be finite and within 1e9 angstrom of the origin. These are numerical/resource limits, not a claim that the current individual-mesh renderer is interactive at the maximum atom count.

## PDB profile

`pdbtbx` handles atomic records and the residue/conformer hierarchy. A bounded fixed-column preflight supplies format-specific compatibility, rejects invalid coordinates before library construction, and reads `CONECT` endpoints. The library's optional Rayon/R-tree/serialization features are disabled; its compression feature is enabled for a build constraint in its read-options API, but compressed import is not exposed.

- Read `ATOM` and `HETATM`, using angstrom coordinates. Keep the first MODEL or first concatenated structure, never merge coordinate frames. Interactive model selection is not implemented yet.
- Prefer explicit element columns 77-78. Otherwise use aligned atom names: ` CA ` is carbon, `CA  ` is calcium, and `1HG ` is hydrogen. Unknown elements fail. D/T labels become hydrogen with an explicit isotope-loss notice.
- Keep common atoms and one alternate conformer per residue, selected by highest mean occupancy; ties use the alphabetically first alternate label, then residue name. Exclude zero-occupancy atoms. Do not combine per-atom choices from incompatible conformers.
- Retain unique `CONECT` connections from columns 12-31, resolve serial numbers after selection, and infer additional bonds even when `CONECT` is present. Ignore connections to deliberately omitted alternatives. Unknown serials, self-connections, and duplicate atom serials fail. Old hydrogen/salt-bridge columns after 31 are not imported as covalent bonds. [wwPDB coordinate records](https://www.wwpdb.org/documentation/file-format-content/format33/sect9.html), [wwPDB connectivity records](https://www.wwpdb.org/documentation/file-format-content/format33/sect10.html).
- Support the repository's old identifier/line-number suffixes only when matched to a legacy HEADER. A narrowly scoped repair recognizes full-occupancy, element-unspecified five-column nucleotide hydrogen names such as ` H2'1`; it records the repair and leaves partial-occupancy numeric alternates alone.
- Blank occupancy and B-factor fields default to 1 and 0. Invalid numbers do not become fabricated coordinates. Non-ASCII records and hybrid-36 serials are rejected in this profile.

The preview stores coordinates, elements, and connectivity, not the full PDB hierarchy. Residue/chain labels, formal charges, occupancies/B-factors, isotope identity, unit cells, symmetry and secondary-structure annotations are not retained after selection; provenance records this loss. Crystal/periodic images are not generated. No quantum data is inferred from a structure file. mmCIF and gzipped PDB are not currently accepted.

## Verification

Tests cover water/methane XYZ, disconnected fragments, element alignment, model/alternate selection, explicit-plus-inferred connections, duplicate suppression, malformed input, dense-input budgets, coincident/planar/collinear points, and an independent exhaustive pair-distance comparison. Browser tests open XYZ/PDB through the actual file input, inspect rendered pixels, save/reopen connectivity, and check mobile layout and recovery after malformed imports.

Repository fixtures, read without modification:

| File | Selected atoms | Bonds |
| --- | ---: | ---: |
| `data/guanine.pdb` | 33 | 35 |
| `data/3POR.pdb` | 2,325 | 2,285 |
| `data/URIDINE-VANADATE.pdb` | 31 | 33 |
| `data/alaninemulti.pdb` | 66 (first structure only) | 65 |

Single release-build measurements on the development Apple Silicon Mac, 3 October 2026, using `cargo run --locked --release -p molekel-core --example bond_benchmark`:

| Workload | Time | Spatial visits | Hypothetical all-pairs comparisons |
| --- | ---: | ---: | ---: |
| 3POR parsing plus connectivity | 7.17 ms | Not recorded | Not recorded |
| Synthetic 10,000-atom lattice, connectivity only | 8.99 ms | 49,600 | 49,995,000 |
| Synthetic 100,000-atom lattice, connectivity only | 96.10 ms | 676,000 | 4,999,950,000 |

These are bounded development measurements, not universal performance promises or renderer benchmarks. The lattice is a spatial-index stress input, not a chemically meaningful bonding reference. Bond orders and broader structure-conversion profiles remain separate work.
