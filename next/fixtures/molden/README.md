# Independent Molden import fixtures

These frozen files test actual external-text import, not a EigenVista-generated
document masquerading as a format test. Normal Rust and browser tests need no
Python, PySCF, IOData, ORCA installation, or network access. The generator never
calls an EigenVista evaluator, parser, or conversion executable.

| Molden file and matching JSON | Scientific content |
| --- | --- |
| `water-rhf-ccpvdz` | RHF water, cc-pVDZ, 24 spherical AOs, 24 spatial MOs, 10-electron occupation-derived total density |
| `hydroxyl-uhf-sto3g` | UHF OH, STO-3G, 6 AOs, 6 alpha and 6 beta MOs, total/alpha/beta/spin densities with 9/5/4/1 electrons |
| `general-spdfg-spherical` | 50 spherical AOs, two three-primitive contractions in each S/P/D/F/G shell, five normalized probe orbitals with fractional occupations |
| `general-spdfg-cartesian` | Corresponding 70 Cartesian AOs with component-dependent primitive normalization accounted for in the exported MO coefficients |
| [`third-party/orca-nh3`](third-party/README.md) | Unmodified real `orca_2mkl` NH3 export from IOData's test corpus: 50 S/P/5D AOs, 50 spatial MOs, 10-electron density |

For each pair, `.molden` is the import input and `.json` contains independent
expected AO, orbital, density, and x/y/z gradient values at eight asymmetric
points, overlap integrals, occupations, energies, coordinates, and provenance.
`manifest.json` pins both file bytes and SHA-256 digests. The ORCA input and
derived reference have a separate upstream attribution/license under
[`third-party/`](third-party/README.md); they are not runtime application code.

## Numerical independence and conventions

The first four inputs were exported by PySCF 2.14.0's actual Molden writer.
Reference values come from compiled PySCF/libcint `GTOval_*_deriv1`, with NumPy
MO contraction and PySCF `eval_rho` for density values and gradients. Overlap
integrals come from `int1e_ovlp`. These are not evaluations of the Rust explicit
polynomial representation. All point coordinates and gradients use bohr;
orbital energies use hartree.

Expected AO arrays and overlap matrices are expressed in Molden component
order. PySCF Cartesian D/F/G functions are not individually unit normalized;
its Molden writer divides those AOs by their overlap norms and multiplies MO
coefficients by the same factors. The expected data apply that documented
change of representation to independently evaluated arrays. Spherical
functions use Molden's `m=0,+1,-1,+2,-2,...` order for D/F/G, with explicit
`[5d]/[7f]/[9g]` flags; Cartesian files declare `[6d]/[10f]/[15g]`.

The molecular calculations converge to a `1e-12` energy tolerance. Their
geometries are test geometries, not optimized structures. The synthetic S-G
probe orbitals are individually normalized, but not guaranteed mutually
orthogonal; occupations `[0.25, 0.5, 1, 1.25, 2]` and energies are synthetic
test metadata, not a physical helium calculation. The two general contractions
are exported as two ordinary segmented Molden shell records. This tests
multi-primitive contraction handling, not a nonstandard multi-column GTO
extension. The resulting synthetic density integrates to five by construction.

For the real ORCA file, pinned IOData independently recognizes and corrects
its primitive-normalization convention, then exports canonical Molden. PySCF
reads that intermediate file and evaluates it through libcint. IOData also
checks MO orthonormality with its independent overlap implementation. The
original ORCA bytes, not the corrected intermediate, are supplied to Rust.
The method and ORCA version are not stated by the upstream fixture and are
not invented here. This case qualifies S/P/5D only; it provides no evidence
for ORCA F/G phase conventions or every ORCA version.

