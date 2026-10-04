use eigenvista_core::{Atom, BOHR_TO_ANGSTROM, bonds, import};

fn atom(element: u16, position: [f64; 3]) -> Atom {
    Atom {
        element,
        position: position.map(|p| p / BOHR_TO_ANGSTROM),
    }
}

#[test]
fn xyz_automatically_connects_water_methane_and_disconnected_fragments() {
    let water = import::xyz(
        "3\nwater\nO 0 0 0\nH 0.9572 0 0\nH -0.239 0.927 0\n",
        "water.xyz",
    )
    .unwrap();
    assert_eq!(water.bonds, [[0, 1], [0, 2]]);
    let methane = import::xyz("6\nmethane\nC 0 0 0\nH 0.629 0.629 0.629\nH -0.629 -0.629 0.629\nH 0.629 -0.629 -0.629\nH -0.629 0.629 -0.629\nO 20 0 0\n", "methane.xyz").unwrap();
    assert_eq!(methane.bonds, [[0, 1], [0, 2], [0, 3], [0, 4]]);
}

#[test]
fn exact_neighbor_search_matches_brute_force_for_unrestricted_atoms() {
    let atoms: Vec<_> = (0..250)
        .map(|i| {
            atom(
                14,
                [
                    ((i * 137) % 997) as f64 / 29. - 17.,
                    ((i * 337) % 991) as f64 / 31. - 16.,
                    ((i * 577) % 983) as f64 / 37. - 13.,
                ],
            )
        })
        .collect();
    let mut expected = Vec::new();
    let cutoff = 2. * bonds::covalent_radius(14).unwrap() + bonds::TOLERANCE_ANGSTROM;
    for i in 0..atoms.len() {
        for j in i + 1..atoms.len() {
            let squared = (0..3)
                .map(|k| ((atoms[i].position[k] - atoms[j].position[k]) * BOHR_TO_ANGSTROM).powi(2))
                .sum::<f64>();
            if squared >= bonds::MIN_DISTANCE_ANGSTROM.powi(2) && squared <= cutoff.powi(2) {
                expected.push([i, j]);
            }
        }
    }
    assert_eq!(bonds::perceive(&atoms, &[]).unwrap().bonds, expected);
}

#[test]
fn hydrogen_coordination_explicit_precedence_and_no_duplicate_edges() {
    let atoms = [
        atom(8, [0., 0., 0.]),
        atom(1, [1., 0., 0.]),
        atom(8, [2.2, 0., 0.]),
    ];
    assert_eq!(bonds::perceive(&atoms, &[]).unwrap().bonds, [[0, 1]]);
    assert_eq!(
        bonds::perceive(&atoms, &[[1, 2], [2, 1]]).unwrap().bonds,
        [[1, 2]]
    );
    assert!(bonds::perceive(&atoms, &[[0, 0]]).is_err());
    assert!(bonds::perceive(&atoms, &[[0, 5]]).is_err());
}

#[test]
fn coincident_planar_and_collinear_coordinates_are_safe() {
    let coincident = vec![atom(6, [0.; 3]); 100];
    assert!(bonds::perceive(&coincident, &[]).unwrap().bonds.is_empty());
    let chain: Vec<_> = (0..1000)
        .map(|i| atom(6, [i as f64 * 1.5 - 600., 0., 0.]))
        .collect();
    let perceived = bonds::perceive(&chain, &[]).unwrap();
    assert_eq!(perceived.bonds.len(), 999);
    assert!(perceived.neighbor_visits < 5000);
    let plane: Vec<_> = (0..100)
        .map(|i| atom(14, [(i % 10) as f64 * 2., (i / 10) as f64 * 2., 0.]))
        .collect();
    assert_eq!(bonds::perceive(&plane, &[]).unwrap().bonds.len(), 180);
}

#[test]
fn bond_detection_rejects_invalid_data_and_covers_all_elements() {
    for z in 1..=118 {
        assert!(bonds::covalent_radius(z).unwrap() > 0.);
    }
    assert!(bonds::perceive(&[atom(0, [0.; 3])], &[]).is_err());
    assert!(bonds::perceive(&[atom(1, [f64::NAN, 0., 0.])], &[]).is_err());
    assert!(bonds::perceive(&[atom(1, [1e20, 0., 0.])], &[]).is_err());
}

#[test]
fn dense_input_hits_a_budget_instead_of_unbounded_pair_work() {
    let atoms = vec![atom(6, [0.; 3]); 3200];
    assert!(
        bonds::perceive(&atoms, &[])
            .err()
            .unwrap()
            .contains("neighbor budget")
    );
}

#[test]
fn numeric_alternate_labels_with_partial_occupancy_are_not_legacy_names() {
    let text = format!(
        "{}{}",
        pdb_atom(1, " H2'", '1', 1, "", [0.; 3], 0.4),
        pdb_atom(2, " H2'", '2', 1, "", [2., 0., 0.], 0.6)
    );
    let doc = import::pdb(&text, "alternates.pdb").unwrap();
    assert_eq!(doc.atoms.len(), 1);
    assert!((doc.atoms[0].position[0] * BOHR_TO_ANGSTROM - 2.).abs() < 1e-12);
}

