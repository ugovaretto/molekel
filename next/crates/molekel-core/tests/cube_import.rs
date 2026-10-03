use molekel_core::{compute, import};
use std::path::Path;

const CUBE: &str = "Signed affine water grid\nScalar test fixture\n3 -1 2 3\n2 1 0 0\n2 0.2 1 0\n2 0 0 -1\n8 8 0 0 0\n1 1 1.8 0 0\n1 1 -0.45 1.75 0\n-1 1 -1 1 -1 1 -1 1\n";

#[test]
fn cube_bonds_bohr_positions_and_affine_field_are_preserved() {
    let d = import::cube(CUBE, "water.cube").unwrap();
    assert_eq!(d.bonds, [[0, 1], [0, 2]]);
    assert_eq!(d.atoms[1].position, [1.8, 0., 0.]);
    assert_eq!(d.grids[0].origin, [-1., 2., 3.]);
    assert_eq!(d.grids[0].axes[1], [0.2, 1., 0.]);
    assert_eq!(d.grids[0].axes[2], [0., 0., -1.]);
    assert_eq!(d.grids[0].values, [-1., -1., -1., -1., 1., 1., 1., 1.]);
    assert!(
        d.provenance
            .iter()
            .any(|s| s.starts_with("Automatic bonds:"))
    );
    assert!(d.basis.is_empty() && d.orbitals.is_empty() && d.densities.is_empty());
    let sampled = compute::sample(&d, "cube", 12).unwrap();
    assert_eq!(sampled, d.grids[0]);
    let hash = compute::source_hash(&d, "cube").unwrap();
    for (iso, height) in [(0.5, 2.25), (-0.5, 2.75)] {
        let mesh = compute::mesh(&sampled, iso, "cube", &hash, "#00ff00", 0.6).unwrap();
        assert!(!mesh.indices.is_empty());
        assert!(
            mesh.positions
                .chunks_exact(3)
                .all(|p| (p[2] - height).abs() < 1e-7)
        );
        assert_eq!(mesh.source_hash.as_deref(), Some(hash.as_str()));
        assert_eq!(mesh.grid_axes, sampled.axes);
    }
}

#[test]
fn standard_single_orbital_cube_consumes_dataset_header_not_field_values() {
    let orbital = CUBE
        .replace("3 -1 2 3", "-3 -1 2 3 1")
        .replace("-1 1 -1 1 -1 1 -1 1", "1\n19\n-1D0 1d0 -1 1 -1 1 -1 1");
    let d = import::cube(&orbital, "orbital.cub").unwrap();
    assert_eq!(d.bonds, [[0, 1], [0, 2]]);
    assert_eq!(d.grids[0].values, [-1., -1., -1., -1., 1., 1., 1., 1.]);
    assert_eq!(d.grids[0].label, "orbital.cub (dataset 19)");
    assert!(
        d.provenance
            .iter()
            .any(|s| s.contains("dataset identifier 19"))
    );
    assert!(d.orbitals.is_empty());
}

#[test]
fn low_amplitude_cube_gets_a_visible_initial_isovalue_without_changing_samples() {
    let input = CUBE.replace(
        "-1 1 -1 1 -1 1 -1 1",
        "-0.001 0.001 -0.001 0.001 -0.001 0.001 -0.001 0.001",
    );
    let document = import::cube(&input, "small.cube").unwrap();
    assert_eq!(document.view.isovalue, 0.0001);
    assert_eq!(
        document.grids[0].values,
        [-0.001, -0.001, -0.001, -0.001, 0.001, 0.001, 0.001, 0.001]
    );
    assert!(
        document
            .provenance
            .iter()
            .any(|p| p.starts_with("Initial display isovalue"))
    );
    let hash = compute::source_hash(&document, "cube").unwrap();
    let mesh = compute::mesh(
        &document.grids[0],
        document.view.isovalue,
        "cube",
        &hash,
        "#00ff00",
        0.6,
    )
    .unwrap();
    assert!(!mesh.indices.is_empty());
    assert_eq!(
        import::cube(CUBE, "normal.cube").unwrap().view.isovalue,
        0.08
    );
    let zero = CUBE.replace("-1 1 -1 1 -1 1 -1 1", "0 0 0 0 0 0 0 0");
    assert_eq!(
        import::cube(&zero, "zero.cube").unwrap().view.isovalue,
        0.08
    );
}

