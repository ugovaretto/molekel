//! Bounded, shared file detection and conversion for the viewer and command line.

mod molden;
#[cfg(test)]
mod tests;

use molekel_core::{Document, import};
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};

pub const MAX_IMPORT_BYTES: usize = 128 * 1024 * 1024;

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ImportReport {
    pub format: String,
    pub warnings: Vec<String>,
}

#[derive(Clone, Debug, Serialize, Deserialize)]
pub struct ImportResult {
    pub document: Document,
    pub report: ImportReport,
}

fn native(bytes: &[u8]) -> Result<ImportResult, String> {
    Ok(ImportResult {
        document: molekel_format::decode(bytes)?,
        report: ImportReport {
            format: "molekel".into(),
            warnings: vec![],
        },
    })
}

/// Detect supported input without relying on a particular Molden extension.
/// Native documents retain their identity and provenance without modification.
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
        },
    })
}
