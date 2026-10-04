use eigenvista_core::{Document, compute};
use eigenvista_format::{encode, native::save_atomic};
use serde::Deserialize;
use std::path::Path;

#[path = "../tests/support/mod.rs"]
mod support;

#[derive(Deserialize)]
struct Fixture {
    document: Document,
}

fn main() -> Result<(), String> {
    let output = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../artifacts/references");
    std::fs::create_dir_all(&output).map_err(|e| e.to_string())?;
    for (json, field, title, bonds) in [
        (
            include_str!("../../../fixtures/pyscf/water-rhf-ccpvdz.json"),
            "spatial-5",
            "Water / RHF cc-pVDZ reference",
            vec![[0, 1], [0, 2]],
        ),
        (
            include_str!("../../../fixtures/pyscf/hydroxyl-uhf-sto3g.json"),
            "alpha-5",
            "Hydroxyl / UHF STO-3G reference",
            vec![[0, 1]],
        ),
        (
            include_str!("../../../fixtures/pyscf/general-spdfg-spherical.json"),
            "transition",
            "General S-G / synthetic transition reference",
            vec![],
        ),
    ] {
        let mut doc = serde_json::from_str::<Fixture>(json)
            .map_err(|e| e.to_string())?
            .document;
        doc.title = title.into();
        doc.bonds = bonds;
        doc.view.field = Some(field.into());
        for (selected, levels) in [(field, vec![0.08, -0.08]), ("total", vec![0.08])] {
            if doc.check_field(selected).is_err() {
                continue;
            }
            let grid = compute::sample(&doc, selected, 32)?;
            let hash = compute::source_hash(&doc, selected)?;
            for iso in levels {
                let color = if iso > 0. {
                    &doc.view.positive_color
                } else {
                    &doc.view.negative_color
                };
                let mut surface =
                    compute::mesh(&grid, iso, selected, &hash, color, doc.view.opacity)?;
                surface.visible = selected == field;
                if !surface.indices.is_empty() {
                    doc.surfaces.push(surface);
                }
            }
        }
        let path = output.join(format!("{}.eigenvista", doc.id));
        let bytes = encode(&doc)?;
        save_atomic(&path, &bytes)?;
        println!("{}: {} cached surfaces", path.display(), doc.surfaces.len());
        if field == "spatial-5" {
            let legacy =
                support::with_profile(&bytes, "molekel", &["molekel-preview-polynomial-v1"]);
            assert_eq!(support::manifest(&legacy)["format"], "molekel");
            let path = output.join("legacy-water.molekel");
            save_atomic(&path, &legacy)?;
            println!(
                "{}: {} legacy cached surfaces",
                path.display(),
                doc.surfaces.len()
            );
        }
    }
    Ok(())
}
