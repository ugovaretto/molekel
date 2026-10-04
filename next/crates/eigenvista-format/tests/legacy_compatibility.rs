mod support;

use eigenvista_core::{compute, fixtures};
use eigenvista_format::{decode, encode};
use serde_json::json;

#[test]
fn legacy_native_roundtrip_preserves_science_hashes_and_saved_surfaces() {
    let mut document = fixtures::hydrogen_pair();
    let hash = compute::source_hash(&document, "antibonding").unwrap();
    let grid = compute::sample(&document, "antibonding", 12).unwrap();
    for iso in [-0.02, 0.02] {
        let surface = compute::mesh(&grid, iso, "antibonding", &hash, "#00ff00", 0.5).unwrap();
        assert!(!surface.indices.is_empty());
        document.surfaces.push(surface);
    }
    document.grids.push(grid);
    let canonical = encode(&document).unwrap();
    let legacy = support::with_profile(&canonical, "molekel", &["molekel-preview-polynomial-v1"]);
    let loaded = decode(&legacy).unwrap();
    assert_eq!(loaded, document);
    assert_eq!(compute::source_hash(&loaded, "antibonding").unwrap(), hash);

    let resaved = encode(&loaded).unwrap();
    let manifest = support::manifest(&resaved);
    assert_eq!(manifest["format"], "eigenvista");
    assert_eq!(
        manifest["required_features"],
        json!(["eigenvista-preview-polynomial-v1"])
    );
    assert_eq!(manifest["version"], json!([0, 1]));
    assert_eq!(manifest["coordinate_unit"], "bohr");
    assert_eq!(manifest["arrays"], support::manifest(&legacy)["arrays"]);
    assert_eq!(manifest["document"], support::manifest(&legacy)["document"]);
    assert_eq!(decode(&resaved).unwrap(), document);
}

#[test]
fn native_profile_requires_exact_canonical_or_legacy_pair() {
    let bytes = encode(&fixtures::hydrogen_pair()).unwrap();
    for (format, profiles) in [
        ("eigenvista", vec!["molekel-preview-polynomial-v1"]),
        ("molekel", vec!["eigenvista-preview-polynomial-v1"]),
        ("unknown", vec!["eigenvista-preview-polynomial-v1"]),
        ("eigenvista", vec![]),
        ("molekel", vec![]),
        (
            "eigenvista",
            vec![
                "eigenvista-preview-polynomial-v1",
                "molekel-preview-polynomial-v1",
            ],
        ),
        ("molekel", vec!["molekel-preview-polynomial-v1", "unknown"]),
    ] {
        let modified = support::with_profile(&bytes, format, &profiles);
        assert!(decode(&modified).unwrap_err().contains("required profile"));
    }
}
