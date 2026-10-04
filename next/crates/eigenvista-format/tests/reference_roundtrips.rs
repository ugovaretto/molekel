use eigenvista_core::{Document, compute};
use eigenvista_format::{decode, encode};
use serde::Deserialize;

#[derive(Deserialize)]
struct Fixture {
    document: Document,
}

fn roundtrip(json: &str, orbital: &str) {
    let mut doc = serde_json::from_str::<Fixture>(json).unwrap().document;
    for (field, levels) in [(orbital, vec![0.08, -0.08]), ("total", vec![0.08])] {
        let grid = compute::sample(&doc, field, 24).unwrap();
        let hash = compute::source_hash(&doc, field).unwrap();
        for iso in levels {
            let color = if iso > 0. { "#158f71" } else { "#c74870" };
            let surface = compute::mesh(&grid, iso, field, &hash, color, 0.43).unwrap();
            assert!(!surface.indices.is_empty(), "{field} at {iso}");
            doc.surfaces.push(surface);
        }
    }
    doc.surfaces[1].visible = false;
    doc.validate().unwrap();
    let restored = decode(&encode(&doc).unwrap()).unwrap();
    assert_eq!(restored, doc);
    assert_eq!(restored.surfaces.len(), 3);
    for field in [orbital, "total"] {
        assert_eq!(
            compute::evaluate(&doc, field, [0.31, -0.57, 1.21]).unwrap(),
            compute::evaluate(&restored, field, [0.31, -0.57, 1.21]).unwrap()
        );
    }
}

#[test]
fn closed_shell_quantum_data_orbital_and_density_meshes_roundtrip() {
    roundtrip(
        include_str!("../../../fixtures/pyscf/water-rhf-ccpvdz.json"),
        "spatial-5",
    );
}

#[test]
fn open_shell_quantum_data_orbital_and_density_meshes_roundtrip() {
    roundtrip(
        include_str!("../../../fixtures/pyscf/hydroxyl-uhf-sto3g.json"),
        "alpha-5",
    );
}
