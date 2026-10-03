use molekel_core::{
    Atom, BOHR_TO_ANGSTROM, BasisFunction, Density, Document, Orbital, Term, bonds,
};
use std::collections::{HashMap, HashSet};

const MAX_LINES: usize = 500_000;
const MAX_LINE_BYTES: usize = 16_384;
const MAX_BASIS: usize = 256;
const MAX_ORBITALS: usize = 512;
const ELEMENTS: &str = "H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn Fe Co Ni Cu Zn Ga Ge As Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag Cd In Sn Sb Te I Xe Cs Ba La Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr Ra Ac Th Pa U Np Pu Am Cm Bk Cf Es Fm Md No Lr Rf Db Sg Bh Hs Mt Ds Rg Cn Nh Fl Mc Lv Ts Og";

#[derive(Clone, Copy)]
struct Line<'a> {
    number: usize,
    text: &'a str,
}

impl Line<'_> {
    fn error(self, message: impl AsRef<str>) -> String {
        format!("Molden line {}: {}", self.number, message.as_ref())
    }
}

struct Section<'a> {
    line: Line<'a>,
    name: String,
    argument: &'a str,
    rows: Vec<Line<'a>>,
}

fn number(token: &str, line: Line<'_>) -> Result<f64, String> {
    let value: f64 = token
        .replace(['d', 'D'], "E")
        .parse()
        .map_err(|_| line.error(format!("Invalid number: {token}")))?;
    if !value.is_finite() {
        return Err(line.error("Non-finite scientific value"));
    }
    Ok(value)
}

fn integer(token: &str, line: Line<'_>) -> Result<usize, String> {
    token
        .parse()
        .map_err(|_| line.error(format!("Invalid integer: {token}")))
}

fn sections(text: &str) -> Result<Vec<Section<'_>>, String> {
    let mut result: Vec<Section<'_>> = vec![];
    for (index, text) in text.lines().enumerate() {
        let line = Line {
            number: index + 1,
            text: text.trim(),
        };
        if index >= MAX_LINES || text.len() > MAX_LINE_BYTES {
            return Err(line.error("Input exceeds the Molden line/count budget"));
        }
        if line.text.is_empty() || line.text.starts_with(['#', '!']) {
            continue;
        }
        if line.text.starts_with('[') {
            let end = line
                .text
                .find(']')
                .ok_or_else(|| line.error("Unclosed section header"))?;
            if result.len() >= 128 {
                return Err(line.error("Too many Molden sections"));
            }
            result.push(Section {
                line,
                name: line.text[1..end].trim().to_ascii_uppercase(),
                argument: line.text[end + 1..].trim(),
                rows: vec![],
            });
        } else {
            result
                .last_mut()
                .ok_or_else(|| line.error("Expected [Molden Format] header"))?
                .rows
                .push(line);
        }
    }
    if result
        .first()
        .is_none_or(|section| section.name != "MOLDEN FORMAT")
    {
        return Err("Molden input must start with [Molden Format]".into());
    }
    Ok(result)
}

fn exactly_one<'a, 'b>(sections: &'a [Section<'b>], name: &str) -> Result<&'a Section<'b>, String> {
    let mut matches = sections.iter().filter(|section| section.name == name);
    let first = matches
        .next()
        .ok_or_else(|| format!("Molden requires [{name}] for this import profile"))?;
    if let Some(duplicate) = matches.next() {
        return Err(duplicate.line.error(format!(
            "Repeated [{name}] is ambiguous; export one geometry/wavefunction"
        )));
    }
    Ok(first)
}

