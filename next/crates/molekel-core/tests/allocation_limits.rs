use molekel_core::{Document, Grid, MAX_SAMPLES, MAX_VERTICES, compute, fixtures};

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
    let document = fixtures::hydrogen_pair();
    for resolution in [0, 11, 81, usize::MAX] {
        let error = compute::sample(&document, "total", resolution).unwrap_err();
        assert!(error.contains("between 12 and 80"), "{error}");
    }
    let sampled = compute::sample(&document, "total", 80).unwrap();
    assert_eq!(sampled.dims, [80; 3]);
    assert_eq!(sampled.values.len(), 80usize.pow(3));
    sampled.validate().unwrap();
}

#[test]
fn meshing_retains_its_worst_case_vertex_allocation_bound() {
    assert!(51usize.pow(3) * 15 <= MAX_VERTICES);
    let accepted = compute::mesh(&grid([52; 3]), 0.08, "field", "hash", "#259d86", 0.63).unwrap();
    assert_eq!(accepted.resolution, [52; 3]);
    assert!(accepted.indices.is_empty());

    assert!(52usize.pow(3) * 15 > MAX_VERTICES);
    let error = compute::mesh(&grid([53; 3]), 0.08, "field", "hash", "#259d86", 0.63).unwrap_err();
    assert!(error.contains("Mesh worst-case allocation"), "{error}");
}

#[test]
fn imported_grids_retain_the_sample_allocation_bound() {
    let mut document = Document::empty("Imported grid allocation boundary");
    document.grids.push(grid([128; 3]));
    assert_eq!(document.grids[0].values.len(), MAX_SAMPLES);
    document.validate().unwrap();

    document.grids[0].dims[0] = 129;
    document.grids[0].values.resize(129 * 128 * 128, 0.);
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
    assert!(error.contains("128^3 sample budget"), "{error}");
}
