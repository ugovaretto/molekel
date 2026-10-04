"""Generate frozen Molden import references with PySCF, never EigenVista."""

import argparse
import hashlib
import importlib.metadata
import io
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]
TMP = ROOT.parent / "tmp"
TMP.mkdir(exist_ok=True)
for key in ("TMPDIR", "TMP", "TEMP"):
    os.environ[key] = str(TMP)
tempfile.tempdir = str(TMP)
os.environ["OMP_NUM_THREADS"] = "1"

import numpy as np
from pyscf import dft, gto, lib, scf
from pyscf.tools import molden

lib.num_threads(1)
OUTPUT = ROOT / "fixtures" / "molden"
IODATA_REVISION = "9f7e800fc414b086d677b5f2882dd0c1dfa919f3"
ORCA_SHA256 = "3c3a0baa0d404c091b5531fa28e8a8f51c38f4e8c263708c0173bea2923b2568"


def write_fixture(name, mol, blocks, calculation, orthonormal, source_text=None, extra_producer=None):
    """blocks contain source PySCF coefficients, occupations, energies, and spin."""
    output = io.StringIO()
    molden.header(mol, output, ignore_h=False)
    for block in blocks:
        molden.orbital_coeff(
            mol, output, block["coefficients"], spin=block["spin"],
            ene=block["energies"], occ=block["occupations"], ignore_h=False,
        )
    text = output.getvalue() if source_text is None else source_text
    points = np.array([
        [0.0, 0.0, 0.0], [0.31, -0.57, 1.21], [-1.4, 0.6, -0.8],
        [0.125, -0.25, 0.375], [0.8, 1.3, -0.4], [-0.07, 0.19, 0.43],
        [2.1, -1.7, 0.95], [4.0, -3.0, 2.0],
    ])
    suffix = "cart" if mol.cart else "sph"
    ao = mol.eval_gto(f"GTOval_{suffix}_deriv1", points, cutoff=1e-30)
    overlap = mol.intor("int1e_ovlp")
    order = np.array(molden.order_ao_index(mol))
    # PySCF's writer normalizes Cartesian components and compensates MOs.
    # Preserve source evaluation, then express AO/overlap references in that
    # independently documented exported convention, without rebuilding a basis.
    norms = np.sqrt(overlap.diagonal()) if mol.cart else np.ones(mol.nao_nr())
    exported_ao = (ao / norms[None, None, :])[:, :, order]
    exported_overlap = (overlap / norms[:, None] / norms[None, :])[np.ix_(order, order)]
    orbital_refs = []
    density_matrices = {}
    for block in blocks:
        coefficients = block["coefficients"]
        spin = block["spin"].lower() if len(blocks) > 1 else "spatial"
        for i, vector in enumerate(coefficients.T):
            orbital_refs.append({
                "spin": spin, "occupation": float(block["occupations"][i]),
                "energy": float(block["energies"][i]),
                "coefficients": (vector * norms)[order].tolist(),
                "samples": np.einsum("dpa,a->pd", ao, vector).tolist(),
            })
        matrix = np.einsum("ai,i,bi->ab", coefficients, block["occupations"], coefficients)
        density_matrices[spin if len(blocks) > 1 else "total"] = matrix
    if len(blocks) > 1:
        density_matrices["total"] = density_matrices["alpha"] + density_matrices["beta"]
        density_matrices["spin"] = density_matrices["alpha"] - density_matrices["beta"]
    densities = []
    for kind, matrix in density_matrices.items():
        densities.append({
            "kind": kind,
            "samples": dft.numint.eval_rho(mol, ao, matrix, xctype="GGA", hermi=0).T.tolist(),
            "electrons": float(np.einsum("ij,ji->", matrix, overlap)),
        })
    reference = {
        "producer": {
            "packages": {p: importlib.metadata.version(p) for p in ("pyscf", "numpy", "scipy", "h5py")},
            "python": platform.python_version(), "calculation": calculation,
            "ao_convention": "unit-normalized Molden Cartesian" if mol.cart else "Molden real spherical",
            "backend": "PySCF GTOval_*_deriv1/int1e_ovlp/eval_rho; NumPy MO contraction",
            "molden_sha256": hashlib.sha256(text.encode()).hexdigest(),
        },
        "reference": {
            "atoms": [{"element": int(z), "position": p.tolist()}
                      for z, p in zip(mol.atom_charges(), mol.atom_coords())],
            "points": points.tolist(), "ao": exported_ao.transpose(1, 2, 0).tolist(),
            "ao_labels": [mol.ao_labels()[i] for i in order],
            "orbitals": orbital_refs, "densities": densities,
            "overlap": exported_overlap.ravel().tolist(),
            "orthonormal_orbitals": orthonormal,
        },
    }
    reference["producer"].update(extra_producer or {})
    entries = []
    for extension, data in (
        ("molden", text.encode()),
        ("json", (json.dumps(reference, separators=(",", ":"), allow_nan=False) + "\n").encode()),
    ):
        path = OUTPUT / f"{name}.{extension}"
        path.write_bytes(data)
        entries.append({"file": str(path.relative_to(OUTPUT)), "bytes": len(data), "sha256": hashlib.sha256(data).hexdigest()})
    return entries


