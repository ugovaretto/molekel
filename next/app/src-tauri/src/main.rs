#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use tauri_plugin_dialog::DialogExt;

#[tauri::command]
async fn save_native(app: tauri::AppHandle, bytes: Vec<u8>, name: String) -> Result<bool, String> {
    tauri::async_runtime::spawn_blocking(move || {
        molekel_format::decode(&bytes)?;
        let Some(path) = app
            .dialog()
            .file()
            .add_filter("Molekel document", &["molekel"])
            .set_file_name(name)
            .blocking_save_file()
        else {
            return Ok(false);
        };
        let path = path.into_path().map_err(|e| e.to_string())?;
        molekel_format::native::save_atomic(&path, &bytes)?;
        Ok(true)
    })
    .await
    .map_err(|e| e.to_string())?
}

#[tauri::command]
fn scientific_core_version() -> &'static str {
    "molekel-core 0.1.0"
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
        .expect("Molekel desktop failed to start");
}
