#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri_plugin_dialog::DialogExt;

fn validate_save_destination(
    path: &std::path::Path,
    protected_paths: &[String],
) -> Result<(), String> {
    if !path.extension().is_some_and(|ext| {
        ext.eq_ignore_ascii_case("eigenvista") || ext.eq_ignore_ascii_case("molekel")
    }) {
        return Err("Save to a .eigenvista filename; original external-format files are not export destinations".into());
    }
    if protected_paths.len() > 256 {
        return Err("Too many protected source paths".into());
    }
    if let Ok(destination) = path.canonicalize() {
        for source in protected_paths {
            if std::path::Path::new(source).canonicalize().ok().as_ref() == Some(&destination) {
                return Err("Choose a new filename; the imported source cannot be replaced".into());
            }
        }
    }
    Ok(())
}

#[tauri::command]
async fn save_native(
    app: tauri::AppHandle,
    bytes: Vec<u8>,
    name: String,
    protected_paths: Option<Vec<String>>,
) -> Result<bool, String> {
    tauri::async_runtime::spawn_blocking(move || {
        eigenvista_format::decode(&bytes)?;
        let Some(path) = app
            .dialog()
            .file()
            .add_filter("EigenVista document", &["eigenvista", "molekel"])
            .set_file_name(name)
            .blocking_save_file()
        else {
            return Ok(false);
        };
        let path = path.into_path().map_err(|e| e.to_string())?;
        validate_save_destination(&path, protected_paths.as_deref().unwrap_or_default())?;
        eigenvista_format::native::save_atomic(&path, &bytes)?;
        Ok(true)
    })
    .await
    .map_err(|e| e.to_string())?
}

#[cfg(test)]
mod tests {
    use super::validate_save_destination;
    use std::path::Path;

    #[test]
    fn native_save_requires_native_extension_and_protects_import_sources() {
        let root = Path::new(env!("CARGO_MANIFEST_DIR")).join("../../../tmp");
        std::fs::create_dir_all(&root).unwrap();
        let path = root.join(format!(
            "native-destination-test-{}.eigenvista",
            std::process::id()
        ));
        std::fs::write(&path, "source bytes").unwrap();
        let protected = vec![path.to_string_lossy().into_owned()];
        assert!(validate_save_destination(&path, &protected).is_err());
        assert!(validate_save_destination(&path, &[]).is_ok());
        assert!(validate_save_destination(&path.with_extension("molden"), &[]).is_err());
        assert!(validate_save_destination(&path.with_extension("EIGENVISTA"), &[]).is_ok());
        assert!(validate_save_destination(&path.with_extension("MOLEKEL"), &[]).is_ok());
        #[cfg(unix)]
        {
            let alias = path.with_file_name(format!(
                "native-destination-alias-{}.eigenvista",
                std::process::id()
            ));
            std::os::unix::fs::symlink(&path, &alias).unwrap();
            assert!(validate_save_destination(&alias, &protected).is_err());
            std::fs::remove_file(alias).unwrap();
        }
        assert_eq!(std::fs::read(&path).unwrap(), b"source bytes");
        std::fs::remove_file(path).unwrap();
    }
}

#[tauri::command]
fn scientific_core_version() -> &'static str {
    concat!("eigenvista-core ", env!("CARGO_PKG_VERSION"))
}

fn main() {
    tauri::Builder::default()
        .plugin(tauri_plugin_dialog::init())
        .plugin(tauri_plugin_fs::init())
        .invoke_handler(tauri::generate_handler![
            scientific_core_version,
            save_native
        ])
        .run(tauri::generate_context!())
        .expect("EigenVista desktop failed to start");
}