def orca_fixture(source):
    revision = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=source, text=True).strip()
    if revision != IODATA_REVISION:
        raise ValueError(f"IOData source must be pinned to {IODATA_REVISION}, got {revision}")
    changed = subprocess.check_output(
        ["git", "status", "--porcelain", "--untracked-files=no"], cwd=source, text=True,
    ).strip()
    if changed:
        raise ValueError("The independent IOData source checkout has modified tracked files")
    sys.path.insert(0, str(source))
    import iodata
    from iodata import dump_one, load_one
    from iodata.overlap import compute_overlap

    assert Path(iodata.__file__).resolve().parent == source / "iodata"
    original = source / "iodata" / "test" / "data" / "nh3_orca.molden"
    text = original.read_text()
    assert hashlib.sha256(text.encode()).hexdigest() == ORCA_SHA256
    loaded = load_one(str(original), fmt="molden")
    # A separate established importer resolves the producer convention, then
    # PySCF/libcint evaluates its canonical export. EigenVista is never imported.
    canonical_path = TMP / "molden-orca-independent-canonical.molden"
    dump_one(loaded, str(canonical_path), fmt="molden")
    mol, energies, coefficients, occupations, _, _ = molden.load(str(canonical_path))
    overlap = compute_overlap(loaded.obasis, loaded.atcoords)
    np.testing.assert_allclose(loaded.mo.coeffs.T @ overlap @ loaded.mo.coeffs,
                               np.eye(loaded.mo.norb), atol=2e-8, rtol=2e-8)
    source_dir = OUTPUT / "third-party"
    source_dir.mkdir(exist_ok=True)
    (source_dir / "IODATA-LICENSE.txt").write_bytes((source / "LICENSE.txt").read_bytes())
    return write_fixture("third-party/orca-nh3", mol, [{
        "coefficients": coefficients, "occupations": occupations,
        "energies": energies, "spin": "Alpha",
    }], {
        "method": "Frozen upstream ORCA export; method/version not stated by source fixture",
        "source": "iodata/test/data/nh3_orca.molden", "iodata_revision": revision,
        "conversion": "IOData ORCA profile correction, canonical Molden export, PySCF Molden load",
    }, True, source_text=text, extra_producer={
        "iodata_revision": revision,
        "source_url": f"https://github.com/theochem/iodata/blob/{revision}/iodata/test/data/nh3_orca.molden",
        "license": "GPL-3.0-or-later (upstream fixture); see IODATA-LICENSE.txt",
        "qc_iodata": importlib.metadata.version("qc-iodata"),
        "attrs": importlib.metadata.version("attrs"),
    })


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--iodata-source", required=True, type=Path,
                        help="IOData source checkout at the documented pinned revision")
    args = parser.parse_args()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    manifest = []
    water = gto.M(
        atom="O 0 0 0; H 0 -1.43233673 1.10715266; H 0 1.43233673 1.10715266",
        unit="Bohr", basis="cc-pvdz", verbose=0,
    )
    rhf = scf.RHF(water)
    rhf.conv_tol = 1e-12
    rhf.kernel()
    assert rhf.converged
    manifest.extend(write_fixture("water-rhf-ccpvdz", water, [{
        "coefficients": rhf.mo_coeff, "occupations": rhf.mo_occ,
        "energies": rhf.mo_energy, "spin": "Alpha",
    }], {"method": "RHF", "basis": "cc-pVDZ", "energy_hartree": rhf.e_tot,
         "convergence_tolerance": rhf.conv_tol}, True))

    radical = gto.M(atom="O 0 0 0; H 0 0 1.83", unit="Bohr", basis="sto-3g", spin=1, verbose=0)
    uhf = scf.UHF(radical)
    uhf.conv_tol = 1e-12
    uhf.kernel()
    assert uhf.converged
    manifest.extend(write_fixture("hydroxyl-uhf-sto3g", radical, [{
        "coefficients": uhf.mo_coeff[i], "occupations": uhf.mo_occ[i],
        "energies": uhf.mo_energy[i], "spin": spin,
    } for i, spin in enumerate(("Alpha", "Beta"))], {
        "method": "UHF", "basis": "STO-3G", "energy_hartree": uhf.e_tot,
        "convergence_tolerance": uhf.conv_tol, "spin_squared": uhf.spin_square()[0],
    }, True))

    shells = [[l, [4.1, 0.17, -0.31], [0.7, 0.58, 0.29], [0.12, 0.24, 0.63]] for l in range(5)]
    for cart in (False, True):
        mol = gto.M(atom="He 0.125 -0.25 0.375", unit="Bohr", basis={"He": shells}, cart=cart, verbose=0)
        probes = np.zeros((mol.nao_nr(), 5))
        offsets = mol.ao_loc_nr()
        overlap = mol.intor("int1e_ovlp")
        for shell in range(mol.nbas):
            lo, hi = offsets[shell:shell + 2]
            probes[lo:hi, shell] = np.sin(np.arange(1, hi - lo + 1))
            probes[:, shell] /= np.sqrt(probes[:, shell] @ overlap @ probes[:, shell])
        convention = "cartesian" if cart else "spherical"
        manifest.extend(write_fixture(f"general-spdfg-{convention}", mol, [{
            "coefficients": probes, "occupations": np.array([0.25, 0.5, 1, 1.25, 2]),
            "energies": np.array([-1.0, -0.4, 0.3, 0.7, 1.1]), "spin": "Alpha",
        }], {
            "method": "Synthetic normalized probe orbitals, not SCF or physical occupations/energies",
            "basis": "Two general contractions x three primitives in each S/P/D/F/G shell",
        }, False))
    manifest.extend(orca_fixture(args.iodata_source.resolve()))
    (OUTPUT / "manifest.json").write_text(json.dumps({"fixtures": manifest}, indent=2) + "\n")
    for entry in manifest:
        print(f"{entry['file']}: {entry['bytes']} bytes, sha256 {entry['sha256']}")


if __name__ == "__main__":
    main()
