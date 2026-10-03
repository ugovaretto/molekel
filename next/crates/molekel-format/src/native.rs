use crate::decode;
use molekel_core::Result;
use std::{
    fs::OpenOptions,
    io::Write,
    path::Path,
    time::{SystemTime, UNIX_EPOCH},
};

/// Validate before writing; replacement never exposes a partially written file.
pub fn save_atomic(path: &Path, bytes: &[u8]) -> Result<()> {
    decode(bytes)?;
    let parent = path.parent().ok_or("No save directory")?;
    let nonce = SystemTime::now()
        .duration_since(UNIX_EPOCH)
        .map_err(|e| e.to_string())?
        .as_nanos();
    let temporary = parent.join(format!(".molekel-save-{}-{nonce}", std::process::id()));
    let mut options = OpenOptions::new();
    options.write(true).create_new(true);
    #[cfg(unix)]
    {
        use std::os::unix::fs::OpenOptionsExt;
        options.mode(0o600);
    }
    // A failed create must not remove a pre-existing file with the same name.
    let mut file = options.open(&temporary).map_err(|e| e.to_string())?;
    let result = (|| {
        file.write_all(bytes).map_err(|e| e.to_string())?;
        file.sync_all().map_err(|e| e.to_string())?;
        drop(file);
        std::fs::rename(&temporary, path).map_err(|e| e.to_string())
    })();
    if result.is_err() {
        let _ = std::fs::remove_file(&temporary);
    }
    result
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::encode;
    use molekel_core::fixtures;
    use std::path::PathBuf;

    fn scratch(label: &str) -> PathBuf {
        let nonce = SystemTime::now()
            .duration_since(UNIX_EPOCH)
            .unwrap()
            .as_nanos();
        let dir = Path::new(env!("CARGO_MANIFEST_DIR"))
            .join("../../../tmp")
            .join(format!(
                "native-save-{label}-{}-{nonce}",
                std::process::id()
            ));
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    #[cfg(unix)]
    fn native_replacement_is_complete_and_roundtrips() {
        let dir = scratch("roundtrip");
        let path = dir.join("example.molekel");
        save_atomic(&path, &encode(&fixtures::hydrogen_pair()).unwrap()).unwrap();
        save_atomic(&path, &encode(&fixtures::open_shell()).unwrap()).unwrap();
        assert_eq!(
            decode(&std::fs::read(&path).unwrap()).unwrap(),
            fixtures::open_shell()
        );
        assert_eq!(std::fs::read_dir(&dir).unwrap().count(), 1);
        std::fs::remove_dir_all(dir).unwrap();
    }

    #[test]
    fn invalid_save_preserves_existing_bytes_and_cleans_failed_writes() {
        let dir = scratch("invalid");
        let path = dir.join("example.molekel");
        let bytes = encode(&fixtures::hydrogen_pair()).unwrap();
        save_atomic(&path, &bytes).unwrap();
        assert!(save_atomic(&path, b"not a native document").is_err());
        assert_eq!(std::fs::read(&path).unwrap(), bytes);
        let destination_dir = dir.join("directory.molekel");
        std::fs::create_dir(&destination_dir).unwrap();
        assert!(save_atomic(&destination_dir, &bytes).is_err());
        assert_eq!(std::fs::read_dir(&dir).unwrap().count(), 2);
        std::fs::remove_dir_all(dir).unwrap();
    }
}
