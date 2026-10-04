use molekel_core::{Document, compute, fixtures};
use serde::Deserialize;

const REFERENCES: [&str; 4] = [
    include_str!("../../../fixtures/pyscf/water-rhf-ccpvdz.json"),
    include_str!("../../../fixtures/pyscf/hydroxyl-uhf-sto3g.json"),
    include_str!("../../../fixtures/pyscf/general-spdfg-spherical.json"),
    include_str!("../../../fixtures/pyscf/general-spdfg-cartesian.json"),
];

#[derive(Deserialize)]
struct Fixture {
    document: Document,
}

fn compare_sampled_reference(index: usize) {
    let fixture: Fixture = serde_json::from_str(REFERENCES[index]).unwrap();
    let document = fixture.document;
    let fields = document
        .orbitals
        .iter()
        .take(2)
        .map(|orbital| &orbital.id)
        .chain(document.densities.iter().map(|density| &density.id));
    for field in fields {
        let grid = compute::sample(&document, field, 12).unwrap();
        assert_eq!(grid.dims, [12; 3]);
        assert!(grid.values.iter().all(|value| value.is_finite()));
        for [x, y, z] in [
            [0, 0, 0],
            [11, 11, 11],
            [5, 6, 7],
            [6, 5, 4],
            [2, 8, 3],
            [8, 3, 9],
        ] {
            let point = grid.position([x as f64, y as f64, z as f64]);
            let expected = compute::evaluate(&document, field, point).unwrap().0;
            let actual = grid.values[(z * 12 + y) * 12 + x];
            assert!(
                (actual - expected).abs() <= 1e-12 + 1e-10 * expected.abs(),
                "fixture {index}, {field}, [{x}, {y}, {z}]: {actual} != {expected}"
            );
        }
    }
}

#[test]
fn closed_shell_sampling_matches_reference_evaluator() {
    compare_sampled_reference(0);
}

#[test]
fn open_shell_sampling_matches_reference_evaluator() {
    compare_sampled_reference(1);
}

#[test]
fn spherical_sampling_through_g_matches_reference_evaluator() {
    compare_sampled_reference(2);
}

#[test]
fn cartesian_sampling_through_g_matches_reference_evaluator() {
    compare_sampled_reference(3);
}

fn compare_density_matrix(matrix: Vec<f64>, kind: &str) -> Vec<f64> {
    let mut document = fixtures::hydrogen_pair();
    document.densities[0].kind = kind.into();
    document.densities[0].matrix = matrix;
    document.validate().unwrap();
    let grid = compute::sample(&document, "total", 12).unwrap();
    for z in 0..12 {
        for y in 0..12 {
            for x in 0..12 {
                let point = grid.position([x as f64, y as f64, z as f64]);
                let expected = compute::evaluate(&document, "total", point).unwrap().0;
                let actual = grid.values[(z * 12 + y) * 12 + x];
                assert!(
                    (actual - expected).abs() <= 1e-14 + 1e-12 * expected.abs(),
                    "{kind}, [{x}, {y}, {z}]: {actual} != {expected}"
                );
            }
        }
    }
    grid.values
}

#[test]
fn nonsymmetric_transition_density_keeps_both_matrix_halves() {
    let values = compare_density_matrix(vec![1., 3., -2., -0.5], "transition");
    assert!(values.iter().any(|value| *value > 0.));
    assert!(values.iter().any(|value| *value < 0.));
}

#[test]
fn antisymmetric_density_contributes_no_scalar_field() {
    let values = compare_density_matrix(vec![0., 3., -3., 0.], "transition");
    assert!(values.iter().all(|value| value.abs() <= 1e-14));
}

#[test]
fn signed_density_sampling_preserves_negative_values() {
    for kind in ["spin", "difference"] {
        let values = compare_density_matrix(vec![1., 0.2, 0.2, -1.], kind);
        assert!(values.iter().any(|value| *value > 0.));
        assert!(values.iter().any(|value| *value < 0.));
    }
}

#[test]
fn finite_off_diagonal_coefficients_do_not_overflow_when_combined() {
    let mut document = fixtures::hydrogen_pair();
    // Widely separated Gaussians keep Pij * AOi * AOj small even though Pij + Pji overflows.
    document.basis[0].center[0] = -19.;
    document.basis[1].center[0] = 19.;
    document.densities[0].matrix = vec![0., 1e308, 1e308, 0.];
    document.validate().unwrap();
    let grid = compute::sample(&document, "total", 12).unwrap();
    assert!(grid.values.iter().any(|value| *value > 0.));
    for z in 0..12 {
        for y in 0..12 {
            for x in 0..12 {
                let point = grid.position([x as f64, y as f64, z as f64]);
                let expected = compute::evaluate(&document, "total", point).unwrap().0;
                let actual = grid.values[(z * 12 + y) * 12 + x];
                assert!(actual.is_finite());
                assert!(
                    (actual - expected).abs() <= 1e-300 + 1e-12 * expected.abs(),
                    "[{x}, {y}, {z}]: {actual} != {expected}"
                );
            }
        }
    }
}

#[test]
fn finite_folded_density_coefficients_avoid_intermediate_overflow() {
    let mut document = fixtures::hydrogen_pair();
    document.basis[0].center = [0.; 3];
    document.basis[1].center = [0.; 3];
    document.basis[0].coefficients = vec![4.];
    document.basis[1].coefficients = vec![1e-309];
    document.densities[0].matrix = vec![0., 5e307, 5e307, 0.];
    document.validate().unwrap();
    let grid = compute::sample(&document, "total", 12).unwrap();
    assert!(grid.values.iter().any(|value| *value > 0.1));
    for z in 0..12 {
        for y in 0..12 {
            for x in 0..12 {
                let point = grid.position([x as f64, y as f64, z as f64]);
                let expected = compute::evaluate(&document, "total", point).unwrap().0;
                let actual = grid.values[(z * 12 + y) * 12 + x];
                assert!(actual.is_finite());
                assert!(
                    (actual - expected).abs() <= 1e-12 + 1e-12 * expected.abs(),
                    "[{x}, {y}, {z}]: {actual} != {expected}"
                );
            }
        }
    }
}
