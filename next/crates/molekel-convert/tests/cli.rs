use std::{
    fs,
    path::{Path, PathBuf},
    process::{Command, Output},
    sync::atomic::{AtomicUsize, Ordering},
    time::{SystemTime, UNIX_EPOCH},
};

struct Scratch(PathBuf);
static SCRATCH_ID: AtomicUsize = AtomicUsize::new(0);
impl Scratch {
    fn new() -> Self {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tmp");
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        fs::create_dir_all(&root).unwrap();
        let id = SCRATCH_ID.fetch_add(1, Ordering::Relaxed);
        let dir = root.join(format!("convert-test-{}-{nonce}-{id}", std::process::id()));
        fs::create_dir(&dir).unwrap();
        Self(dir)
    }
    fn run(&self, args: &[&str]) -> Output {
        Command::new(env!("CARGO_BIN_EXE_molekel-convert"))
            .args(args)
            .current_dir(&self.0)
            .env("TMPDIR", &self.0)
            .env("TMP", &self.0)
            .env("TEMP", &self.0)
            .output()
            .unwrap()
    }
    fn water(&self, name: &str) {
        fs::write(
            self.0.join(name),
            "3\nWater\nO 0 0 0\nH 0.758 0 0.504\nH -0.758 0 0.504\n",
        )
        .unwrap();
    }
}
impl Drop for Scratch {
    fn drop(&mut self) {
        let _ = fs::remove_dir_all(&self.0);
    }
}

#[test]
fn help_version_and_invalid_options_are_explicit() {
    let dir = Scratch::new();
    let help = dir.run(&["--help"]);
    assert!(help.status.success());
    assert!(String::from_utf8_lossy(&help.stdout).contains("--output-dir"));
    assert!(dir.run(&["--version"]).status.success());
    for args in [
        vec![],
        vec!["--unknown"],
        vec!["--output"],
        vec!["--check", "--force", "in.xyz"],
    ] {
        assert_eq!(dir.run(&args).status.code(), Some(2));
    }
    assert_eq!(fs::read_dir(&dir.0).unwrap().count(), 0);
}

#[test]
fn conversion_preserves_structure_and_refuses_accidental_overwrite() {
    let dir = Scratch::new();
    dir.water("water.xyz");
    let original = fs::read(dir.0.join("water.xyz")).unwrap();
    let result = dir.run(&["water.xyz", "--json"]);
    assert!(
        result.status.success(),
        "{}",
        String::from_utf8_lossy(&result.stderr)
    );
    let report: serde_json::Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(report[0]["status"], "converted");
    assert_eq!(report[0]["report"]["format"], "xyz");
    let saved = fs::read(dir.0.join("water.molekel")).unwrap();
    let doc = molekel_format::decode(&saved).unwrap();
    assert_eq!(doc.atoms.len(), 3);
    assert_eq!(doc.bonds.len(), 2);
    assert_eq!(dir.run(&["water.xyz"]).status.code(), Some(2));
    assert_eq!(fs::read(dir.0.join("water.molekel")).unwrap(), saved);
    assert!(dir.run(&["water.xyz", "--force"]).status.success());
    assert_eq!(fs::read(dir.0.join("water.xyz")).unwrap(), original);
}

#[test]
fn molden_cli_and_library_produce_the_same_scientific_document() {
    let dir = Scratch::new();
    let source =
        Path::new(env!("CARGO_MANIFEST_DIR")).join("../../fixtures/molden/water-rhf-ccpvdz.molden");
    fs::copy(source, dir.0.join("water.molden.input")).unwrap();
    let input = fs::read(dir.0.join("water.molden.input")).unwrap();
    let expected = molekel_import::import_bytes(&input, "water.molden.input").unwrap();
    let result = dir.run(&["water.molden.input", "--json"]);
    assert!(
        result.status.success(),
        "{}",
        String::from_utf8_lossy(&result.stderr)
    );
    let doc = molekel_format::decode(&fs::read(dir.0.join("water.molekel")).unwrap()).unwrap();
    assert_eq!(doc, expected.document);
    assert_eq!(doc.basis.len(), 24);
    assert!(!doc.orbitals.is_empty());
    assert!(!doc.densities.is_empty());
    assert!(!doc.bonds.is_empty());
}