pub(super) fn parse(text: &str, name: &str) -> Result<(Document, Vec<String>), String> {
    let sections = sections(text)?;
    exactly_one(&sections, "MOLDEN FORMAT")?;
    let mut warnings = vec![];
    let producer_lines = sections
        .iter()
        .filter(|s| matches!(s.name.as_str(), "MOLDEN FORMAT" | "TITLE"))
        .flat_map(|section| &section.rows);
    let orca = producer_lines
        .clone()
        .any(|line| line.text.to_ascii_lowercase().contains("orca_2mkl"));
    if producer_lines
        .clone()
        .any(|line| line.text.to_ascii_lowercase().contains("orca"))
        && !orca
    {
        return Err("Unrecognized ORCA Molden producer profile; an identifiable orca_2mkl export is required for its normalization convention.".into());
    }
    if let Some(section) = sections.iter().find(|section| section.name == "STO") {
        return Err(section
            .line
            .error("Slater [STO] bases are unsupported; export a Gaussian [GTO] wavefunction"));
    }
    let atoms = exactly_one(&sections, "ATOMS")?;
    let gto = exactly_one(&sections, "GTO")?;
    let mut document = Document::empty(name);
    if let Some(title) = sections.iter().find(|section| section.name == "TITLE") {
        let title: String = title
            .rows
            .iter()
            .enumerate()
            .flat_map(|(index, line)| {
                std::iter::once(' ')
                    .filter(move |_| index > 0)
                    .chain(line.text.chars())
            })
            .take(1024)
            .collect();
        if !title.is_empty() {
            document.title = title;
        }
    }
    let atom_ids = parse_atoms(atoms, &mut document)?;
    let modes = modes(&sections)?;
    parse_basis(gto, &atom_ids, modes, orca, &mut document, &mut warnings)?;
    let mo_sections: Vec<_> = sections
        .iter()
        .filter(|section| section.name == "MO")
        .collect();
    if mo_sections.is_empty() {
        return Err(
            "Molden requires [MO]; geometry-only/vibration Molden profiles are not supported"
                .into(),
        );
    }
    for section in mo_sections {
        parse_orbitals(section, &mut document)?;
    }
    finish_orbitals(&mut document, &mut warnings)?;
    let mut ignored = HashSet::new();
    for section in &sections {
        if matches!(
            section.name.as_str(),
            "MOLDEN FORMAT" | "TITLE" | "ATOMS" | "GTO" | "MO"
        ) || is_mode(&section.name)
        {
            continue;
        }
        if ignored.insert(section.name.clone()) {
            warnings.push(format!(
                "[{}] data are not retained in this preview (line {}).",
                section.name, section.line.number
            ));
        }
    }
    let connectivity = bonds::perceive(&document.atoms, &[])?;
    document.provenance.push(connectivity.description());
    document.bonds = connectivity.bonds;
    document.provenance.push(format!(
        "Molden canonical Gaussian profile v1: normalized primitives and contractions; D={}, F={}, G={}; coordinates converted to bohr; energies in hartree; real coefficients in declared Molden component order.",
        if modes[0] { "5 spherical" } else { "6 Cartesian" },
        if modes[1] { "7 spherical" } else { "10 Cartesian" },
        if modes[2] { "9 spherical" } else { "15 Cartesian" },
    ));
    if orca {
        warnings.push("Applied the identified orca_2mkl S/P/5D primitive-normalization profile; higher-angular-momentum and Cartesian D exports are not yet qualified.".into());
        document.provenance.push("Producer profile: orca_2mkl S/P/5D v1; primitive normalization converted explicitly before canonical AO construction.".into());
    }
    document.validate()?;
    Ok((document, warnings))
}

fn parse_atoms(
    section: &Section<'_>,
    document: &mut Document,
) -> Result<HashMap<usize, usize>, String> {
    let units = section.argument.trim_matches(['(', ')']).trim();
    let scale = if units.eq_ignore_ascii_case("AU") {
        1.
    } else if units.eq_ignore_ascii_case("Angs") {
        1. / BOHR_TO_ANGSTROM
    } else {
        return Err(section
            .line
            .error("[Atoms] must explicitly specify AU or Angs"));
    };
    let elements: Vec<_> = ELEMENTS.split_whitespace().collect();
    let mut ids = HashMap::new();
    for &line in &section.rows {
        let row: Vec<_> = line.text.split_whitespace().collect();
        if row.len() != 6 {
            return Err(
                line.error("Atom needs symbol, index, atomic number, and three coordinates")
            );
        }
        if document.atoms.len() >= 100_000 {
            return Err(line.error("Atom count exceeds 100000"));
        }
        let id = integer(row[1], line)?;
        let element = integer(row[2], line)?;
        if !(1..=118).contains(&element) || id == 0 {
            return Err(line.error("Only real atoms with positive IDs and atomic numbers 1..118 are supported (no ghost/ECP charge substitution)"));
        }
        let symbol = row[0].trim_end_matches(|c: char| c.is_ascii_digit());
        if !elements[element - 1].eq_ignore_ascii_case(symbol) {
            return Err(line.error("Atom symbol and atomic number disagree"));
        }
        if ids.insert(id, document.atoms.len()).is_some() {
            return Err(line.error("Duplicate atom index"));
        }
        document.atoms.push(Atom {
            element: element as u16,
            position: [
                number(row[3], line)? * scale,
                number(row[4], line)? * scale,
                number(row[5], line)? * scale,
            ],
        });
    }
    if document.atoms.is_empty() {
        return Err(section.line.error("Empty [Atoms] section"));
    }
    Ok(ids)
}

