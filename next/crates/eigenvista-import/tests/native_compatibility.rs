#[path = "../../eigenvista-format/tests/support/mod.rs"]
mod support;

use eigenvista_core::{compute, fixtures};
use eigenvista_format::{decode, encode};
use eigenvista_import::import_bytes;

#[test]
fn legacy_native_import_reports_new_name_and_preserves_science_during_bond_repair() {
    let mut document = fixtures::hydrogen_pair();
    document.bonds.clear();
    let hash = compute::source_hash(&document, "antibonding").unwrap();
    let grid = compute::sample(&document, "antibonding", 12).unwrap();
    let surface = compute::mesh(&grid, 0.02, "antibonding", &hash, "#00ff00", 0.5).unwrap();
    assert!(!surface.indices.is_empty());
    document.grids.push(grid);
    document.surfaces.push(surface);
    let canonical = encode(&document).unwrap();
    let legacy = support::with_profile(&canonical, "molekel", &["molekel-preview-polynomial-v1"]);
    assert_eq!(decode(&legacy).unwrap(), document);
    for bytes in [&legacy, &canonical] {
        for name in [
            "old.molekel",
            "new.eigenvista",
            "UPPER.EIGENVISTA",
            "unknown",
        ] {
            let result = import_bytes(bytes, name).unwrap();
            assert_eq!(result.report.format, "eigenvista");
            assert!(result.report.requires_save);
            assert_eq!(result.document.bonds, [[0, 1]]);
            assert!(result.report.warnings[0].contains("Added 1 missing display bonds"));
            assert_eq!(
                compute::source_hash(&result.document, "antibonding").unwrap(),
                hash
            );
            let mut without_repair = result.document.clone();
            without_repair.bonds = document.bonds.clone();
            without_repair.provenance = document.provenance.clone();
            assert_eq!(without_repair, document);

            let saved = encode(&result.document).unwrap();
            assert_eq!(support::manifest(&saved)["format"], "eigenvista");
            let reopened = import_bytes(&saved, "saved.eigenvista").unwrap();
            assert_eq!(reopened.document, result.document);
            assert!(!reopened.report.requires_save);
            assert!(reopened.report.warnings.is_empty());
        }
    }
}

#[test]
fn legacy_native_import_without_bond_changes_remains_clean() {
    let document = fixtures::hydrogen_pair();
    let legacy = support::with_profile(
        &encode(&document).unwrap(),
        "molekel",
        &["molekel-preview-polynomial-v1"],
    );
    let result = import_bytes(&legacy, "original.molekel").unwrap();
    assert_eq!(result.report.format, "eigenvista");
    assert_eq!(result.document, document);
    assert!(!result.report.requires_save);
    assert!(result.report.warnings.is_empty());
}