#[test]
fn check_does_not_write_and_batch_reports_individual_failures() {
    let dir = Scratch::new();
    dir.water("water.xyz");
    fs::write(
        dir.0.join("broken.molden"),
        "[Molden Format]\n[Atoms] AU\ninvalid\n",
    )
    .unwrap();
    let checked = dir.run(&["--check", "--json", "water.xyz"]);
    assert!(checked.status.success());
    assert!(!dir.0.join("water.molekel").exists());
    fs::create_dir(dir.0.join("out")).unwrap();
    let result = dir.run(&[
        "--json",
        "--output-dir",
        "out",
        "water.xyz",
        "broken.molden",
    ]);
    assert_eq!(result.status.code(), Some(1));
    let report: serde_json::Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(report[0]["status"], "converted");
    assert_eq!(report[1]["status"], "failed");
    assert!(report[1]["error"].as_str().unwrap().contains("Molden"));
    assert!(dir.0.join("out/water.molekel").exists());
    assert!(!dir.0.join("out/broken.molekel").exists());
}

#[test]
fn cube_conversion_preserves_scalar_data_and_repairs_missing_native_bonds() {
    let dir = Scratch::new();
    let input = include_bytes!("../../../fixtures/cube/signed-affine.cube");
    fs::write(dir.0.join("field.cube"), input).unwrap();
    let expected = molekel_import::import_bytes(input, "field.cube")
        .unwrap()
        .document;
    let result = dir.run(&["field.cube", "--json"]);
    assert!(
        result.status.success(),
        "{}",
        String::from_utf8_lossy(&result.stderr)
    );
    let converted =
        molekel_format::decode(&fs::read(dir.0.join("field.molekel")).unwrap()).unwrap();
    assert_eq!(converted, expected);
    assert_eq!(converted.bonds, vec![[0, 1]]);
    assert!(converted.basis.is_empty());
    assert_eq!(converted.grids[0].values.len(), 343);
    let mut missing = converted.clone();
    missing.bonds.clear();
    fs::write(
        dir.0.join("old.molekel"),
        molekel_format::encode(&missing).unwrap(),
    )
    .unwrap();
    let result = dir.run(&["old.molekel", "-o", "repaired.molekel", "--json"]);
    assert!(result.status.success());
    let report: serde_json::Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(report[0]["report"]["requires_save"], true);
    let repaired =
        molekel_format::decode(&fs::read(dir.0.join("repaired.molekel")).unwrap()).unwrap();
    assert_eq!(repaired.bonds, converted.bonds);
    assert_eq!(repaired.grids, converted.grids);
    assert_eq!(
        molekel_format::decode(&fs::read(dir.0.join("old.molekel")).unwrap()).unwrap(),
        missing
    );
    assert_eq!(fs::read(dir.0.join("field.cube")).unwrap(), input);
}

#[test]
fn batch_output_collisions_and_input_replacement_are_rejected_before_writing() {
    let dir = Scratch::new();
    fs::create_dir(dir.0.join("a")).unwrap();
    fs::create_dir(dir.0.join("b")).unwrap();
    fs::create_dir(dir.0.join("out")).unwrap();
    dir.water("a/water.xyz");
    dir.water("b/water.xyz");
    let result = dir.run(&[
        "--force",
        "--output-dir",
        "out",
        "a/water.xyz",
        "b/water.xyz",
    ]);
    assert_eq!(result.status.code(), Some(2));
    assert_eq!(fs::read_dir(dir.0.join("out")).unwrap().count(), 0);
    let native = molekel_format::encode(&molekel_core::fixtures::hydrogen_pair()).unwrap();
    fs::write(dir.0.join("source.molekel"), &native).unwrap();
    assert_eq!(
        dir.run(&["source.molekel", "--force"]).status.code(),
        Some(2)
    );
    assert_eq!(fs::read(dir.0.join("source.molekel")).unwrap(), native);
}