#[test]
fn repository_pdb_fixtures_load_with_automatically_computed_bonds() {
    for (name, text, expected_atoms) in [
        (
            "guanine.pdb",
            include_str!("../../../../data/guanine.pdb"),
            33,
        ),
        ("3POR.pdb", include_str!("../../../../data/3POR.pdb"), 2325),
        (
            "uridine.pdb",
            include_str!("../../../../data/URIDINE-VANADATE.pdb"),
            31,
        ),
        (
            "alaninemulti.pdb",
            include_str!("../../../../data/alaninemulti.pdb"),
            66,
        ),
    ] {
        let d = import::pdb(text, name).unwrap_or_else(|e| panic!("{name}: {e}"));
        assert_eq!(d.atoms.len(), expected_atoms, "{name}");
        assert!(d.bonds.len() >= d.atoms.len() / 2, "{name}");
        assert!(d.bonds.iter().all(|b| b[0] < b[1]), "{name}");
        println!("{name}: {} atoms, {} bonds", d.atoms.len(), d.bonds.len());
    }
}

fn pdb_atom(
    id: usize,
    name: &str,
    alt: char,
    residue: usize,
    element: &str,
    p: [f64; 3],
    occupancy: f64,
) -> String {
    format!(
        "ATOM  {id:5} {name:4}{alt}LIG A{residue:4}    {:8.3}{:8.3}{:8.3}{occupancy:6.2}{:6.2}          {element:>2}  \n",
        p[0], p[1], p[2], 0.
    )
}

#[test]
fn pdb_name_alignment_is_not_confused_with_calcium_or_mercury() {
    let text = [
        pdb_atom(1, " CA ", ' ', 1, "", [0., 0., 0.], 1.),
        pdb_atom(2, "CA  ", ' ', 2, "", [10., 0., 0.], 1.),
        pdb_atom(3, "1HG ", ' ', 3, "", [20., 0., 0.], 1.),
        pdb_atom(4, "HG  ", ' ', 4, "", [30., 0., 0.], 1.),
        pdb_atom(5, " D1 ", ' ', 5, "D", [40., 0., 0.], 1.),
        pdb_atom(6, " CA ", ' ', 6, "N", [50., 0., 0.], 1.),
    ]
    .join("");
    let d = import::pdb(&text, "elements.pdb").unwrap();
    assert_eq!(
        d.atoms.iter().map(|a| a.element).collect::<Vec<_>>(),
        [6, 20, 1, 80, 1, 7]
    );
}

#[test]
fn pdb_selects_one_model_and_coherent_high_occupancy_alternate() {
    let a = pdb_atom(1, " O  ", ' ', 1, "O", [0.; 3], 1.);
    let b = pdb_atom(2, " H1 ", 'A', 1, "H", [1., 0., 0.], 0.4);
    let c = pdb_atom(3, " H1 ", 'B', 1, "H", [-1., 0., 0.], 0.6);
    let zero = pdb_atom(4, " H2 ", ' ', 1, "H", [0., 1., 0.], 0.);
    let text = format!(
        "MODEL        1\n{a}{b}{c}{zero}ENDMDL\nMODEL        2\n{a}ENDMDL\nCONECT    1    2    3\nEND\n"
    );
    let d = import::pdb(&text, "models.pdb").unwrap();
    assert_eq!(d.atoms.len(), 2);
    assert!(d.atoms[1].position[0] < 0.);
    assert_eq!(d.bonds, [[0, 1]]);
}

#[test]
fn pdb_preserves_explicit_long_bonds_and_also_infers_missing_ones() {
    let text = format!(
        "{}{}{}CONECT    1    2    2\nCONECT    2    1\nEND\n",
        pdb_atom(1, " C1 ", ' ', 1, "C", [0.; 3], 1.),
        pdb_atom(2, " C2 ", ' ', 1, "C", [5., 0., 0.], 1.),
        pdb_atom(3, " H1 ", ' ', 1, "H", [1., 0., 0.], 1.)
    );
    let d = import::pdb(&text, "connected.pdb").unwrap();
    assert_eq!(d.bonds, [[0, 1], [0, 2]]);
    assert!((d.atoms[1].position[0] * BOHR_TO_ANGSTROM - 5.).abs() < 1e-12);
}

#[test]
fn malformed_pdb_returns_errors_without_fabricated_coordinates() {
    let valid = pdb_atom(1, " C1 ", ' ', 1, "C", [0.; 3], 1.);
    for text in [
        "ATOM      1".into(),
        valid.replace("   0.000", "     NaN"),
        valid.replace("  C  ", " XX  "),
        format!("{valid}{valid}"),
        format!("{valid}CONECT    1   20\n"),
        valid.replace("  1.00", " -1.00"),
        "REMARK no atoms\nEND\n".into(),
        "ATOM \u{00e9}".into(),
    ] {
        assert!(import::pdb(&text, "invalid.pdb").is_err(), "{text}");
    }
}
