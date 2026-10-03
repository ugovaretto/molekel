use crate::{Atom, BOHR_TO_ANGSTROM, Document, MAX_BONDS, Result, bonds};
use pdbtbx::{Element, Format, ReadOptions, StrictnessLevel};
use std::collections::{HashMap, HashSet};
use std::io::BufReader;

fn column(line: &str, start: usize, end: usize) -> &str {
    line.get(start..end).unwrap_or("")
}

fn serial(text: &str) -> Result<usize> {
    text.trim()
        .parse::<usize>()
        .ok()
        .filter(|n| (1..=99_999).contains(n))
        .ok_or_else(|| {
            format!("Invalid PDB atom serial {text:?}; decimal serials 1..99999 are required")
        })
}

fn element(line: &str) -> Result<(Element, bool)> {
    let explicit = column(line, 76, 78).trim();
    let name = column(line, 12, 16);
    let symbol = if !explicit.is_empty() {
        explicit
    } else if name.starts_with(' ') || name.as_bytes().first().is_some_and(u8::is_ascii_digit) {
        let first = name
            .find(|c: char| c.is_ascii_alphabetic())
            .ok_or("Missing PDB element/atom name")?;
        &name[first..first + 1]
    } else if name.len() == 4 && name.as_bytes()[0] == b'H' && !name.ends_with(' ') {
        &name[..1]
    } else if Element::from_symbol(column(name, 0, 2)).is_some() {
        &name[..2]
    } else {
        column(name, 0, 1)
    };
    if symbol.eq_ignore_ascii_case("D") || symbol.eq_ignore_ascii_case("T") {
        return Ok((Element::H, true));
    }
    Element::from_symbol(symbol)
        .map(|e| (e, false))
        .ok_or_else(|| format!("Unknown PDB element {symbol:?} for atom {name:?}"))
}