#[test]
fn oversized_input_and_invalid_conversion_leave_no_partial_document() {
    let dir = Scratch::new();
    let file = fs::File::create(dir.0.join("large.xyz")).unwrap();
    file.set_len(molekel_import::MAX_IMPORT_BYTES as u64 + 1)
        .unwrap();
    assert_eq!(dir.run(&["large.xyz"]).status.code(), Some(1));
    assert!(!dir.0.join("large.molekel").exists());
    dir.water("source.xyz");
    assert!(dir.run(&["source.xyz"]).status.success());
    let existing = fs::read(dir.0.join("source.molekel")).unwrap();
    fs::write(dir.0.join("source.xyz"), "invalid").unwrap();
    assert_eq!(dir.run(&["source.xyz", "--force"]).status.code(), Some(1));
    assert_eq!(fs::read(dir.0.join("source.molekel")).unwrap(), existing);
    assert_eq!(fs::read_dir(&dir.0).unwrap().count(), 3);
}

#[test]
#[cfg(unix)]
fn output_symlinks_are_not_followed_even_with_force() {
    let dir = Scratch::new();
    dir.water("water.xyz");
    let original = fs::read(dir.0.join("water.xyz")).unwrap();
    std::os::unix::fs::symlink(dir.0.join("water.xyz"), dir.0.join("water.molekel")).unwrap();
    assert_eq!(dir.run(&["water.xyz", "--force"]).status.code(), Some(2));
    assert_eq!(fs::read(dir.0.join("water.xyz")).unwrap(), original);
}

#[test]
#[cfg(target_os = "macos")]
fn force_batch_cannot_overwrite_a_new_unicode_alias_output() {
    let dir = Scratch::new();
    for name in ["a", "b", "out"] {
        fs::create_dir(dir.0.join(name)).unwrap();
    }
    dir.water("a/caf\u{e9}.xyz");
    fs::write(dir.0.join("b/cafe\u{301}.xyz"), "1\nHelium\nHe 0 0 0\n").unwrap();
    let result = dir.run(&[
        "--force",
        "--json",
        "--output-dir",
        "out",
        "a/caf\u{e9}.xyz",
        "b/cafe\u{301}.xyz",
    ]);
    assert_eq!(result.status.code(), Some(1));
    let report: serde_json::Value = serde_json::from_slice(&result.stdout).unwrap();
    assert_eq!(report[0]["status"], "converted");
    assert_eq!(report[1]["status"], "failed");
    let first = fs::read(dir.0.join("out/caf\u{e9}.molekel")).unwrap();
    assert_eq!(molekel_format::decode(&first).unwrap().atoms.len(), 3);
    assert_eq!(fs::read_dir(dir.0.join("out")).unwrap().count(), 1);
}

#[test]
#[cfg(target_os = "macos")]
fn force_batch_rejects_preexisting_unicode_aliases_before_writing() {
    let dir = Scratch::new();
    for name in ["a", "b", "out"] {
        fs::create_dir(dir.0.join(name)).unwrap();
    }
    dir.water("a/caf\u{e9}.xyz");
    dir.water("b/cafe\u{301}.xyz");
    let original = molekel_format::encode(&molekel_core::fixtures::hydrogen_pair()).unwrap();
    fs::write(dir.0.join("out/caf\u{e9}.molekel"), &original).unwrap();
    let result = dir.run(&[
        "--force",
        "--output-dir",
        "out",
        "a/caf\u{e9}.xyz",
        "b/cafe\u{301}.xyz",
    ]);
    assert_eq!(result.status.code(), Some(2));
    assert_eq!(
        fs::read(dir.0.join("out/caf\u{e9}.molekel")).unwrap(),
        original
    );
    assert_eq!(fs::read_dir(dir.0.join("out")).unwrap().count(), 1);
}
