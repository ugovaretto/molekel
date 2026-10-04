use eigenvista_core::{Atom, Document, compute};
use eigenvista_import::import_bytes;
use serde::Deserialize;
use sha2::{Digest, Sha256};

const CASES: [(&str, &[u8], &str); 5] = [
    (
        "water-rhf-ccpvdz",
        include_bytes!("../../../fixtures/molden/water-rhf-ccpvdz.molden"),
        include_str!("../../../fixtures/molden/water-rhf-ccpvdz.json"),
    ),
    (
        "hydroxyl-uhf-sto3g",
        include_bytes!("../../../fixtures/molden/hydroxyl-uhf-sto3g.molden"),
        include_str!("../../../fixtures/molden/hydroxyl-uhf-sto3g.json"),
    ),
    (
        "general-spdfg-spherical",
        include_bytes!("../../../fixtures/molden/general-spdfg-spherical.molden"),
        include_str!("../../../fixtures/molden/general-spdfg-spherical.json"),
    ),
    (
        "general-spdfg-cartesian",
        include_bytes!("../../../fixtures/molden/general-spdfg-cartesian.molden"),
        include_str!("../../../fixtures/molden/general-spdfg-cartesian.json"),
    ),
    (
        "third-party/orca-nh3",
        include_bytes!("../../../fixtures/molden/third-party/orca-nh3.molden"),
        include_str!("../../../fixtures/molden/third-party/orca-nh3.json"),
    ),
];

#[derive(Deserialize)]
struct Fixture {
    reference: Reference,
}

#[derive(Deserialize)]
struct Reference {
    atoms: Vec<Atom>,
    points: Vec<[f64; 3]>,
    ao: Vec<Vec<[f64; 4]>>,
    orbitals: Vec<OrbitalReference>,
    densities: Vec<DensityReference>,
    overlap: Vec<f64>,
    orthonormal_orbitals: bool,
}

#[derive(Deserialize)]
struct OrbitalReference {
    spin: String,
    occupation: f64,
    energy: f64,
    coefficients: Vec<f64>,
    samples: Vec<[f64; 4]>,
}

#[derive(Deserialize)]
struct DensityReference {
    kind: String,
    samples: Vec<[f64; 4]>,
    electrons: f64,
}

fn close(actual: f64, expected: f64, context: &str) -> f64 {
    let error = (actual - expected).abs();
    // Molden serializes coefficients/basis parameters at finite text precision.
    let tolerance = 2e-9 + 2e-8 * expected.abs();
    assert!(
        error <= tolerance,
        "{context}: actual {actual:.16e}, expected {expected:.16e}, error {error:.4e}"
    );
    error
}