#[test]
fn cube_limits_and_unsupported_channels_fail_explicitly() {
    for (text, expected) in [
        (CUBE.replace("3 -1 2 3", "3 -1 2 3 2"), "single-channel"),
        (CUBE.replace("3 -1 2 3", "-2147483648 -1 2 3"), "atom count"),
        (CUBE.replace("3 -1 2 3", "100001 -1 2 3"), "atom count"),
        (CUBE.replace("2 1 0 0", "-2 1 0 0"), "positive"),
        (CUBE.replace("2 1 0 0", "1 1 0 0"), "sample budget"),
        (CUBE.replace("2 1 0 0", "2000000 1 0 0"), "sample budget"),
        (CUBE.replace("2 0 0 -1", "2 0 0 0"), "invalid axes"),
        (CUBE.replace("8 8 0 0 0", "0 0 0 0 0"), "atomic number"),
        (
            CUBE.replace("-1 1 -1 1 -1 1 -1 1", "-1 1 NaN 1 -1 1 -1 1"),
            "Non-finite",
        ),
        (CUBE.replace("-1 1 -1 1 -1 1 -1 1", "-1 1"), "Truncated"),
        (format!("{CUBE} 9"), "extra cube values"),
    ] {
        assert!(
            import::cube(&text, "bad.cube")
                .unwrap_err()
                .contains(expected),
            "{expected}"
        );
    }
    let orbital = CUBE.replace("3 -1 2 3", "-3 -1 2 3");
    for (header, expected) in [
        ("0", "single-channel"),
        ("2 19 20", "single-channel"),
        ("1 invalid", "dataset identifier"),
    ] {
        let input = orbital.replace("-1 1 -1 1 -1 1 -1 1", header);
        assert!(
            import::cube(&input, "bad.cube")
                .unwrap_err()
                .contains(expected)
        );
    }
    let missing = orbital.replace("-1 1 -1 1 -1 1 -1 1", "");
    assert!(
        import::cube(&missing, "bad.cube")
            .unwrap_err()
            .contains("dataset count")
    );
    assert!(
        import::cube(&format!("{missing}1"), "bad.cube")
            .unwrap_err()
            .contains("dataset identifier")
    );
}

#[test]
fn repository_density_and_single_orbital_cubes_generate_bonds_and_surfaces() {
    let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../..");
    for (path, atoms, bonds, dims, signed) in [
        ("data/h2o-dens.cube", 3, 2, [40, 40, 40], false),
        (
            "all_data/Benzene.MO19-BOTH-SIGNS.cube",
            12,
            12,
            [60, 60, 60],
            true,
        ),
        (
            "all_data/molden_test/test_homo.cube",
            3,
            2,
            [40, 40, 40],
            true,
        ),
    ] {
        let text = std::fs::read_to_string(root.join(path)).unwrap();
        let document = import::cube(&text, path).unwrap();
        assert_eq!(document.atoms.len(), atoms, "{path}");
        assert_eq!(document.bonds.len(), bonds, "{path}");
        assert_eq!(document.grids[0].dims, dims, "{path}");
        let before = document.grids[0].clone();
        let sampled = compute::sample(&document, "cube", 24).unwrap();
        let hash = compute::source_hash(&document, "cube").unwrap();
        let positive = compute::mesh(&sampled, 0.02, "cube", &hash, "#00ff00", 0.6).unwrap();
        assert!(!positive.indices.is_empty(), "{path}");
        if signed {
            let negative = compute::mesh(&sampled, -0.02, "cube", &hash, "#ff0000", 0.6).unwrap();
            assert!(!negative.indices.is_empty(), "{path}");
        }
        assert_eq!(document.grids[0], before);
    }
}
