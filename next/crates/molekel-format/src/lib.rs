use molekel_core::{Document, Result};
use serde::{Deserialize, Serialize};
use serde_json::{Value, json};
use sha2::{Digest, Sha256};
use std::{
    collections::{BTreeMap, HashSet},
    io::{Cursor, Read, Write},
};
use zip::{CompressionMethod, ZipArchive, ZipWriter, write::SimpleFileOptions};

#[cfg(not(target_arch = "wasm32"))]
pub mod native;

const BUDGET: usize = 128 * 1024 * 1024;
const MANIFEST_BUDGET: usize = 8 * 1024 * 1024;
const PROFILE: &str = "molekel-preview-polynomial-v1";

#[derive(Serialize, Deserialize)]
struct Array {
    dtype: String,
    shape: Vec<usize>,
    entry: String,
    bytes: usize,
    sha256: String,
}
#[derive(Serialize, Deserialize)]
struct Manifest {
    format: String,
    version: [u32; 2],
    required_features: Vec<String>,
    coordinate_unit: String,
    arrays: BTreeMap<String, Array>,
    document: Value,
}

fn digest(bytes: &[u8]) -> String {
    format!("{:x}", Sha256::digest(bytes))
}

fn numeric_shape(v: &Value) -> Option<Vec<usize>> {
    match v {
        Value::Number(_) => Some(vec![]),
        Value::Array(a) if !a.is_empty() => {
            let child = numeric_shape(&a[0])?;
            if a.iter()
                .skip(1)
                .any(|v| numeric_shape(v).as_ref() != Some(&child))
            {
                return None;
            }
            Some(std::iter::once(a.len()).chain(child).collect())
        }
        _ => None,
    }
}
fn flatten(v: &Value, out: &mut Vec<Value>) {
    if let Value::Array(a) = v {
        for v in a {
            flatten(v, out);
        }
    } else {
        out.push(v.clone());
    }
}

fn pack(
    v: &mut Value,
    arrays: &mut BTreeMap<String, Array>,
    entries: &mut Vec<(String, Vec<u8>)>,
) -> Result<()> {
    if v.is_array()
        && let Some(shape) = numeric_shape(v)
    {
        let mut flat = vec![];
        flatten(v, &mut flat);
        let integer = flat
            .iter()
            .all(|v| v.as_u64().is_some_and(|n| n <= u32::MAX as u64));
        let mut bytes = vec![];
        for v in flat {
            if integer {
                bytes.extend((v.as_u64().ok_or("Invalid integer")? as u32).to_le_bytes());
            } else {
                bytes.extend(v.as_f64().ok_or("Invalid float")?.to_le_bytes());
            }
        }
        let id = format!("a{}", arrays.len());
        let entry = format!("arrays/{id}/0.bin");
        arrays.insert(
            id.clone(),
            Array {
                dtype: if integer { "u32" } else { "f64" }.into(),
                shape,
                entry: entry.clone(),
                bytes: bytes.len(),
                sha256: digest(&bytes),
            },
        );
        entries.push((entry, bytes));
        *v = json!({"$array":id});
    } else {
        match v {
            Value::Array(a) => {
                for v in a {
                    pack(v, arrays, entries)?;
                }
            }
            Value::Object(o) => {
                for v in o.values_mut() {
                    pack(v, arrays, entries)?;
                }
            }
            _ => {}
        }
    }
    Ok(())
}

pub fn encode(doc: &Document) -> Result<Vec<u8>> {
    doc.validate()?;
    let mut value = serde_json::to_value(doc).map_err(|e| e.to_string())?;
    let mut arrays = BTreeMap::new();
    let mut entries = vec![];
    pack(&mut value, &mut arrays, &mut entries)?;
    if entries.len() >= 8192 {
        return Err("Document has too many array entries for the preview profile".into());
    }
    let manifest = Manifest {
        format: "molekel".into(),
        version: [0, 1],
        required_features: vec![PROFILE.into()],
        coordinate_unit: "bohr".into(),
        arrays,
        document: value,
    };
    let manifest = serde_json::to_vec(&manifest).map_err(|e| e.to_string())?;
    if manifest.len() > MANIFEST_BUDGET
        || entries.iter().map(|e| e.1.len()).sum::<usize>() + manifest.len() > BUDGET
    {
        return Err("Native document exceeds preview budget".into());
    }
    let mut zip = ZipWriter::new(Cursor::new(vec![]));
    let options = SimpleFileOptions::default().compression_method(CompressionMethod::Deflated);
    zip.start_file("manifest.json", options)
        .map_err(|e| e.to_string())?;
    zip.write_all(&manifest).map_err(|e| e.to_string())?;
    for (name, bytes) in entries {
        zip.start_file(name, options).map_err(|e| e.to_string())?;
        zip.write_all(&bytes).map_err(|e| e.to_string())?;
    }
    Ok(zip.finish().map_err(|e| e.to_string())?.into_inner())
}