fn verify_fields(document: &Document, reference: &Reference, name: &str) {
    let n = document.basis.len();
    assert_eq!(reference.overlap.len(), n * n);
    assert_eq!(document.atoms.len(), reference.atoms.len());
    for (actual, expected) in document.atoms.iter().zip(&reference.atoms) {
        assert_eq!(actual.element, expected.element);
        for axis in 0..3 {
            close(
                actual.position[axis],
                expected.position[axis],
                "atom position",
            );
        }
    }
    let mut max_error = 0.0_f64;
    assert_eq!(reference.ao.len(), reference.points.len());
    for (point, expected) in reference.points.iter().zip(&reference.ao) {
        assert_eq!(expected.len(), n);
        for (index, (basis, expected)) in document.basis.iter().zip(expected).enumerate() {
            let (value, gradient) = compute::basis_value(basis, *point);
            for (actual, reference) in [value, gradient[0], gradient[1], gradient[2]]
                .into_iter()
                .zip(expected)
            {
                max_error =
                    max_error.max(close(actual, *reference, &format!("{name}, AO {index}")));
            }
        }
    }
    assert_eq!(document.orbitals.len(), reference.orbitals.len());
    for (orbital, expected) in document.orbitals.iter().zip(&reference.orbitals) {
        assert_eq!(orbital.spin, expected.spin);
        close(
            orbital.occupation.unwrap(),
            expected.occupation,
            "occupation",
        );
        close(orbital.energy.unwrap(), expected.energy, "energy");
        assert_eq!(orbital.coefficients.len(), expected.coefficients.len());
        for (actual, expected) in orbital.coefficients.iter().zip(&expected.coefficients) {
            close(
                *actual,
                *expected,
                "Molden AO order and MO coefficient scaling",
            );
        }
        assert_eq!(expected.samples.len(), reference.points.len());
        for (point, expected) in reference.points.iter().zip(&expected.samples) {
            let (value, gradient) = compute::evaluate(document, &orbital.id, *point).unwrap();
            for (actual, reference) in [value, gradient[0], gradient[1], gradient[2]]
                .into_iter()
                .zip(expected)
            {
                max_error = max_error.max(close(actual, *reference, &orbital.id));
            }
        }
    }
    assert_eq!(document.densities.len(), reference.densities.len());
    for expected in &reference.densities {
        let density = document
            .densities
            .iter()
            .find(|density| density.kind == expected.kind)
            .unwrap();
        assert_eq!(expected.samples.len(), reference.points.len());
        for (point, expected) in reference.points.iter().zip(&expected.samples) {
            let (value, gradient) = compute::evaluate(document, &density.id, *point).unwrap();
            for (actual, reference) in [value, gradient[0], gradient[1], gradient[2]]
                .into_iter()
                .zip(expected)
            {
                max_error = max_error.max(close(actual, *reference, &density.id));
            }
        }
        let electrons: f64 = (0..n)
            .flat_map(|i| {
                (0..n).map(move |j| density.matrix[i * n + j] * reference.overlap[j * n + i])
            })
            .sum();
        close(
            electrons,
            expected.electrons,
            "occupation-derived trace(P S)",
        );
    }
    for i in 0..n {
        close(
            reference.overlap[i * n + i],
            1.,
            "unit-normalized Molden AOs",
        );
    }
    if reference.orthonormal_orbitals {
        for a in &document.orbitals {
            for b in document.orbitals.iter().filter(|b| b.spin == a.spin) {
                let norm: f64 = (0..n)
                    .flat_map(|i| {
                        (0..n).map(move |j| {
                            a.coefficients[i] * reference.overlap[i * n + j] * b.coefficients[j]
                        })
                    })
                    .sum();
                close(norm, f64::from(a.id == b.id), "imported C^T S C");
            }
        }
    }
    println!(
        "{name}: {n} AOs, {} orbitals; max absolute field error {max_error:.4e}",
        document.orbitals.len()
    );
}

fn verify(index: usize) {
    let (name, source, json) = CASES[index];
    let imported = import_bytes(source, &format!("{name}.molden")).unwrap();
    imported.document.validate().unwrap();
    assert_eq!(imported.report.format, "molden");
    let fixture: Fixture = serde_json::from_str(json).unwrap();
    verify_fields(&imported.document, &fixture.reference, name);
    if index == 0 {
        assert_eq!(imported.document.bonds.len(), 2);
    } else if index == 1 {
        assert_eq!(imported.document.bonds.len(), 1);
    }
}

#[test]
fn rhf_water_import_matches_independent_values_gradients_and_ten_electrons() {
    verify(0);
}

#[test]
fn uhf_hydroxyl_import_matches_independent_spin_densities_and_electron_counts() {
    verify(1);
}

#[test]
fn normalized_spherical_general_contractions_through_g_match_pyscf() {
    verify(2);
}

#[test]
fn normalized_cartesian_general_contractions_through_g_match_pyscf() {
    verify(3);
}

#[test]
fn real_orca_export_matches_independent_spd_correction_and_fields() {
    verify(4);
    let imported = import_bytes(CASES[4].1, "orca-nh3.molden").unwrap();
    assert!(
        imported
            .report
            .warnings
            .iter()
            .any(|warning| warning.to_ascii_lowercase().contains("orca"))
    );
}

#[test]
fn fixture_sources_and_independent_results_have_pinned_checksums() {
    #[derive(Deserialize)]
    struct Entry {
        file: String,
        sha256: String,
        bytes: usize,
    }
    #[derive(Deserialize)]
    struct Manifest {
        fixtures: Vec<Entry>,
    }
    let manifest: Manifest =
        serde_json::from_str(include_str!("../../../fixtures/molden/manifest.json")).unwrap();
    assert_eq!(manifest.fixtures.len(), CASES.len() * 2);
    for entry in manifest.fixtures {
        let (stem, extension) = entry.file.rsplit_once('.').unwrap();
        let (_, source, json) = CASES.iter().find(|(name, _, _)| *name == stem).unwrap();
        let data = if extension == "json" {
            json.as_bytes()
        } else {
            source
        };
        assert_eq!(data.len(), entry.bytes);
        assert_eq!(format!("{:x}", Sha256::digest(data)), entry.sha256);
    }
}

