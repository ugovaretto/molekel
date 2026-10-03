use crate::*;
mod pdb;
pub use pdb::pdb;

const ELEMENTS: &str = "H He Li Be B C N O F Ne Na Mg Al Si P S Cl Ar K Ca Sc Ti V Cr Mn Fe Co Ni Cu Zn Ga Ge As Se Br Kr Rb Sr Y Zr Nb Mo Tc Ru Rh Pd Ag Cd In Sn Sb Te I Xe Cs Ba La Ce Pr Nd Pm Sm Eu Gd Tb Dy Ho Er Tm Yb Lu Hf Ta W Re Os Ir Pt Au Hg Tl Pb Bi Po At Rn Fr Ra Ac Th Pa U Np Pu Am Cm Bk Cf Es Fm Md No Lr Rf Db Sg Bh Hs Mt Ds Rg Cn Nh Fl Mc Lv Ts Og";
fn num(s: &str) -> Result<f64> {
    let n: f64 = s
        .replace(['D', 'd'], "E")
        .parse()
        .map_err(|_| format!("Invalid number: {s}"))?;
    if n.is_finite() {
        Ok(n)
    } else {
        Err("Non-finite input".into())
    }
}
fn count(s: &str) -> Result<usize> {
    s.parse().map_err(|_| format!("Invalid count: {s}"))
}

pub fn xyz(text: &str, name: &str) -> Result<Document> {
    if text.len() > 16 * 1024 * 1024 {
        return Err("XYZ exceeds the preview file budget".into());
    }
    let mut lines = text.lines();
    let n = count(lines.next().ok_or("Missing atom count")?.trim())?;
    if n == 0 || n > 100_000 {
        return Err("XYZ atom count must be 1..100000".into());
    }
    let title = lines.next().ok_or("Missing XYZ comment line")?;
    let mut d = Document::empty(if title.trim().is_empty() { name } else { title });
    let elements: Vec<_> = ELEMENTS.split_whitespace().collect();
    for index in 0..n {
        let row: Vec<_> = lines
            .next()
            .ok_or("Truncated XYZ")?
            .split_whitespace()
            .collect();
        if row.len() != 4 {
            return Err(format!(
                "XYZ atom {} needs symbol and three coordinates",
                index + 1
            ));
        }
        let element = elements
            .iter()
            .position(|e| *e == row[0])
            .ok_or_else(|| format!("Unknown element: {}", row[0]))?
            + 1;
        d.atoms.push(Atom {
            element: element as u16,
            position: [
                num(row[1])? / BOHR_TO_ANGSTROM,
                num(row[2])? / BOHR_TO_ANGSTROM,
                num(row[3])? / BOHR_TO_ANGSTROM,
            ],
        });
    }
    if lines.any(|l| !l.trim().is_empty()) {
        return Err(
            "Multiple XYZ frames and extended XYZ are not supported in this preview".into(),
        );
    }
    let connectivity = bonds::perceive(&d.atoms, &[])?;
    d.provenance = vec![
        format!("Imported {name}; conventional XYZ coordinates assumed angstrom."),
        connectivity.description(),
    ];
    d.bonds = connectivity.bonds;
    d.validate()?;
    Ok(d)
}

