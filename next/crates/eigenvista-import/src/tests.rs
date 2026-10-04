use super::*;

const HYDROGEN: &str = "[Molden Format]\n[Atoms] AU\nH 1 1 0 0 0\n[GTO]\n1 0\ns 1 1.00\n1.0 1.0\n[MO]\nSym=A\nEne=-0.5\nSpin=Alpha\nOccup=1\n1 1.0\n";

fn load(text: &str) -> ImportResult {
    import_bytes(text.as_bytes(), "test.molden").unwrap()
}

fn fails(text: &str, expected: &str) {
    let error = import_bytes(text.as_bytes(), "test.molden").unwrap_err();
    assert!(
        error
            .to_ascii_lowercase()
            .contains(&expected.to_ascii_lowercase()),
        "{error:?} does not contain {expected:?}"
    );
}

#[test]
fn detects_molden_content_before_filename_and_retains_source_identity() {
    let result = import_bytes(HYDROGEN.as_bytes(), "molden.input").unwrap();
    assert_eq!(result.report.format, "molden");
    assert!(result.report.requires_save);
    assert_eq!(result.document.atoms.len(), 1);
    assert_eq!(result.document.basis.len(), 1);
    assert!(
        result
            .document
            .provenance
            .iter()
            .any(|entry| entry.contains("SHA-256"))
    );
    assert!(
        result
            .document
            .provenance
            .iter()
            .any(|entry| entry.contains("canonical Gaussian profile"))
    );
    assert_eq!(result.document.orbitals[0].spin, "spatial");
    assert_eq!(result.document.densities[0].matrix, vec![1.]);
    assert_eq!(
        import_bytes(HYDROGEN.as_bytes(), "misnamed.eigenvista")
            .unwrap()
            .report
            .format,
        "molden"
    );
}

#[test]
fn native_dispatch_preserves_document_without_adding_import_provenance() {
    let document = load(HYDROGEN).document;
    let bytes = eigenvista_format::encode(&document).unwrap();
    let result = import_bytes(&bytes, "unknown").unwrap();
    assert_eq!(result.document, document);
    assert_eq!(result.report.format, "eigenvista");
    assert!(result.report.warnings.is_empty());
    assert!(!result.report.requires_save);
}

#[test]
fn native_import_adds_only_missing_bonds_and_preserves_scientific_caches() {
    use eigenvista_core::{Atom, compute, fixtures};
    let mut document = fixtures::hydrogen_pair();
    document.atoms.extend([
        Atom {
            element: 1,
            position: [30., 0., 0.],
        },
        Atom {
            element: 1,
            position: [31.4, 0., 0.],
        },
    ]);
    document.bonds = vec![[1, 0]];
    let hash = compute::source_hash(&document, "antibonding").unwrap();
    let grid = compute::sample(&document, "antibonding", 12).unwrap();
    let surface = compute::mesh(&grid, 0.02, "antibonding", &hash, "#00ff00", 0.5).unwrap();
    assert!(!surface.indices.is_empty());
    document.grids.push(grid);
    document.surfaces.push(surface);
    let encoded = eigenvista_format::encode(&document).unwrap();
    assert_eq!(eigenvista_format::decode(&encoded).unwrap(), document);
    let imported = import_bytes(&encoded, "existing.eigenvista").unwrap();
    assert!(imported.report.requires_save);
    assert_eq!(imported.document.bonds, [[1, 0], [2, 3]]);
    assert!(imported.report.warnings[0].contains("Added 1 missing display bonds"));
    assert_eq!(
        compute::source_hash(&imported.document, "antibonding").unwrap(),
        hash
    );
    let mut unchanged = imported.document.clone();
    unchanged.bonds = document.bonds.clone();
    unchanged.provenance = document.provenance.clone();
    assert_eq!(unchanged, document);
    let reopened = import_bytes(
        &eigenvista_format::encode(&imported.document).unwrap(),
        "saved.eigenvista",
    )
    .unwrap();
    assert_eq!(reopened.document, imported.document);
    assert!(!reopened.report.requires_save);
    assert!(reopened.report.warnings.is_empty());
}