#[test]
fn incompatible_shell_flags_are_not_silently_reinterpreted() {
    let spherical = std::str::from_utf8(CASES[2].1).unwrap();
    for bad in [
        spherical.replace("[5d]", "[5d]\n[6d]"),
        spherical.replace("[7f]", "[7f]\n[10f]"),
        spherical.replace("[9g]", "[9g]\n[15g]"),
    ] {
        assert!(import_bytes(bad.as_bytes(), "inconsistent.molden").is_err());
    }
}

#[test]
fn unqualified_orca_high_angular_momentum_profile_is_rejected() {
    let spherical = std::str::from_utf8(CASES[2].1).unwrap();
    let orca = spherical.replace(
        "made by pyscf v[2.14.0]",
        "Molden file created by orca_2mkl for BaseName=unqualified",
    );
    let error = import_bytes(orca.as_bytes(), "unqualified.molden.input").unwrap_err();
    assert!(error.contains("orca_2mkl"), "{error}");
    assert!(error.contains("F/G"), "{error}");
}

#[test]
fn imported_quantum_data_and_signed_cached_meshes_survive_native_conversion() {
    for (name, source, json) in &CASES[..2] {
        let mut document = import_bytes(source, &format!("{name}.input"))
            .unwrap()
            .document;
        let orbital = document
            .orbitals
            .iter()
            .rfind(|orbital| {
                orbital.spin != "beta" && orbital.occupation.is_some_and(|value| value > 0.)
            })
            .unwrap()
            .id
            .clone();
        let density = document
            .densities
            .iter()
            .find(|density| density.kind == "total")
            .unwrap()
            .id
            .clone();
        for (field, levels) in [(&orbital, vec![0.08, -0.08]), (&density, vec![0.08])] {
            let grid = compute::sample(&document, field, 24).unwrap();
            let hash = compute::source_hash(&document, field).unwrap();
            for isovalue in levels {
                let mesh = compute::mesh(&grid, isovalue, field, &hash, "#259d86", 0.63).unwrap();
                assert!(!mesh.indices.is_empty());
                document.surfaces.push(mesh);
            }
        }
        document.surfaces[1].visible = false;
        let bytes = eigenvista_format::encode(&document).unwrap();
        let reopened = import_bytes(&bytes, "converted.eigenvista").unwrap();
        assert_eq!(reopened.report.format, "eigenvista");
        assert_eq!(reopened.document, document);
        let fixture: Fixture = serde_json::from_str(json).unwrap();
        verify_fields(&reopened.document, &fixture.reference, name);
        assert!(
            reopened
                .document
                .provenance
                .iter()
                .any(|entry| entry.contains("SHA-256"))
        );
        assert!(
            reopened
                .document
                .densities
                .iter()
                .all(|density| density.label.contains("Occupation-derived"))
        );
    }
}

#[test]
fn legacy_molden_input_imports_without_modifying_its_source() {
    let source = include_bytes!("../../../../data/molden.input");
    let digest = format!("{:x}", Sha256::digest(source));
    assert_eq!(
        digest,
        "9309464ace24326f70c344852d4bbc49bc1bca418876a1a0d6b42863202b4645"
    );
    let imported = import_bytes(source, "molden.input").unwrap();
    let document = &imported.document;
    document.validate().unwrap();
    assert_eq!(document.atoms.len(), 17);
    assert_eq!(document.basis.len(), 125);
    assert_eq!(document.orbitals.len(), 118);
    assert_eq!(document.densities.len(), 1);
    assert_eq!(document.densities[0].kind, "total");
    assert!(document.bonds.len() >= 16);
    assert!(
        document
            .provenance
            .iter()
            .any(|entry| entry.contains(&digest))
    );
    for section in [
        "[FREQ]",
        "[FR-COORD]",
        "[FR-NORM-COORD]",
        "[GEOCONV]",
        "[GEOMETRIES]",
    ] {
        assert!(
            imported
                .report
                .warnings
                .iter()
                .any(|warning| warning.contains(section)),
            "{section}"
        );
    }
    let (value, gradient) =
        compute::evaluate(document, &document.orbitals[0].id, [0.31, -0.57, 1.21]).unwrap();
    assert!(
        [value, gradient[0], gradient[1], gradient[2]]
            .iter()
            .all(|value| value.is_finite())
    );
}