pub fn cube(text: &str, name: &str) -> Result<Document> {
    if text.len() > 64 * 1024 * 1024 {
        return Err("Cube exceeds the preview file budget".into());
    }
    let mut lines = text.lines();
    let title = lines.next().ok_or("Missing cube title")?;
    let comment = lines.next().ok_or("Missing cube comment")?;
    let header: Vec<_> = lines
        .next()
        .ok_or("Missing cube origin")?
        .split_whitespace()
        .collect();
    if header.len() < 4 || header.len() > 5 {
        return Err("Invalid cube origin line".into());
    }
    let n: i32 = header[0].parse().map_err(|_| "Invalid cube atom count")?;
    if !(-100_000..=100_000).contains(&n) {
        return Err("Cube atom count must have an absolute value no larger than 100000".into());
    }
    if header.len() == 5 && count(header[4])? != 1 {
        return Err("Only single-channel cubes are supported; NVAL must be one".into());
    }
    let origin = [num(header[1])?, num(header[2])?, num(header[3])?];
    let mut dims = [0; 3];
    let mut axes = [[0.; 3]; 3];
    for k in 0..3 {
        let row: Vec<_> = lines
            .next()
            .ok_or("Missing cube axis")?
            .split_whitespace()
            .collect();
        if row.len() != 4 {
            return Err("Invalid cube axis".into());
        }
        dims[k] = count(row[0]).map_err(|_| {
            "Cube axis counts must be positive; only the standard bohr coordinate profile is supported"
        })?;
        axes[k] = [num(row[1])?, num(row[2])?, num(row[3])?];
    }
    let size = dims
        .iter()
        .try_fold(1usize, |n, d| n.checked_mul(*d))
        .ok_or("Cube dimensions overflow")?;
    if dims.iter().any(|d| *d < 2) || size > MAX_SAMPLES {
        return Err("Cube exceeds the 128^3 sample budget".into());
    }
    let mut d = Document::empty(if title.trim().is_empty() { name } else { title });
    for _ in 0..n.unsigned_abs() {
        let row: Vec<_> = lines
            .next()
            .ok_or("Truncated cube atoms")?
            .split_whitespace()
            .collect();
        if row.len() != 5 {
            return Err("Invalid cube atom".into());
        }
        let element = count(row[0])?;
        if !(1..=118).contains(&element) {
            return Err("Unsupported cube atomic number".into());
        }
        num(row[1])?;
        d.atoms.push(Atom {
            element: element as u16,
            position: [num(row[2])?, num(row[3])?, num(row[4])?],
        });
    }
    let mut tokens = lines.flat_map(str::split_whitespace);
    // Negative NATOMS introduces dataset IDs, not a change in coordinate units.
    let dataset = if n < 0 {
        if count(tokens.next().ok_or("Missing cube dataset count")?)? != 1 {
            return Err(
                "Only single-channel cubes are supported; export one orbital/dataset per cube"
                    .into(),
            );
        }
        Some(
            tokens
                .next()
                .ok_or("Missing cube dataset identifier")?
                .parse::<i64>()
                .map_err(|_| "Invalid cube dataset identifier")?,
        )
    } else {
        None
    };
    let mut values = vec![0.; size];
    for x in 0..dims[0] {
        for y in 0..dims[1] {
            for z in 0..dims[2] {
                values[(z * dims[1] + y) * dims[0] + x] =
                    num(tokens.next().ok_or("Truncated cube values")?)?;
            }
        }
    }
    if tokens.next().is_some() {
        return Err("Unexpected extra cube values".into());
    }
    d.grids.push(Grid {
        id: "cube".into(),
        label: dataset.map_or_else(|| name.into(), |id| format!("{name} (dataset {id})")),
        quantity: "unknown scalar".into(),
        origin,
        axes,
        dims,
        values,
    });
    d.view.field = Some("cube".into());
    let connectivity = bonds::perceive(&d.atoms, &[])?;
    d.provenance = vec![
        format!(
            "Imported {name}; standard cube bohr coordinate profile. Scalar meaning and units unspecified. {comment}"
        ),
        connectivity.description(),
    ];
    if let Some(id) = dataset {
        d.provenance.push(format!(
            "Cube dataset identifier {id}; one sampled scalar field, no basis or orbital coefficients reconstructed."
        ));
    }
    let peak = d.grids[0].values.iter().map(|v| v.abs()).fold(0., f64::max);
    let initial_iso = 0.1 * peak;
    if d.view.isovalue >= peak && initial_iso > 0. && initial_iso < peak {
        d.view.isovalue = initial_iso;
        d.provenance.push(format!(
            "Initial display isovalue {initial_iso} chosen as one tenth of the field peak magnitude; scalar samples unchanged."
        ));
    }
    d.bonds = connectivity.bonds;
    d.validate()?;
    Ok(d)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn xyz_units_and_errors() {
        let d = xyz("1\nHydrogen\nH 0.529177210903 0 0\n", "a.xyz").unwrap();
        assert_eq!(d.atoms[0].position[0], 1.);
        assert!(xyz("2\nx\nH 0 0 0", "x").is_err());
        assert!(xyz("1\nx\nH NaN 0 0", "x").is_err());
    }
    #[test]
    fn cube_axis_order() {
        let text = "test\nunknown\n0 0 0 0\n2 1 0 0\n2 0.2 1 0\n2 0 0 1\n0 1 2 3 4 5 6 7\n";
        let d = cube(text, "x.cube").unwrap();
        assert_eq!(d.grids[0].values, vec![0., 4., 2., 6., 1., 5., 3., 7.]);
        assert_eq!(d.grids[0].axes[1], [0.2, 1., 0.]);
        assert!(cube(&text.replace("6 7", "6"), "x").is_err());
    }
}