/// Read the first geometry, select one occupancy-ranked conformer per residue,
/// retain explicit connectivity, then infer missing connections from coordinates.
pub fn pdb(text: &str, name: &str) -> Result<Document> {
    if text.len() > 32 * 1024 * 1024 {
        return Err("PDB exceeds the 32 MiB preview file budget".into());
    }
    if !text.is_ascii() {
        return Err("PDB fixed-column records must contain ASCII text".into());
    }
    let mut normalized = String::new();
    let mut atom_order = HashMap::new();
    let mut connections = Vec::new();
    let mut title = Vec::new();
    let mut model = 0;
    let mut first_model = String::from("implicit first model");
    let mut in_model = false;
    let mut first_closed = false;
    let mut isotope_count = 0;
    let mut legacy_count = 0;
    let mut legacy_names = 0;
    let mut inferred_elements = 0;
    let mut legacy_id = None;
    for (index, line) in text.lines().enumerate() {
        let row_number = index + 1;
        let record = line.get(..6).unwrap_or(line).trim();
        let selected = model <= 1 && !first_closed;
        match record {
            "HEADER" => {
                let id = column(line, 62, 66);
                if id.len() == 4
                    && !id.trim().is_empty()
                    && column(line, 72, 76) == id
                    && column(line, 76, 80).trim().parse::<usize>().is_ok()
                {
                    legacy_id = Some(id);
                }
            }
            "TITLE" => title.push(line.get(10..).unwrap_or("").trim().to_string()),
            "MODEL" => {
                if in_model || (model == 0 && !atom_order.is_empty()) {
                    return Err(format!("Ambiguous PDB MODEL boundary at line {row_number}"));
                }
                model += 1;
                in_model = true;
                if model == 1 {
                    first_model = format!("MODEL {}", column(line, 10, 14).trim());
                }
            }
            "ENDMDL" => {
                first_closed = true;
                in_model = false;
            }
            "END" => break,
            "TER" if selected => normalized.push_str("TER   \n"),
            "ATOM" | "HETATM" if selected => {
                if line.len() < 54 || line.bytes().any(|b| !b.is_ascii_graphic() && b != b' ') {
                    return Err(format!(
                        "Truncated or invalid PDB atom at line {row_number}"
                    ));
                }
                let id = serial(column(line, 6, 11))?;
                if atom_order.len() == 100_000 {
                    return Err("PDB exceeds the 100000 atom budget".into());
                }
                if atom_order.insert(id, atom_order.len()).is_some() {
                    return Err(format!(
                        "Duplicate PDB atom serial {id} in the selected model"
                    ));
                }
                // Preflight finite/range constraints before the library's Atom constructor.
                for (start, end) in [(30, 38), (38, 46), (46, 54)] {
                    let value = column(line, start, end)
                        .trim()
                        .parse::<f64>()
                        .map_err(|_| format!("Invalid PDB coordinate at line {row_number}"))?;
                    if !value.is_finite() {
                        return Err(format!("Non-finite PDB coordinate at line {row_number}"));
                    }
                }
                if column(line, 12, 16).trim().is_empty() || column(line, 17, 20).trim().is_empty()
                {
                    return Err(format!(
                        "Missing PDB atom or residue name at line {row_number}"
                    ));
                }
                let mut row = format!("{:<80}", &line[..line.len().min(80)]);
                if legacy_id.is_some_and(|id| column(line, 72, 76) == id)
                    && column(line, 76, 80).trim().parse::<usize>().is_ok()
                {
                    row.replace_range(72..80, "        ");
                    legacy_count += 1;
                }
                for (start, end, default) in [(54, 60, "  1.00"), (60, 66, "  0.00")] {
                    if column(&row, start, end).trim().is_empty() {
                        row.replace_range(start..end, default);
                    }
                    let value = column(&row, start, end)
                        .trim()
                        .parse::<f64>()
                        .map_err(|_| {
                            format!("Invalid PDB occupancy/B-factor at line {row_number}")
                        })?;
                    if !value.is_finite() || value < 0. || (start == 54 && value > 1.) {
                        return Err(format!(
                            "Invalid PDB occupancy/B-factor at line {row_number}"
                        ));
                    }
                }
                // Older nucleotide files place the fifth character of " H2'1"
                // in altLoc. Restrict this repair to full-occupancy, untyped H names.
                if column(&row, 76, 78).trim().is_empty()
                    && row.as_bytes()[12] == b' '
                    && row.as_bytes()[13] == b'H'
                    && matches!(row.as_bytes()[15], b'\'' | b'*')
                    && row.as_bytes()[16].is_ascii_digit()
                    && column(&row, 54, 60).trim().parse::<f64>().ok() == Some(1.)
                {
                    let name = row[13..17].to_string();
                    row.replace_range(12..17, &format!("{name} "));
                    legacy_names += 1;
                }
                if column(&row, 76, 78).trim().is_empty() {
                    inferred_elements += 1;
                }
                let (element, isotope) =
                    element(&row).map_err(|e| format!("{e}, line {row_number}"))?;
                isotope_count += usize::from(isotope);
                row.replace_range(76..78, &format!("{:>2}", element.symbol()));
                normalized.push_str(&row);
                normalized.push('\n');
            }
            "CONECT" if selected || !in_model => {
                let from = serial(column(line, 6, 11))?;
                // Only columns 12-31 are covalent bond endpoints; old hydrogen/salt fields are not bonds.
                for start in [11, 16, 21, 26] {
                    let endpoint = line
                        .get(start..line.len().min(start + 5))
                        .unwrap_or("")
                        .trim();
                    if endpoint.is_empty() {
                        continue;
                    }
                    if connections.len() == MAX_BONDS {
                        return Err("PDB CONECT exceeds the connection budget".into());
                    }
                    let to = serial(endpoint)?;
                    if to == from {
                        return Err(format!("PDB CONECT self-bond at atom {from}"));
                    }
                    connections.push([from, to]);
                }
            }
            _ => (),
        }
    }
    if atom_order.is_empty() {
        return Err("No atoms found in the first PDB model".into());
    }
    let (structure, warnings) = ReadOptions::new()
        .set_format(Format::Pdb)
        .set_level(StrictnessLevel::Loose)
        .set_only_atomic_coords(true)
        .read_raw(BufReader::new(normalized.as_bytes()))
        .map_err(|errors| {
            format!(
                "Invalid PDB: {}",
                errors
                    .iter()
                    .take(3)
                    .map(ToString::to_string)
                    .collect::<Vec<_>>()
                    .join("; ")
            )
        })?;
    let mut selected_atoms = Vec::new();
    let mut discarded = 0;
    for residue in structure.residues() {
        let preferred = residue
            .conformers()
            .filter(|c| c.alternative_location().is_some())
            .map(|c| {
                let occupancy =
                    c.atoms().map(|a| a.occupancy()).sum::<f64>() / c.atom_count().max(1) as f64;
                (occupancy, c.alternative_location().unwrap(), c.name())
            })
            .max_by(|a, b| a.0.total_cmp(&b.0).then(b.1.cmp(a.1)).then(b.2.cmp(a.2)));
        let mut names = HashSet::new();
        for conformer in residue.conformers() {
            let keep = conformer.alternative_location().is_none()
                || preferred.is_some_and(|(_, alt, name)| {
                    conformer.alternative_location() == Some(alt) && conformer.name() == name
                });
            for atom in conformer.atoms() {
                if !keep || atom.occupancy() == 0. {
                    discarded += 1;
                    continue;
                }
                if !names.insert(atom.name()) {
                    return Err(format!(
                        "Duplicate PDB atom name {} in selected residue {}",
                        atom.name(),
                        residue.serial_number()
                    ));
                }
                selected_atoms.push(atom);
            }
        }
    }
    selected_atoms.sort_unstable_by_key(|a| atom_order[&a.serial_number()]);
    let title = title.join(" ");
    let mut doc = Document::empty(if title.is_empty() { name } else { &title });
    let mut serial_to_index = HashMap::new();
    for atom in selected_atoms {
        serial_to_index.insert(atom.serial_number(), doc.atoms.len());
        doc.atoms.push(Atom {
            element: atom
                .element()
                .ok_or("PDB atom has no element")?
                .atomic_number() as u16,
            position: [atom.x(), atom.y(), atom.z()].map(|v| v / BOHR_TO_ANGSTROM),
        });
    }
    if doc.atoms.is_empty() {
        return Err("PDB has no atoms after conformer/occupancy selection".into());
    }
    let mut explicit = Vec::new();
    let mut omitted_connections = 0;
    for [a, b] in connections {
        match (serial_to_index.get(&a), serial_to_index.get(&b)) {
            (Some(&a), Some(&b)) => explicit.push([a, b]),
            _ if atom_order.contains_key(&a) && atom_order.contains_key(&b) => {
                omitted_connections += 1
            }
            _ => {
                return Err(format!(
                    "PDB CONECT references an unknown atom serial: {a}, {b}"
                ));
            }
        }
    }
    explicit.iter_mut().for_each(|pair| pair.sort_unstable());
    explicit.sort_unstable();
    explicit.dedup();
    let connectivity = bonds::perceive(&doc.atoms, &explicit)?;
    doc.provenance = vec![
        format!("Imported {name}; PDB coordinates in angstrom converted to bohr. Loaded {first_model} only; later models/concatenated structures are not merged."),
        format!("Alternate locations: highest mean occupancy per residue, alphabetic label on ties, plus common atoms; omitted {discarded} alternate/zero-occupancy atoms. Preserved {} unique CONECT bonds; omitted {omitted_connections} connections to unselected alternatives.", explicit.len()),
        connectivity.description(),
        format!("Element inferred from aligned atom names for {inferred_elements} atoms. Recognized {legacy_count} legacy PDB identifier/line-number suffixes. Converted {isotope_count} D/T isotope labels to hydrogen (isotope identity not retained)."),
        format!("Normalized {legacy_names} legacy five-column nucleotide hydrogen names; numeric alternate labels with partial occupancy are not repaired."),
        "Structure-only import: residue/chain labels, occupancy, B-factors, formal charges, crystal cells and symmetry are not retained in the preview model; no quantum data fabricated.".into(),
    ];
    for warning in warnings.iter().take(5) {
        doc.provenance
            .push(format!("PDB parser: {}", warning.short_description()));
    }
    doc.bonds = connectivity.bonds;
    doc.validate()?;
    Ok(doc)
}
