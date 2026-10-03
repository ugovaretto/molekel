# Independent numerical references

These frozen numerical fixtures were generated with PySCF 2.14.0 on Apple Silicon using Python 3.14.7. They are test inputs, not application dependencies or a general-purpose PySCF converter. No Molekel evaluator is called by the generator. Normal Rust/browser tests read the checked-in data without Python or PySCF.

| Fixture | Content |
| --- | --- |
| `water-rhf-ccpvdz.json` | Closed-shell water, RHF/cc-pVDZ, 24 real spherical AOs, 24 spatial MOs, explicit 10-electron density |
| `hydroxyl-uhf-sto3g.json` | Open-shell OH, UHF/STO-3G, 6 AOs, separate alpha/beta MOs and matrices, total and spin densities, 5 alpha and 4 beta electrons |
| `general-spdfg-spherical.json` | 50 real spherical AOs, two general contractions of three primitives per S/P/D/F/G shell, five polynomial probes, nonsymmetric synthetic transition matrix |
| `general-spdfg-cartesian.json` | Corresponding 70 Cartesian AOs and synthetic probes/matrix, with libcint's component-dependent D/F/G normalization |

The molecular geometries are explicitly chosen test geometries, not optimized structures. RHF/UHF convergence is required at `1e-12` energy tolerance. The frozen total energies are -76.02675905148838 and -74.36249681893162 hartree, respectively. The OH UHF spin-squared value is 0.7532295363088823; this is not an exactly spin-pure reference. The S-G probes are not calculated or normalized molecular orbitals and have no assigned energies or occupations.

## Conventions and independence

All coordinates are bohr. Each file records software versions, calculation settings, AO labels/order, and source conventions. `document` is the explicit-polynomial Molekel model. `reference` contains independent AO/MO/density values and x/y/z gradients, overlap integrals, expected populations, and point coordinates. Each document also contains one asymmetric, skewed, reflected 3 x 4 x 5 grid in x-fastest order.

The export step expands PySCF shell data into explicit polynomials using `bas_ctr_coeff`, `gto_norm`, and `cart2sph`. Primitive and contraction normalization is absorbed into the radial coefficients. Cartesian S/P angular factors are included; Cartesian D/F/G preserve libcint's normalization, not an assumed unit norm for every component. The Rust evaluator applies no hidden shell normalization or ordering convention.

Expected AO values and derivatives come from PySCF's compiled `GTOval_*_deriv1` evaluator. MO references use NumPy contraction with PySCF SCF coefficients. Density values and gradients use PySCF `eval_rho` with its full non-Hermitian path. Thus expected values do not come from the polynomial evaluator being tested. However, the conversion and reference generator share PySCF conventions; this does not validate Molden or other producer conventions.

Primary references: [PySCF Gaussian basis API](https://pyscf.org/pyscf_api_docs/pyscf.gto.html), [PySCF basis normalization and transformations](https://pyscf.org/_modules/pyscf/gto/mole.html), and [PySCF density evaluation](https://pyscf.org/_modules/pyscf/dft/numint.html). PySCF is Apache-2.0 licensed; the bundled STO-3G and cc-pVDZ parameters are exported from that installation. The generated fixtures contain numerical data, not copied implementation source. Full release notice collection remains a release gate.

## Checks and limits

Rust tests compare every stored AO/field value and gradient with `abs(error) <= 1e-10 + 1e-8 * abs(reference)`. They also check `C^T S C` within each SCF spin block, `trace(P S)` populations, affine-grid samples, central-difference gradient convergence, and the SHA-256 manifest. Native format tests preserve the molecular documents, both signs of a HOMO mesh, a total-density mesh, and appearance exactly through Rust encode/decode.

Chromium/WebKit tests exercise the same frozen field values and gradients through Rust/WASM after a native-format roundtrip. Browser JSON canonicalizes negative zero to positive zero; scientific hashes intentionally treat them identically. A separate test opens a natively generated transition-density mesh with negative-zero matrix entries, edits its material in JavaScript, and saves it without losing the scientific association.

This suite does not validate full molecular spatial-integral convergence, all diffuse/tight exponents, complex orbitals, relativistic/ECP profiles, producer-specific ordering, topology, or rendering accuracy. The analytic two-center integral check remains separate. Broader scientific qualification is still required.

## Regeneration

From the repository root, using a development-only environment in the ignored repository temporary directory:

```sh
mkdir -p tmp
export TMPDIR="$PWD/tmp" TMP="$PWD/tmp" TEMP="$PWD/tmp"
python3 -m venv tmp/reference-venv
tmp/reference-venv/bin/python -m pip install --only-binary=:all: -r next/tools/reference-requirements.txt
tmp/reference-venv/bin/python next/tools/generate_references.py
```

The generator also sets all temporary-directory variables before loading PySCF. Packages are pinned for the recorded environment; normal builds do not install them. Regeneration overwrites only these four generated JSON fixtures and their manifest. Review numerical changes rather than automatically regenerating expected data in tests: SCF phases and degenerate subspaces may differ with software or BLAS versions. `manifest.json` pins the checked-in bytes.

To produce usable `.molekel` examples from the frozen molecular fixtures, run from `next/` with the same temporary-directory environment:

```sh
cargo run --locked -p molekel-format --example reference_documents
```

This creates files in ignored `next/artifacts/references/`, each with quantum data and cached meshes. Water and OH initially show signed HOMO meshes; a saved total-density mesh is also available but hidden. The synthetic spherical example displays its supplied transition density. All use the current classic marching-cubes preview, not topology-certified extraction. The browser test runner creates these artifacts automatically.
