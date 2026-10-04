use eigenvista_core::{Document, Grid, MAX_SAMPLES, MAX_TRANSIENT_SAMPLES, compute, fixtures};

fn grid(dims: [usize; 3]) -> Grid {
    Grid {
        id: "sampled-field".into(),
        label: "Allocation boundary fixture".into(),
        quantity: "unknown".into(),
        origin: [0.; 3],
        axes: [[1., 0., 0.], [0., 1., 0.], [0., 0., 1.]],
        dims,
        values: vec![0.; dims.into_iter().product()],
    }
}

#[test]
fn analytic_sampling_retains_its_resolution_allocation_bound() {
    let document = fixtures::open_shell();
    for resolution in [0, 11, 257, usize::MAX] {
        let error = compute::sample(&document, "total", resolution).unwrap_err();
        assert!(error.contains("between 12 and 256"), "{error}");
    }
    let sampled = compute::sample(&document, "total", 256).unwrap();
    assert_eq!(sampled.dims, [256; 3]);
    assert_eq!(sampled.values.len(), MAX_TRANSIENT_SAMPLES);
    sampled.validate_transient().unwrap();
    assert!(sampled.validate().is_err());
    for [x, y, z] in [[0; 3], [128; 3], [255; 3], [127, 130, 123]] {
        let point = sampled.position([x as f64, y as f64, z as f64]);
        let expected = compute::evaluate(&document, "total", point).unwrap().0;
        let actual = sampled.values[(z * 256 + y) * 256 + x];
        assert!((actual - expected).abs() <= 1e-12 + 1e-10 * expected.abs());
    }
}

#[test]
fn high_resolution_constant_and_signed_planes_fit_the_actual_mesh_budget() {
    let mut field = grid([256; 3]);
    let empty = compute::mesh(&field, 0.08, "field", "hash", "#259d86", 0.63).unwrap();
    assert_eq!(empty.resolution, [256; 3]);
    assert!(empty.indices.is_empty());

    for (i, value) in field.values.iter_mut().enumerate() {
        *value = (i % 256) as f64 - 127.5;
    }
    for iso in [0.25, -0.25] {
        let surface = compute::mesh(&field, iso, "field", "hash", "#259d86", 0.63).unwrap();
        assert_eq!(surface.resolution, [256; 3]);
        assert_eq!(surface.positions.len() / 3, 255 * 255 * 6);
        assert_eq!(surface.indices.len(), 255 * 255 * 6);
        for position in surface.positions.chunks_exact(3) {
            assert!((position[0] - (127.5 + iso)).abs() < 1e-6);
        }
        for normal in surface.normals.chunks_exact(3) {
            assert!((normal[0] + iso.signum()).abs() < 1e-6);
            assert_eq!(&normal[1..], &[0., 0.]);
        }
    }
}

#[test]
fn pathological_mesh_still_exceeds_the_actual_vertex_allocation_bound() {
    let mut field = grid([64; 3]);
    for z in 0..64 {
        for y in 0..64 {
            for x in 0..64 {
                field.values[(z * 64 + y) * 64 + x] = ((x + y + z) % 2) as f64;
            }
        }
    }
    let error = compute::mesh(&field, 0.5, "field", "hash", "#259d86", 0.63).unwrap_err();
    assert!(error.contains("2,000,000-vertex memory budget"), "{error}");
}

#[test]
fn imported_grids_retain_the_sample_allocation_bound() {
    let mut document = Document::empty("Imported grid allocation boundary");
    document.grids.push(grid([128; 3]));
    assert_eq!(document.grids[0].values.len(), MAX_SAMPLES);
    document.validate().unwrap();

    document.grids[0].dims[0] = 129;
    document.grids[0].values.resize(129 * 128 * 128, 0.);
    document.grids[0].validate_transient().unwrap();
    let error = compute::sample(&document, "sampled-field", 24).unwrap_err();
    assert!(error.contains("128^3 sample budget"), "{error}");
}

#[test]
fn overflowing_grid_dimensions_are_rejected_before_allocation() {
    let mut invalid = grid([2; 3]);
    invalid.dims = [usize::MAX, 2, 2];
    let error = invalid.validate().unwrap_err();
    assert!(error.contains("128^3 sample budget"), "{error}");
    let error = compute::mesh(&invalid, 0.08, "field", "hash", "#259d86", 0.63).unwrap_err();
    assert!(error.contains("256^3 transient sample budget"), "{error}");
}
