use molekel_core::compute;
use molekel_import::import_bytes;

const SOURCE: &[u8] = include_bytes!("../../../../data/molden.input");

#[test]
fn repository_molden_density_generates_and_persists_at_minimum_ui_resolution() {
    let mut document = import_bytes(SOURCE, "molden.input").unwrap().document;
    assert_eq!(document.basis.len(), 125);
    let field = document.densities[0].id.clone();
    let hash = compute::source_hash(&document, &field).unwrap();
    let grid = compute::sample(&document, &field, 24).unwrap();
    assert_eq!(grid.dims, [24; 3]);
    assert!(grid.values.iter().all(|value| value.is_finite()));
    let mesh = compute::mesh(&grid, 0.08, &field, &hash, "#259d86", 0.63).unwrap();
    assert!(!mesh.indices.is_empty());
    assert_eq!(mesh.source_hash.as_deref(), Some(hash.as_str()));
    document.surfaces.push(mesh);
    let bytes = molekel_format::encode(&document).unwrap();
    let reopened = import_bytes(&bytes, "density.molekel").unwrap().document;
    assert_eq!(reopened, document);
    assert_eq!(compute::source_hash(&reopened, &field).unwrap(), hash);
}

#[test]
fn repository_molden_density_generates_larger_meshes_at_every_ui_resolution() {
    let document = import_bytes(SOURCE, "molden.input").unwrap().document;
    let field = &document.densities[0].id;
    let hash = compute::source_hash(&document, field).unwrap();
    let coarse_grid = compute::sample(&document, field, 24).unwrap();
    let coarse_mesh = compute::mesh(&coarse_grid, 0.08, field, &hash, "#259d86", 0.63).unwrap();

    for resolution in [32, 40, 48] {
        let grid = compute::sample(&document, field, resolution).unwrap();
        assert_eq!(grid.dims, [resolution; 3]);
        assert!(grid.values.iter().all(|value| value.is_finite()));
        for [x, y, z] in [
            [0; 3],
            [resolution - 1; 3],
            [resolution / 2; 3],
            [resolution / 2 + 1, resolution / 2 - 2, resolution / 2],
            [resolution / 3, resolution * 2 / 3, resolution / 2],
        ] {
            let point = grid.position([x as f64, y as f64, z as f64]);
            let expected = compute::evaluate(&document, field, point).unwrap().0;
            let actual = grid.values[(z * resolution + y) * resolution + x];
            assert!(
                (actual - expected).abs() <= 1e-12 + 1e-10 * expected.abs(),
                "resolution {resolution}, [{x}, {y}, {z}]: {actual} != {expected}"
            );
        }
        let mesh = compute::mesh(&grid, 0.08, field, &hash, "#259d86", 0.63).unwrap();
        assert_eq!(mesh.resolution, [resolution; 3]);
        assert_eq!(mesh.source_hash.as_deref(), Some(hash.as_str()));
        assert!(
            mesh.indices.len() > coarse_mesh.indices.len(),
            "resolution {resolution} must produce a finer mesh than 24"
        );
        let mut generated = document.clone();
        generated.surfaces.push(mesh);
        generated.validate().unwrap();
    }
}