#[test]
fn native_import_keeps_explicit_long_bonds_and_does_not_claim_false_changes() {
    let mut document = eigenvista_core::fixtures::hydrogen_pair();
    document.atoms[1].position = [100., 0., 0.];
    document.bonds = vec![[1, 0]];
    let result = import_bytes(
        &eigenvista_format::encode(&document).unwrap(),
        "explicit.eigenvista",
    )
    .unwrap();
    assert_eq!(result.document, document);
    assert!(!result.report.requires_save);
    assert!(result.report.warnings.is_empty());
    let report: ImportReport =
        serde_json::from_str(r#"{"format":"eigenvista","warnings":[]}"#).unwrap();
    assert!(!report.requires_save);
}

#[test]
fn shared_cube_dispatch_creates_bonds_and_a_persistable_scalar_field() {
    let bytes = b"Water\nScalar field\n3 0 0 0\n2 1 0 0\n2 0 1 0\n2 0 0 1\n8 8 0 0 0\n1 1 1.8 0 0\n1 1 -0.45 1.75 0\n0 1 2 3 4 5 6 7\n";
    for name in ["field.cube", "field.CUB"] {
        let result = import_bytes(bytes, name).unwrap();
        assert_eq!(result.report.format, "cube");
        assert!(result.report.requires_save);
        assert_eq!(result.document.bonds, [[0, 1], [0, 2]]);
        assert_eq!(
            result.document.grids[0].values,
            [0., 4., 2., 6., 1., 5., 3., 7.]
        );
        let reopened = import_bytes(
            &eigenvista_format::encode(&result.document).unwrap(),
            "field.eigenvista",
        )
        .unwrap();
        assert_eq!(reopened.document, result.document);
        assert!(!reopened.report.requires_save);
    }
}

#[test]
fn structure_dispatch_and_wrong_extension_errors_are_actionable() {
    let result = import_bytes(b"2\nHydrogen\nH 0 0 0\nH 0 0 0.74\n", "molecule.XYZ").unwrap();
    assert_eq!(result.report.format, "xyz");
    assert_eq!(result.document.bonds, vec![[0, 1]]);
    assert!(
        import_bytes(b"unsupported", "sample.gbw")
            .unwrap_err()
            .contains("Unsupported file")
    );
    assert!(import_bytes(&[], "empty.molden").is_err());
    assert!(
        import_bytes(&[255, 254], "sample.molden")
            .unwrap_err()
            .contains("UTF-8")
    );
}

#[test]
fn atom_units_indexes_bonds_and_fortran_exponents() {
    let text = HYDROGEN
        .replace(
            "[Atoms] AU\nH 1 1 0 0 0",
            "[Atoms] (Angs)\nH 7 1 0 0 0\nH 3 1 0 0 0.74",
        )
        .replace("[GTO]\n1 0", "[GTO]\n3")
        .replace("1.0 1.0", "1.0D+00 1.0d+00");
    let result = load(&text);
    assert!(
        (result.document.atoms[1].position[2] - 0.74 / eigenvista_core::BOHR_TO_ANGSTROM).abs()
            < 1e-12
    );
    assert_eq!(
        result.document.basis[0].center,
        result.document.atoms[1].position
    );
    assert_eq!(result.document.bonds, vec![[0, 1]]);
}

#[test]
fn sparse_orbitals_keep_omitted_components_zero_and_do_not_invent_metadata() {
    let text = HYDROGEN
        .replace("s 1 1.00", "p 1 1.00")
        .replace("Ene=-0.5\n", "")
        .replace("Occup=1\n", "")
        .replace("1 1.0\n", "2 1.0\n");
    let result = load(&text);
    assert_eq!(result.document.orbitals[0].coefficients, vec![0., 1., 0.]);
    assert_eq!(result.document.orbitals[0].energy, None);
    assert_eq!(result.document.orbitals[0].occupation, None);
    assert!(result.document.densities.is_empty());
    assert!(
        result
            .report
            .warnings
            .iter()
            .any(|warning| warning.contains("occupations are missing"))
    );
}

#[test]
fn sp_shells_expand_in_s_xyz_order() {
    let result = load(&HYDROGEN.replace("s 1 1.00\n1.0 1.0", "sp 1\n1.0 0.3 0.7"));
    assert_eq!(result.document.basis.len(), 4);
    for (basis, powers) in
        result
            .document
            .basis
            .iter()
            .zip([[0, 0, 0], [1, 0, 0], [0, 1, 0], [0, 0, 1]])
    {
        assert_eq!(basis.terms[0].powers, powers);
    }
}

#[test]
fn default_and_mixed_spherical_component_flags() {
    let input = HYDROGEN.replace("s 1 1.00\n1.0 1.0", "d 1\n1 1\nf 1\n1 1\ng 1\n1 1");
    for (flags, size) in [
        ("", 31),
        ("[5D]\n", 27),
        ("[5D10F]\n", 30),
        ("[7F]\n", 28),
        ("[5D7F]\n[9G]\n", 21),
    ] {
        assert_eq!(
            load(&input.replace("[MO]", &format!("{flags}[MO]")))
                .document
                .basis
                .len(),
            size
        );
    }
    fails(&input.replace("[MO]", "[5D]\n[6D]\n[MO]"), "Conflicting");
    fails(&input.replace("[MO]", "[11H]\n[MO]"), "Unsupported");
}

#[test]
fn spin_resolved_and_fractional_occupation_densities() {
    let text = format!(
        "{}Sym=B\nSpin=Beta\nOccup=0.25\n1 0.5\n",
        HYDROGEN.replace("Occup=1", "Occup=0.75")
    );
    let result = load(&text);
    assert_eq!(result.document.orbitals[0].spin, "alpha");
    assert_eq!(result.document.orbitals[1].spin, "beta");
    assert_eq!(
        result
            .document
            .densities
            .iter()
            .find(|d| d.kind == "total")
            .unwrap()
            .matrix,
        vec![0.8125]
    );
    assert_eq!(
        result
            .document
            .densities
            .iter()
            .find(|d| d.kind == "spin")
            .unwrap()
            .matrix,
        vec![0.6875]
    );
    fails(&text.replace("Occup=0.75", "Occup=2"), "cannot exceed one");
}

#[test]
fn separate_mo_sections_and_beta_only_exports_remain_explicit() {
    let text = format!("{}[MO]\nSym=B\nSpin=Beta\nOccup=1\n1 1\n", HYDROGEN);
    assert_eq!(load(&text).document.orbitals.len(), 2);
    let beta = load(&HYDROGEN.replace("Spin=Alpha", "Spin=Beta"));
    assert_eq!(beta.document.orbitals[0].spin, "beta");
    assert_eq!(beta.document.densities[0].kind, "beta");
}

#[test]
fn ignored_sections_are_reported_and_persisted() {
    let result = load(&format!("{HYDROGEN}[FREQ]\n123\n[FR-COORD]\nH 0 0 0\n"));
    assert!(
        result
            .report
            .warnings
            .iter()
            .any(|warning| warning.contains("[FREQ] data are not retained"))
    );
    assert!(
        result
            .document
            .provenance
            .iter()
            .any(|warning| warning.contains("[FR-COORD] data are not retained"))
    );
}

#[test]
fn malformed_or_unsupported_scientific_inputs_fail_at_the_boundary() {
    for (text, reason) in [
        (
            HYDROGEN.replace("[Atoms] AU", "[Atoms]"),
            "explicitly specify",
        ),
        (HYDROGEN.replace("H 1 1 0 0 0", "He 1 1 0 0 0"), "disagree"),
        (HYDROGEN.replace("H 1 1 0 0 0", "H 1 0 0 0 0"), "real atoms"),
        (
            HYDROGEN.replace("H 1 1 0 0 0", "H 1 1 0 0 0\nH 1 1 1 0 0"),
            "Duplicate atom",
        ),
        (HYDROGEN.replace("[GTO]\n1 0", "[GTO]\n2 0"), "unknown atom"),
        (HYDROGEN.replace("[GTO]", "[STO]"), "Slater"),
        (HYDROGEN.replace("s 1 1.00", "h 1 1.00"), "Only Gaussian"),
        (
            HYDROGEN.replace("s 1 1.00", "s 1 2.00"),
            "Non-unit shell scale",
        ),
        (HYDROGEN.replace("s 1 1.00", "s 65"), "Primitive count"),
        (HYDROGEN.replace("1.0 1.0", "0 1"), "positive"),
        (HYDROGEN.replace("1.0 1.0", "1 NaN"), "Non-finite"),
        (HYDROGEN.replace("1.0 1.0", "1 0"), "normalization"),
        (HYDROGEN.replace("1.0 1.0", "1e-300 1"), "normalization"),
        (HYDROGEN.replace("1.0 1.0", "1e300 1"), "normalization"),
        (HYDROGEN.replace("Spin=Alpha", "Spin=Spinor"), "Spin must"),
        (HYDROGEN.replace("Spin=Alpha\n", ""), "Explicit Spin"),
        (HYDROGEN.replace("Occup=1", "Occup=-1"), "Occupation"),
        (HYDROGEN.replace("1 1.0\n", "2 1.0\n"), "index exceeds"),
        (format!("{HYDROGEN}1 0.5\n"), "Duplicate AO"),
        (
            format!("{HYDROGEN}Sym=truncated\nSpin=Alpha\n"),
            "Missing coefficients",
        ),
        (HYDROGEN.replace("1 1.0\n", "1 0\n"), "All-zero"),
        (
            HYDROGEN.replace("[GTO]", "[Atoms] AU\nH 2 1 0 0 1\n[GTO]"),
            "Repeated [ATOMS]",
        ),
    ] {
        fails(&text, reason);
    }
}

#[test]
fn excessive_counts_are_rejected_before_unbounded_expansion() {
    let large_basis = HYDROGEN.replace("s 1 1.00\n1.0 1.0", &"g 1\n1 1\n".repeat(18));
    fails(&large_basis, "256 atomic orbitals");
    let orbitals = format!(
        "{}{}",
        HYDROGEN,
        "Sym=A\nSpin=Alpha\nOccup=0\n1 1\n".repeat(512)
    );
    fails(&orbitals, "512 molecular orbitals");
    let long_line = format!("{HYDROGEN}{}", "X".repeat(16385));
    fails(&long_line, "line/count budget");
    fails(&format!("{HYDROGEN}{}", "[FREQ]\n".repeat(128)), "Too many");
}

#[test]
fn known_orca_profiles_never_silently_use_canonical_normalization() {
    let orca = HYDROGEN.replace(
        "[Atoms]",
        "[Title]\nMolden file created by orca_2mkl\n[Atoms]",
    );
    assert!(
        load(&orca)
            .report
            .warnings
            .iter()
            .any(|warning| warning.contains("orca_2mkl"))
    );
    fails(&orca.replace("s 1 1.00", "f 1"), "qualified only");
    fails(&orca.replace("s 1 1.00", "d 1"), "qualified only");
    fails(
        &orca.replace("orca_2mkl", "Unknown ORCA export"),
        "Unrecognized ORCA",
    );
}
