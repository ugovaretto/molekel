//! Bounded, shared file detection and conversion for the viewer and command line.

mod molden;
#[cfg(test)]
mod tests;

use molekel_core::{Document, bonds, import};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use std::collections::BTreeSet;

pub const MAX_IMPORT_BYTES: usize = 128 * 1024 * 1024;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ImportReport {
    pub format: String,
    pub warnings: Vec<String>,
    #[serde(default)]
    pub requires_save: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ImportResult {
    pub document: Document,
    pub report: ImportReport,
}

fn native(bytes: &[u8]) -> Result<ImportResult, String> {
    let mut document = molekel_format::decode(bytes)?;
    let connectivity = bonds::perceive(&document.atoms, &document.bonds)?;
    let existing: BTreeSet<_> = document
        .bonds
        .iter()
        .map(|&[a, b]| [a.min(b), a.max(b)])
        .collect();
    let missing: Vec<_> = connectivity
        .bonds
        .iter()
        .filter(|bond| !existing.contains(*bond))
        .copied()
        .collect();
    let requires_save = !missing.is_empty();
    let mut warnings = vec![];
    if requires_save {
        warnings.push(format!(
            "Added {} missing display bonds from atom positions; existing bonds and scientific data were retained. Save to keep the added bonds.",
            missing.len()
        ));
        document.provenance.push(warnings[0].clone());
        document.provenance.push(connectivity.description());
        document.bonds.extend(missing);
        document.validate()?;
    }
    Ok(ImportResult {
        document,
        report: ImportReport {
            format: "molekel".into(),
            warnings,
            requires_save,
        },
    })
}

/// Detect supported input without relying on a particular Molden extension.
/// Native documents retain scientific data and explicit bonds; missing display
/// bonds are supplemented and reported as a change requiring an explicit save.
pub fn import_bytes(bytes: &[u8], name: &str) -> Result<ImportResult, String> {
    if bytes.is_empty() || bytes.len() > MAX_IMPORT_BYTES {
        return Err("Input is empty or exceeds 128 MiB".into());
    }
    let suffix = name.rsplit('.').next().unwrap_or("").to_ascii_lowercase();
    if bytes.starts_with(b"PK\x03\x04") {
        return native(bytes);
    }
    let text = match std::str::from_utf8(bytes) {
        Ok(text) => text.trim_start_matches('\u{feff}'),
        Err(_) if suffix == "molekel" => return native(bytes),
        Err(_) => return Err("Text molecular imports require UTF-8/ASCII data".into()),
    };
    let first = text
        .lines()
        .find(|line| !line.trim().is_empty())
        .unwrap_or("")
        .trim();
    let (mut document, format, warnings) = if first.eq_ignore_ascii_case("[molden format]")
        || matches!(suffix.as_str(), "molden" | "mold" | "molden_input")
    {
        let (document, warnings) = molden::parse(text, name)?;
        (document, "molden", warnings)
    } else {
        match suffix.as_str() {
            "molekel" => return native(bytes),
            "xyz" => (import::xyz(text, name)?, "xyz", vec![]),
            "pdb" | "ent" => {
                let document = import::pdb(text, name)?;
                let warnings = document.provenance.iter().filter(|entry| entry.starts_with("Structure-only") || entry.starts_with("PDB parser:")).cloned().collect();
                (document, "pdb", warnings)
            }
            "cube" | "cub" => {
                let document = import::cube(text, name)?;
                (document, "cube", vec!["Cube scalar meaning and units are unspecified; only the sampled field is available.".into()])
            }
            _ => return Err("Unsupported file. Open .molekel, Molden, .xyz, .pdb, or .cube/.cub; Molden content is detected by [Molden Format].".into()),
        }
    };
    document.provenance.push(format!(
        "Import source: {name}; SHA-256 {:x}; molekel-import {}.",
        Sha256::digest(bytes),
        env!("CARGO_PKG_VERSION")
    ));
    document.provenance.extend(
        warnings
            .iter()
            .map(|warning| format!("Import warning: {warning}")),
    );
    document.validate()?;
    Ok(ImportResult {
        document,
        report: ImportReport {
            format: format.into(),
            warnings,
            requires_save: true,
        },
    })
}