Primary references: [Molden format specification](https://www.theochem.ru.nl/molden/molden_format.html),
[PySCF Molden implementation](https://pyscf.org/_modules/pyscf/tools/molden.html),
[PySCF basis evaluation](https://pyscf.org/pyscf_api_docs/pyscf.gto.html),
[PySCF density evaluation](https://pyscf.org/_modules/pyscf/dft/numint.html),
and [pinned IOData Molden importer](https://github.com/theochem/iodata/blob/9f7e800fc414b086d677b5f2882dd0c1dfa919f3/iodata/formats/molden.py).

## Acceptance and limits

The [Rust integration tests](../../crates/eigenvista-import/tests/independent_molden.rs)
compare every stored AO/MO/density value and gradient with
`abs(error) <= 2e-9 + 2e-8 * abs(reference)`, allowing the input text's finite
precision. They additionally check imported coefficients/order, geometry,
automatic bonds, `C^T S C` within SCF spin blocks, and `trace(P S)` populations.
Water and OH imports are meshed at both orbital signs plus a total-density
surface, encoded to `.eigenvista`, reopened through the shared importer, and
compared exactly including scientific inputs, provenance, and saved meshes.

A separate read-only smoke regression checks the existing repository
[`data/molden.input`](../../../data/molden.input): 17 atoms, 125 AOs, 118 MOs,
automatic bonds, one occupation-derived density, and explicit warnings for
omitted vibration/geometry-history sections. Its input digest is pinned, but
it has no independent numerical field oracle. It must remain in full-checkout
and packaged-source test environments; the rewrite never modifies it.

On the recorded environment the largest canonical value/gradient discrepancy
was `3.13e-12`; the ORCA route's largest absolute discrepancy was `6.64e-9`,
within the relative tolerance. This is finite-precision convention validation,
not a claim that ORCA-derived values are exact to `1e-12`.

Additional tests reject contradictory shell flags and the unqualified
high-angular-momentum ORCA profile. Sparse MO coefficients are valid Molden,
so a declared basis with omitted indices cannot be rejected solely because
the largest supplied index is less than the basis size.

The suite does not qualify arbitrary producer conventions, all ORCA exports,
ECP/relativistic/spinor wavefunctions, complex coefficients, H or higher shells,
correlated/transition density recovery, spatial-integral convergence, complete
real-world file dialect coverage, or the mesher's topology. Imported densities
are explicitly derived from listed orbitals and occupations, not independently
supplied matrices. PySCF and the format exporter share conventions, but the
real ORCA fixture adds a separate producer and parser to the evidence.

## Regeneration

Regenerate deliberately, never as an automatic response to a failed numerical
test. From the repository root, create a development-only environment and
pin the independent IOData source. All temporary files remain in repository
`tmp/`; reuse existing directories only after checking their contents.

```sh
mkdir -p tmp
export TMPDIR="$PWD/tmp" TMP="$PWD/tmp" TEMP="$PWD/tmp"
python3 -m venv tmp/reference-venv
tmp/reference-venv/bin/python -m pip install --only-binary=:all: -r next/tools/reference-requirements.txt
git clone --depth 1 https://github.com/theochem/iodata.git tmp/molden-iodata-reference
git -C tmp/molden-iodata-reference fetch --depth 1 origin 9f7e800fc414b086d677b5f2882dd0c1dfa919f3
git -C tmp/molden-iodata-reference checkout --detach 9f7e800fc414b086d677b5f2882dd0c1dfa919f3
tmp/reference-venv/bin/python -m pip install attrs==26.1.0 ./tmp/molden-iodata-reference
tmp/reference-venv/bin/python next/tools/generate_molden_references.py --iodata-source tmp/molden-iodata-reference
cargo test --manifest-path next/Cargo.toml --locked -p eigenvista-import --test independent_molden
```

The generator verifies the IOData revision and original ORCA fixture digest.
It overwrites only these generated Molden/JSON pairs, their manifest, and the
copied upstream license; the canonical ORCA intermediate stays in ignored
`tmp/`. The recorded environment is Python 3.14.7, PySCF 2.14.0, NumPy 2.5.3,
SciPy 1.18.1, h5py 3.16.0, attrs 26.1.0, and the pinned IOData revision on Apple
Silicon. A shallow source build reports IOData package version `0.0.post1`;
the commit, not this inferred version label, identifies the importer used.

Review changed orbital phases, degenerate subspaces, software versions, and
checksums when regenerating. Keep the frozen expected values independent of
the implementation under test. The generator is an offline qualification tool,
not an additional application converter or mandatory build dependency.