fn reshape(values: &[Value], shape: &[usize]) -> Value {
    if shape.len() == 1 {
        return Value::Array(values.to_vec());
    }
    let stride = shape[1..].iter().product();
    Value::Array(
        values
            .chunks_exact(stride)
            .map(|v| reshape(v, &shape[1..]))
            .collect(),
    )
}
fn unpack(v: &mut Value, arrays: &mut BTreeMap<String, Value>) -> Result<()> {
    if let Some(id) = v.get("$array") {
        let id = id.as_str().ok_or("Invalid array reference")?;
        if v.as_object().is_none_or(|o| o.len() != 1) {
            return Err("Malformed array reference".into());
        }
        *v = arrays
            .remove(id)
            .ok_or("Unknown or multiply referenced array")?;
    } else {
        match v {
            Value::Array(a) => {
                for v in a {
                    unpack(v, arrays)?;
                }
            }
            Value::Object(o) => {
                for v in o.values_mut() {
                    unpack(v, arrays)?;
                }
            }
            _ => {}
        }
    }
    Ok(())
}

pub fn decode(bytes: &[u8]) -> Result<Document> {
    if bytes.len() > BUDGET {
        return Err("File exceeds 128 MiB preview budget".into());
    }
    let mut zip = ZipArchive::new(Cursor::new(bytes)).map_err(|e| e.to_string())?;
    if zip.len() > 8192 {
        return Err("Too many ZIP entries".into());
    }
    let mut names = HashSet::new();
    let mut total = 0u64;
    for i in 0..zip.len() {
        let file = zip.by_index(i).map_err(|e| e.to_string())?;
        let name = file.name();
        if name.starts_with('/')
            || name.contains('\\')
            || name
                .split('/')
                .any(|s| s.is_empty() || s == ".." || s == ".")
            || !names.insert(name.to_owned())
            || file.unix_mode().is_some_and(|m| m & 0o170000 == 0o120000)
            || ![CompressionMethod::Stored, CompressionMethod::Deflated]
                .contains(&file.compression())
        {
            return Err("Unsafe, duplicate, or unsupported ZIP entry".into());
        }
        total = total
            .checked_add(file.size())
            .ok_or("ZIP length overflow")?;
        if total > BUDGET as u64 {
            return Err("Decompressed document exceeds preview budget".into());
        }
    }
    let manifest_file = zip
        .by_name("manifest.json")
        .map_err(|_| "Missing manifest.json")?;
    if manifest_file.size() > MANIFEST_BUDGET as u64 {
        return Err("Manifest too large".into());
    }
    let mut manifest_bytes = vec![];
    manifest_file
        .take(MANIFEST_BUDGET as u64 + 1)
        .read_to_end(&mut manifest_bytes)
        .map_err(|e| e.to_string())?;
    if manifest_bytes.len() > MANIFEST_BUDGET {
        return Err("Manifest exceeds inflated budget".into());
    }
    let mut manifest: Manifest =
        serde_json::from_slice(&manifest_bytes).map_err(|e| e.to_string())?;
    if manifest.format != "molekel"
        || manifest.version != [0, 1]
        || manifest.coordinate_unit != "bohr"
        || manifest.required_features != [PROFILE]
    {
        return Err("Unsupported native format version or required profile".into());
    }
    let mut decoded = BTreeMap::new();
    let mut used = HashSet::from(["manifest.json".to_string()]);
    for (id, a) in manifest.arrays {
        let width = match a.dtype.as_str() {
            "f64" => 8,
            "u32" => 4,
            _ => return Err("Unsupported array type".into()),
        };
        let n = a
            .shape
            .iter()
            .try_fold(1usize, |n, d| n.checked_mul(*d))
            .ok_or("Array shape overflow")?;
        if a.shape.is_empty()
            || a.shape.len() > 4
            || a.shape.contains(&0)
            || n.checked_mul(width) != Some(a.bytes)
            || a.bytes > BUDGET
            || !a.entry.starts_with("arrays/")
            || !used.insert(a.entry.clone())
        {
            return Err("Invalid array shape, size, or path".into());
        }
        let file = zip.by_name(&a.entry).map_err(|_| "Missing array entry")?;
        if file.size() != a.bytes as u64 {
            return Err("Array byte length mismatch".into());
        }
        let mut bytes = vec![];
        file.take(a.bytes as u64 + 1)
            .read_to_end(&mut bytes)
            .map_err(|e| e.to_string())?;
        if bytes.len() != a.bytes {
            return Err("Inflated array length mismatch".into());
        }
        if digest(&bytes) != a.sha256 {
            return Err("Array checksum mismatch".into());
        }
        let mut values = Vec::with_capacity(n);
        for b in bytes.chunks_exact(width) {
            if width == 4 {
                values.push(json!(u32::from_le_bytes(b.try_into().unwrap())));
            } else {
                let v = f64::from_le_bytes(b.try_into().unwrap());
                if !v.is_finite() {
                    return Err("Non-finite scientific array".into());
                }
                values.push(json!(v));
            }
        }
        decoded.insert(id, reshape(&values, &a.shape));
    }
    if used != names {
        return Err("Unreferenced entries are not supported by the preview profile".into());
    }
    unpack(&mut manifest.document, &mut decoded)?;
    if !decoded.is_empty() {
        return Err("Unreferenced arrays".into());
    }
    let doc: Document = serde_json::from_value(manifest.document).map_err(|e| e.to_string())?;
    doc.validate()?;
    Ok(doc)
}

