use serde_json::Value;
use std::io::{Cursor, Read, Write};
use zip::{ZipArchive, ZipWriter, write::SimpleFileOptions};

pub fn manifest(bytes: &[u8]) -> Value {
    let mut archive = ZipArchive::new(Cursor::new(bytes)).unwrap();
    let file = archive.by_name("manifest.json").unwrap();
    serde_json::from_reader(file).unwrap()
}

pub fn with_profile(bytes: &[u8], format: &str, features: &[&str]) -> Vec<u8> {
    let mut input = ZipArchive::new(Cursor::new(bytes)).unwrap();
    let mut output = ZipWriter::new(Cursor::new(vec![]));
    for index in 0..input.len() {
        let mut file = input.by_index(index).unwrap();
        let name = file.name().to_owned();
        let mut data = vec![];
        file.read_to_end(&mut data).unwrap();
        if name == "manifest.json" {
            let mut value: Value = serde_json::from_slice(&data).unwrap();
            value["format"] = format.into();
            value["required_features"] = serde_json::to_value(features).unwrap();
            data = serde_json::to_vec(&value).unwrap();
        }
        output
            .start_file(name, SimpleFileOptions::default())
            .unwrap();
        output.write_all(&data).unwrap();
    }
    output.finish().unwrap().into_inner()
}
