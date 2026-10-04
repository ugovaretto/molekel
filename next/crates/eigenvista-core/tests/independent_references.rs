use eigenvista_core::{Document, compute};
use serde::Deserialize;
use sha2::{Digest, Sha256};
use std::collections::BTreeMap;

const FIXTURES: [(&str, &str); 4] = [
    (
        "water-rhf-ccpvdz.json",
        include_str!("../../../fixtures/pyscf/water-rhf-ccpvdz.json"),
    ),
    (
        "hydroxyl-uhf-sto3g.json",
        include_str!("../../../fixtures/pyscf/hydroxyl-uhf-sto3g.json"),
    ),
    (
        "general-spdfg-spherical.json",
        include_str!("../../../fixtures/pyscf/general-spdfg-spherical.json"),
    ),
    (
        "general-spdfg-cartesian.json",
        include_str!("../../../fixtures/pyscf/general-spdfg-cartesian.json"),
    ),
];

#[derive(Deserialize)]
struct Fixture {
    document: Document,
    reference: Reference,
}

#[derive(Deserialize)]
struct Reference {
    points: Vec<[f64; 3]>,
    ao: Vec<Vec<[f64; 4]>>,
    fields: Vec<FieldReference>,
    overlap: Vec<f64>,
    electrons: BTreeMap<String, f64>,
    orthonormal_orbitals: bool,
    grid_field: String,
}

#[derive(Deserialize)]
struct FieldReference {
    id: String,
    samples: Vec<[f64; 4]>,
}

fn close(actual: f64, expected: f64, context: &str) -> f64 {
    let error = (actual - expected).abs();
    let tolerance = 1e-10 + 1e-8 * expected.abs();
    assert!(
        error <= tolerance,
        "{context}: actual {actual:.16e}, expected {expected:.16e}, error {error:.4e}"
    );
    error
}

fn verify(index: usize) {
    let (name, json) = FIXTURES[index];
    let fixture: Fixture = serde_json::from_str(json).unwrap();
    let d = fixture.document;
    let r = fixture.reference;
    d.validate().unwrap();
    let n = d.basis.len();
    assert_eq!(r.ao.len(), r.points.len());
    assert_eq!(r.overlap.len(), n * n);
    let mut max_error = 0.0_f64;
    for (p, expected) in r.points.iter().zip(&r.ao) {
        assert_eq!(expected.len(), n);
        for (b, expected) in d.basis.iter().zip(expected) {
            let (value, gradient) = compute::basis_value(b, *p);
            for (actual, reference) in [value, gradient[0], gradient[1], gradient[2]]
                .into_iter()
                .zip(expected)
            {
                max_error = max_error.max(close(actual, *reference, name));
            }
        }
    }
    for field in &r.fields {
        assert_eq!(field.samples.len(), r.points.len());
        for (p, expected) in r.points.iter().zip(&field.samples) {
            let (value, gradient) = compute::evaluate(&d, &field.id, *p).unwrap();
            for (actual, reference) in [value, gradient[0], gradient[1], gradient[2]]
                .into_iter()
                .zip(expected)
            {
                max_error = max_error.max(close(actual, *reference, &field.id));
            }
        }
    }
    if r.orthonormal_orbitals {
        for a in &d.orbitals {
            for b in d.orbitals.iter().filter(|b| b.spin == a.spin) {
                let mut norm = 0.;
                for i in 0..n {
                    for j in 0..n {
                        norm += a.coefficients[i] * r.overlap[i * n + j] * b.coefficients[j];
                    }
                }
                close(norm, f64::from(a.id == b.id), "C^T S C");
            }
        }
    }
    for (field, expected) in &r.electrons {
        let density = d.densities.iter().find(|v| &v.id == field).unwrap();
        let mut electrons = 0.;
        for i in 0..n {
            for j in 0..n {
                electrons += density.matrix[i * n + j] * r.overlap[j * n + i];
            }
        }
        close(electrons, *expected, "trace(P S)");
    }
    let grid = &d.grids[0];
    for z in 0..grid.dims[2] {
        for y in 0..grid.dims[1] {
            for x in 0..grid.dims[0] {
                let point = grid.position([x as f64, y as f64, z as f64]);
                let actual = compute::evaluate(&d, &r.grid_field, point).unwrap().0;
                let i = (z * grid.dims[1] + y) * grid.dims[0] + x;
                max_error =
                    max_error.max(close(actual, grid.values[i], "affine x-fastest samples"));
            }
        }
    }
    println!(
        "{name}: {n} AOs, {} fields, {} points; maximum absolute error {max_error:.4e}",
        r.fields.len(),
        r.points.len()
    );
}

#[test]
fn closed_shell_pyscf_values_gradients_normalization_and_grid() {
    verify(0);
}

#[test]
fn open_shell_pyscf_values_gradients_populations_and_grid() {
    verify(1);
}

#[test]
fn spherical_general_contractions_through_g_match_pyscf() {
    verify(2);
}

#[test]
fn cartesian_general_contractions_through_g_match_pyscf() {
    verify(3);
}

#[test]
fn reference_fixture_checksums_are_pinned() {
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
        serde_json::from_str(include_str!("../../../fixtures/pyscf/manifest.json")).unwrap();
    assert_eq!(manifest.fixtures.len(), FIXTURES.len());
    for entry in manifest.fixtures {
        let (_, data) = FIXTURES
            .iter()
            .find(|(name, _)| *name == entry.file)
            .unwrap();
        assert_eq!(data.len(), entry.bytes);
        assert_eq!(
            format!("{:x}", Sha256::digest(data.as_bytes())),
            entry.sha256
        );
    }
}

#[test]
fn independent_fixture_gradients_have_central_difference_convergence() {
    for (_, json) in FIXTURES {
        let fixture: Fixture = serde_json::from_str(json).unwrap();
        let d = fixture.document;
        let point = [0.31, -0.57, 1.21];
        for field in &fixture.reference.fields {
            let gradient = compute::evaluate(&d, &field.id, point).unwrap().1;
            for (axis, derivative) in gradient.iter().enumerate() {
                let errors: Vec<_> = [1e-3, 1e-4]
                    .into_iter()
                    .map(|h| {
                        let mut left = point;
                        let mut right = point;
                        left[axis] -= h;
                        right[axis] += h;
                        let numerical = (compute::evaluate(&d, &field.id, right).unwrap().0
                            - compute::evaluate(&d, &field.id, left).unwrap().0)
                            / (2. * h);
                        (numerical - derivative).abs()
                    })
                    .collect();
                assert!(
                    errors[1] <= errors[0] * 0.03 + 1e-10,
                    "{}: {errors:?}",
                    field.id
                );
                assert!(
                    errors[1] <= 1e-7 + 1e-7 * derivative.abs(),
                    "{}: {errors:?}",
                    field.id
                );
            }
        }
    }
}