#[cfg(test)]
mod tests {
    use super::*;
    use molekel_core::{compute, fixtures};
    #[test]
    fn saved_mesh_rejects_changed_inputs_but_allows_appearance_edits() {
        let mut d = fixtures::hydrogen_pair();
        let g = compute::sample(&d, "bonding", 24).unwrap();
        let hash = compute::source_hash(&d, "bonding").unwrap();
        d.surfaces
            .push(compute::mesh(&g, 0.08, "bonding", &hash, "#259d86", 0.6).unwrap());
        d.surfaces[0].opacity = 0.3;
        assert!(d.validate().is_ok());
        d.orbitals[0].coefficients[0] += 0.1;
        assert!(d.validate().is_err());
    }
    #[test]
    fn array_digest_is_checked() {
        let bytes = encode(&fixtures::open_shell()).unwrap();
        let mut input = ZipArchive::new(Cursor::new(bytes)).unwrap();
        let mut output = ZipWriter::new(Cursor::new(vec![]));
        for i in 0..input.len() {
            let mut f = input.by_index(i).unwrap();
            let name = f.name().to_owned();
            let mut b = vec![];
            f.read_to_end(&mut b).unwrap();
            if name == "arrays/a0/0.bin" {
                b[0] ^= 1;
            }
            output
                .start_file(name, SimpleFileOptions::default())
                .unwrap();
            output.write_all(&b).unwrap();
        }
        assert!(
            decode(&output.finish().unwrap().into_inner())
                .unwrap_err()
                .contains("checksum")
        );
    }
    #[test]
    fn quantum_mesh_roundtrip() {
        let mut d = fixtures::hydrogen_pair();
        let g = compute::sample(&d, "antibonding", 24).unwrap();
        let hash = compute::source_hash(&d, "antibonding").unwrap();
        d.surfaces
            .push(compute::mesh(&g, 0.08, "antibonding", &hash, "#259d86", 0.6).unwrap());
        let bytes = encode(&d).unwrap();
        assert!(bytes.starts_with(b"PK"));
        assert_eq!(decode(&bytes).unwrap(), d);
    }
    #[test]
    fn open_shell_roundtrip() {
        let d = fixtures::open_shell();
        assert_eq!(decode(&encode(&d).unwrap()).unwrap(), d);
    }
    #[test]
    fn corruption_and_truncation_rejected() {
        let mut b = encode(&fixtures::hydrogen_pair()).unwrap();
        assert!(decode(&b[..b.len() / 2]).is_err());
        b[0] = 0;
        assert!(decode(&b[..20]).is_err());
    }
    #[test]
    fn unsafe_paths_rejected() {
        let mut z = ZipWriter::new(Cursor::new(vec![]));
        z.start_file("../manifest.json", SimpleFileOptions::default())
            .unwrap();
        z.write_all(b"{}").unwrap();
        assert!(decode(&z.finish().unwrap().into_inner()).is_err());
    }
    #[test]
    fn unknown_required_profile_rejected() {
        let d = fixtures::hydrogen_pair();
        let bytes = encode(&d).unwrap();
        let mut source = ZipArchive::new(Cursor::new(bytes)).unwrap();
        let mut out = ZipWriter::new(Cursor::new(vec![]));
        for i in 0..source.len() {
            let mut f = source.by_index(i).unwrap();
            let name = f.name().to_owned();
            let mut b = vec![];
            f.read_to_end(&mut b).unwrap();
            if name == "manifest.json" {
                let mut m: Value = serde_json::from_slice(&b).unwrap();
                m["required_features"] = json!(["unknown"]);
                b = serde_json::to_vec(&m).unwrap();
            }
            out.start_file(name, SimpleFileOptions::default()).unwrap();
            out.write_all(&b).unwrap();
        }
        assert!(decode(&out.finish().unwrap().into_inner()).is_err());
    }
}