fn is_mode(name: &str) -> bool {
    matches!(
        name,
        "5D" | "6D" | "7F" | "10F" | "9G" | "15G" | "5D7F" | "5D10F" | "6D7F" | "6D10F"
    )
}

fn modes(sections: &[Section<'_>]) -> Result<[bool; 3], String> {
    let mut explicit = [None; 3];
    let mut implicit_f = false;
    for section in sections {
        if !is_mode(&section.name) {
            if section
                .name
                .chars()
                .next()
                .is_some_and(|c| c.is_ascii_digit())
            {
                return Err(section
                    .line
                    .error("Unsupported or ambiguous angular-component convention"));
            }
            continue;
        }
        if !section.rows.is_empty() || !section.argument.is_empty() {
            return Err(section
                .line
                .error("Angular-component flag must be on its own line"));
        }
        let choices: &[(usize, bool)] = match section.name.as_str() {
            "5D" => {
                implicit_f = true;
                &[(0, true)]
            }
            "6D" => &[(0, false)],
            "7F" => &[(1, true)],
            "10F" => &[(1, false)],
            "9G" => &[(2, true)],
            "15G" => &[(2, false)],
            "5D7F" => &[(0, true), (1, true)],
            "5D10F" => &[(0, true), (1, false)],
            "6D7F" => &[(0, false), (1, true)],
            "6D10F" => &[(0, false), (1, false)],
            _ => unreachable!(),
        };
        for &(index, value) in choices {
            if explicit[index].is_some_and(|previous| previous != value) {
                return Err(section.line.error("Conflicting angular-component flags"));
            }
            explicit[index] = Some(value);
        }
    }
    Ok([
        explicit[0].unwrap_or(false),
        explicit[1].unwrap_or(implicit_f),
        explicit[2].unwrap_or(false),
    ])
}

fn parse_basis(
    section: &Section<'_>,
    atom_ids: &HashMap<usize, usize>,
    modes: [bool; 3],
    orca: bool,
    document: &mut Document,
    warnings: &mut Vec<String>,
) -> Result<(), String> {
    if !section.argument.is_empty() {
        return Err(section.line.error("Unsupported [GTO] header options"));
    }
    let mut rows = section.rows.iter().copied();
    let mut center = None;
    let mut seen_atoms = HashSet::new();
    let mut renormalized = false;
    while let Some(line) = rows.next() {
        let row: Vec<_> = line.text.split_whitespace().collect();
        if row[0].chars().all(|c| c.is_ascii_digit()) {
            if row.len() > 2 || (row.len() == 2 && number(row[1], line)? != 0.) {
                return Err(line.error("GTO atom header must contain its index and optional zero"));
            }
            let id = integer(row[0], line)?;
            let index = atom_ids
                .get(&id)
                .ok_or_else(|| line.error("GTO references an unknown atom index"))?;
            if !seen_atoms.insert(id) {
                return Err(line.error("Repeated GTO atom block is ambiguous"));
            }
            center = Some(document.atoms[*index].position);
            continue;
        }
        let center = center.ok_or_else(|| line.error("Shell occurs before its GTO atom header"))?;
        if !(2..=3).contains(&row.len()) {
            return Err(line.error("Shell needs label, primitive count, and optional 1.0"));
        }
        if row.len() == 3 && number(row[2], line)? != 1. {
            return Err(line.error("Non-unit shell scale is ambiguous across Molden producers; export explicit coefficients"));
        }
        let label = row[0].to_ascii_lowercase();
        let angular = match label.as_str() {
            "s" | "sp" => 0,
            "p" => 1,
            "d" => 2,
            "f" => 3,
            "g" => 4,
            _ => return Err(line.error("Only Gaussian S, P, D, F, G, and SP shells are supported")),
        };
        if orca && (angular > 2 || (angular == 2 && !modes[0]) || label == "sp") {
            return Err(line.error("This orca_2mkl profile is qualified only for S/P/spherical 5D shells; SP, Cartesian D, and F/G need independent producer-specific validation"));
        }
        let count = integer(row[1], line)?;
        if !(1..=64).contains(&count) {
            return Err(line.error("Primitive count must be 1..64"));
        }
        let mut exponents = Vec::with_capacity(count);
        let mut coefficients = Vec::with_capacity(count);
        let mut p_coefficients = Vec::with_capacity(count);
        for _ in 0..count {
            let primitive = rows
                .next()
                .ok_or_else(|| line.error("Truncated primitive list"))?;
            let values: Vec<_> = primitive.text.split_whitespace().collect();
            if values.len() != if label == "sp" { 3 } else { 2 } {
                return Err(primitive.error("Primitive needs exponent and one coefficient (two for SP); generalized columns are unsupported"));
            }
            let exponent = number(values[0], primitive)?;
            if exponent <= 0. {
                return Err(primitive.error("Gaussian exponent must be positive"));
            }
            exponents.push(exponent);
            let mut coefficient = number(values[1], primitive)?;
            if orca {
                let powers = match angular {
                    0 => [0, 0, 0],
                    1 => [1, 0, 0],
                    2 => [1, 1, 0],
                    _ => unreachable!(),
                };
                coefficient *= polynomial_overlap(&[term(powers, 1.)], 2. * exponent).sqrt();
            }
            coefficients.push(coefficient);
            if label == "sp" {
                p_coefficients.push(number(values[2], primitive)?);
            }
        }
        renormalized |= append_shell(
            document,
            center,
            &exponents,
            &coefficients,
            angular,
            angular >= 2 && modes[angular - 2],
            line,
        )?;
        if label == "sp" {
            renormalized |= append_shell(
                document,
                center,
                &exponents,
                &p_coefficients,
                1,
                false,
                line,
            )?;
        }
    }
    if document.basis.is_empty() {
        return Err(section.line.error("Empty [GTO] section"));
    }
    if renormalized {
        warnings.push("Gaussian contractions were normalized under the canonical Molden convention; noncanonical producer normalization is not inferred.".into());
    }
    Ok(())
}

fn term(powers: [u32; 3], weight: f64) -> Term {
    Term { powers, weight }
}

fn components(angular: usize, spherical: bool) -> Vec<Vec<Term>> {
    if spherical {
        let polynomials: Vec<Vec<([u32; 3], f64)>> = match angular {
            2 => vec![
                vec![([0, 0, 2], 2.), ([2, 0, 0], -1.), ([0, 2, 0], -1.)],
                vec![([1, 0, 1], 1.)],
                vec![([0, 1, 1], 1.)],
                vec![([2, 0, 0], 1.), ([0, 2, 0], -1.)],
                vec![([1, 1, 0], 1.)],
            ],
            3 => vec![
                vec![([0, 0, 3], 2.), ([2, 0, 1], -3.), ([0, 2, 1], -3.)],
                vec![([1, 0, 2], 4.), ([3, 0, 0], -1.), ([1, 2, 0], -1.)],
                vec![([0, 1, 2], 4.), ([2, 1, 0], -1.), ([0, 3, 0], -1.)],
                vec![([2, 0, 1], 1.), ([0, 2, 1], -1.)],
                vec![([1, 1, 1], 1.)],
                vec![([3, 0, 0], 1.), ([1, 2, 0], -3.)],
                vec![([2, 1, 0], 3.), ([0, 3, 0], -1.)],
            ],
            4 => vec![
                vec![
                    ([0, 0, 4], 8.),
                    ([2, 0, 2], -24.),
                    ([0, 2, 2], -24.),
                    ([4, 0, 0], 3.),
                    ([2, 2, 0], 6.),
                    ([0, 4, 0], 3.),
                ],
                vec![([1, 0, 3], 4.), ([3, 0, 1], -3.), ([1, 2, 1], -3.)],
                vec![([0, 1, 3], 4.), ([2, 1, 1], -3.), ([0, 3, 1], -3.)],
                vec![
                    ([2, 0, 2], 6.),
                    ([0, 2, 2], -6.),
                    ([4, 0, 0], -1.),
                    ([0, 4, 0], 1.),
                ],
                vec![([1, 1, 2], 6.), ([3, 1, 0], -1.), ([1, 3, 0], -1.)],
                vec![([3, 0, 1], 1.), ([1, 2, 1], -3.)],
                vec![([2, 1, 1], 3.), ([0, 3, 1], -1.)],
                vec![([4, 0, 0], 1.), ([2, 2, 0], -6.), ([0, 4, 0], 1.)],
                vec![([3, 1, 0], 1.), ([1, 3, 0], -1.)],
            ],
            _ => unreachable!(),
        };
        return polynomials
            .into_iter()
            .map(|terms| terms.into_iter().map(|(p, w)| term(p, w)).collect())
            .collect();
    }
    let powers: &[[u32; 3]] = match angular {
        0 => &[[0, 0, 0]],
        1 => &[[1, 0, 0], [0, 1, 0], [0, 0, 1]],
        2 => &[
            [2, 0, 0],
            [0, 2, 0],
            [0, 0, 2],
            [1, 1, 0],
            [1, 0, 1],
            [0, 1, 1],
        ],
        3 => &[
            [3, 0, 0],
            [0, 3, 0],
            [0, 0, 3],
            [1, 2, 0],
            [2, 1, 0],
            [2, 0, 1],
            [1, 0, 2],
            [0, 1, 2],
            [0, 2, 1],
            [1, 1, 1],
        ],
        4 => &[
            [4, 0, 0],
            [0, 4, 0],
            [0, 0, 4],
            [3, 1, 0],
            [3, 0, 1],
            [1, 3, 0],
            [0, 3, 1],
            [1, 0, 3],
            [0, 1, 3],
            [2, 2, 0],
            [2, 0, 2],
            [0, 2, 2],
            [2, 1, 1],
            [1, 2, 1],
            [1, 1, 2],
        ],
        _ => unreachable!(),
    };
    powers
        .iter()
        .map(|powers| vec![term(*powers, 1.)])
        .collect()
}

fn moment(power: u32, exponent: f64) -> f64 {
    if !power.is_multiple_of(2) {
        return 0.;
    }
    let mut integral = (std::f64::consts::PI / exponent).sqrt();
    for n in 1..=power / 2 {
        integral *= f64::from(2 * n - 1) / (2. * exponent);
    }
    integral
}

fn polynomial_overlap(terms: &[Term], exponent: f64) -> f64 {
    terms
        .iter()
        .flat_map(|left| {
            terms.iter().map(move |right| {
                left.weight
                    * right.weight
                    * (0..3)
                        .map(|axis| moment(left.powers[axis] + right.powers[axis], exponent))
                        .product::<f64>()
            })
        })
        .sum()
}

fn append_shell(
    document: &mut Document,
    center: [f64; 3],
    exponents: &[f64],
    coefficients: &[f64],
    angular: usize,
    spherical: bool,
    line: Line<'_>,
) -> Result<bool, String> {
    let components = components(angular, spherical);
    if document.basis.len() + components.len() > MAX_BASIS {
        return Err(line.error("Basis exceeds the preview limit of 256 atomic orbitals"));
    }
    // Normalized primitives of equal angular degree have a component-independent overlap.
    let mut norm = 0.;
    for (i, &a) in exponents.iter().enumerate() {
        for (j, &b) in exponents.iter().enumerate() {
            norm += coefficients[i]
                * coefficients[j]
                * (2. * (a.sqrt() * b.sqrt()) / (a + b)).powf(angular as f64 + 1.5);
        }
    }
    if !norm.is_finite() || norm <= 1e-24 {
        return Err(
            line.error("Gaussian contraction has zero, unstable, or non-finite normalization")
        );
    }
    for terms in components {
        let radial: Result<Vec<_>, String> = exponents
            .iter()
            .zip(coefficients)
            .map(|(&a, &c)| {
                let integral = polynomial_overlap(&terms, 2. * a);
                if !integral.is_finite() || integral <= 0. {
                    return Err(line.error("Gaussian primitive normalization overflow/underflow"));
                }
                Ok(c / (norm * integral).sqrt())
            })
            .collect();
        let radial = radial?;
        if radial.iter().any(|value| !value.is_finite()) {
            return Err(line.error("Gaussian normalization overflow/underflow"));
        }
        document.basis.push(BasisFunction {
            center,
            exponents: exponents.to_vec(),
            coefficients: radial,
            terms,
        });
    }
    Ok((norm - 1.).abs() > 1e-5)
}

#[derive(Default)]
struct OrbitalBlock {
    sym: Option<String>,
    energy: Option<f64>,
    spin: Option<String>,
    occupation: Option<f64>,
    values: HashMap<usize, f64>,
    line: usize,
}

fn parse_orbitals(section: &Section<'_>, document: &mut Document) -> Result<(), String> {
    let mut block = OrbitalBlock::default();
    for &line in &section.rows {
        if let Some((key, value)) = line.text.split_once('=') {
            if !block.values.is_empty() {
                push_orbital(std::mem::take(&mut block), document)?;
            }
            if block.line == 0 {
                block.line = line.number;
            }
            let value = value.trim();
            if value.is_empty() || value.len() > 256 {
                return Err(line.error("Invalid or oversized orbital header value"));
            }
            match key.trim().to_ascii_lowercase().as_str() {
                "sym" => {
                    if block.sym.replace(value.into()).is_some() {
                        return Err(line.error("Repeated Sym before orbital coefficients"));
                    }
                }
                "ene" => {
                    if block.energy.replace(number(value, line)?).is_some() {
                        return Err(line.error("Repeated Ene before orbital coefficients"));
                    }
                }
                "spin" => {
                    let spin = value.to_ascii_lowercase();
                    if !matches!(spin.as_str(), "alpha" | "beta") {
                        return Err(line.error("Spin must be Alpha or Beta"));
                    }
                    if block.spin.replace(spin).is_some() {
                        return Err(line.error("Repeated Spin before orbital coefficients"));
                    }
                }
                "occup" => {
                    let occupation = number(value, line)?;
                    if !(0. ..=2.).contains(&occupation) {
                        return Err(line.error("Occupation must be between zero and two"));
                    }
                    if block.occupation.replace(occupation).is_some() {
                        return Err(line.error("Repeated Occup before orbital coefficients"));
                    }
                }
                _ => return Err(line.error("Unsupported molecular-orbital header")),
            }
        } else {
            if block.line == 0 {
                return Err(line.error("Orbital coefficients occur before MO headers"));
            }
            let row: Vec<_> = line.text.split_whitespace().collect();
            if row.len() != 2 {
                return Err(line.error("MO coefficient needs an AO index and real coefficient"));
            }
            let index = integer(row[0], line)?;
            if index == 0 || index > document.basis.len() {
                return Err(line.error("MO atomic-orbital index exceeds the declared basis"));
            }
            if block
                .values
                .insert(index - 1, number(row[1], line)?)
                .is_some()
            {
                return Err(line.error("Duplicate AO index within an orbital"));
            }
        }
    }
    if block.line != 0 {
        push_orbital(block, document)?;
    }
    Ok(())
}

fn push_orbital(block: OrbitalBlock, document: &mut Document) -> Result<(), String> {
    let error = |message| format!("Molden orbital starting at line {}: {message}", block.line);
    if document.orbitals.len() >= MAX_ORBITALS {
        return Err(error("More than 512 molecular orbitals"));
    }
    if block.values.is_empty() {
        return Err(error("Missing coefficients; truncated or empty orbital"));
    }
    let spin = block
        .spin
        .ok_or_else(|| error("Explicit Spin=Alpha/Beta is required"))?;
    let mut coefficients = vec![0.; document.basis.len()];
    for (index, value) in block.values {
        coefficients[index] = value;
    }
    if coefficients.iter().all(|value| *value == 0.) {
        return Err(error("All-zero molecular orbital"));
    }
    let index = document.orbitals.len() + 1;
    document.orbitals.push(Orbital {
        id: format!("molden-mo-{index}"),
        label: format!(
            "MO {index}{}",
            block
                .sym
                .filter(|s| !s.is_empty())
                .map(|s| format!(" ({s})"))
                .unwrap_or_default()
        ),
        spin,
        occupation: block.occupation,
        energy: block.energy,
        coefficients,
    });
    Ok(())
}

fn finish_orbitals(document: &mut Document, warnings: &mut Vec<String>) -> Result<(), String> {
    if document.orbitals.is_empty() {
        return Err("Molden [MO] section contains no orbitals".into());
    }
    let has_alpha = document
        .orbitals
        .iter()
        .any(|orbital| orbital.spin == "alpha");
    let has_beta = document
        .orbitals
        .iter()
        .any(|orbital| orbital.spin == "beta");
    if has_alpha && !has_beta {
        for orbital in &mut document.orbitals {
            orbital.spin = "spatial".into();
        }
        warnings.push("A single Alpha orbital set is treated as spatial orbitals using the supplied occupations; separate alpha/beta densities cannot be inferred.".into());
    } else if document
        .orbitals
        .iter()
        .any(|orbital| orbital.occupation.is_some_and(|value| value > 1.))
    {
        return Err("Molden spin-resolved Alpha/Beta occupations cannot exceed one".into());
    }
    if document
        .orbitals
        .iter()
        .any(|orbital| orbital.energy.is_none())
    {
        warnings.push("Some orbital energies are missing and remain unspecified.".into());
    }
    if document
        .orbitals
        .iter()
        .any(|orbital| orbital.occupation.is_none())
    {
        warnings.push(
            "Some occupations are missing; no occupation-derived density was constructed.".into(),
        );
    } else {
        let size = document.basis.len() * document.basis.len();
        let mut alpha = vec![0.; size];
        let mut beta = vec![0.; size];
        for orbital in &document.orbitals {
            let occupation = orbital.occupation.unwrap();
            let target = if orbital.spin == "beta" {
                &mut beta
            } else {
                &mut alpha
            };
            for (i, &left) in orbital.coefficients.iter().enumerate() {
                for (j, &right) in orbital.coefficients.iter().enumerate() {
                    target[i * document.basis.len() + j] += occupation * left * right;
                }
            }
        }
        if has_alpha && has_beta {
            let total = alpha.iter().zip(&beta).map(|(a, b)| a + b).collect();
            let spin = alpha.iter().zip(&beta).map(|(a, b)| a - b).collect();
            for (kind, matrix) in [
                ("total", total),
                ("alpha", alpha),
                ("beta", beta),
                ("spin", spin),
            ] {
                document.densities.push(Density {
                    id: format!("molden-density-{kind}"),
                    label: format!("Occupation-derived {kind} density (listed orbitals)"),
                    kind: kind.into(),
                    matrix,
                });
            }
        } else {
            let kind = if has_beta { "beta" } else { "total" };
            document.densities.push(Density {
                id: format!("molden-density-{kind}"),
                label: "Occupation-derived density (listed orbitals)".into(),
                kind: kind.into(),
                matrix: if has_beta { beta } else { alpha },
            });
        }
        warnings.push("Density matrices are occupation-derived sums over the listed orbitals, not independently supplied correlated/transition densities; omitted orbitals or core electrons cannot be recovered.".into());
    }
    document.view.field = document
        .orbitals
        .iter()
        .rev()
        .find(|orbital| orbital.occupation.is_some_and(|occupation| occupation > 0.))
        .or_else(|| document.orbitals.first())
        .map(|orbital| orbital.id.clone());
    Ok(())
}
