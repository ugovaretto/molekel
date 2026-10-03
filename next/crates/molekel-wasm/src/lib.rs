use molekel_core::{Document, compute, fixtures};
use wasm_bindgen::prelude::*;
fn err(e: impl ToString) -> JsValue {
    JsValue::from_str(&e.to_string())
}
fn parse(s: &str) -> Result<Document, JsValue> {
    let d: Document = serde_json::from_str(s).map_err(err)?;
    d.validate().map_err(err)?;
    Ok(d)
}
#[wasm_bindgen]
pub fn example(open_shell: bool) -> String {
    serde_json::to_string(&if open_shell {
        fixtures::open_shell()
    } else {
        fixtures::hydrogen_pair()
    })
    .unwrap()
}
#[wasm_bindgen]
pub fn validate(json: &str) -> Result<(), JsValue> {
    parse(json).map(|_| ())
}
#[wasm_bindgen]
pub fn encode(json: &str) -> Result<Vec<u8>, JsValue> {
    molekel_format::encode(&parse(json)?).map_err(err)
}
#[wasm_bindgen]
pub fn decode(bytes: &[u8]) -> Result<String, JsValue> {
    serde_json::to_string(&molekel_format::decode(bytes).map_err(err)?).map_err(err)
}
#[wasm_bindgen]
pub fn import_text(text: &str, name: &str) -> Result<String, JsValue> {
    let result = molekel_import::import_bytes(text.as_bytes(), name).map_err(err)?;
    serde_json::to_string(&result.document).map_err(err)
}
#[wasm_bindgen]
pub fn import_document(bytes: &[u8], name: &str) -> Result<String, JsValue> {
    serde_json::to_string(&molekel_import::import_bytes(bytes, name).map_err(err)?).map_err(err)
}
#[wasm_bindgen]
pub fn sample(json: &str, field: &str, resolution: usize) -> Result<String, JsValue> {
    serde_json::to_string(&compute::sample(&parse(json)?, field, resolution).map_err(err)?)
        .map_err(err)
}
#[wasm_bindgen]
pub fn surfaces(json: &str, grid_json: &str, field: &str, iso: f64) -> Result<String, JsValue> {
    let d = parse(json)?;
    let g = serde_json::from_str(grid_json).map_err(err)?;
    let hash = compute::source_hash(&d, field).map_err(err)?;
    let mut surfaces = vec![];
    for (level, color) in [
        (iso, &d.view.positive_color),
        (-iso, &d.view.negative_color),
    ] {
        let s = compute::mesh(&g, level, field, &hash, color, d.view.opacity).map_err(err)?;
        if !s.indices.is_empty() {
            surfaces.push(s);
        }
    }
    serde_json::to_string(&surfaces).map_err(err)
}
#[wasm_bindgen]
pub fn point(json: &str, field: &str, x: f64, y: f64, z: f64) -> Result<f64, JsValue> {
    compute::evaluate(&parse(json)?, field, [x, y, z])
        .map(|v| v.0)
        .map_err(err)
}

/// Value followed by the x/y/z analytic derivatives in atomic units.
#[wasm_bindgen]
pub fn point_with_gradient(
    json: &str,
    field: &str,
    x: f64,
    y: f64,
    z: f64,
) -> Result<Vec<f64>, JsValue> {
    let (value, gradient) = compute::evaluate(&parse(json)?, field, [x, y, z]).map_err(err)?;
    Ok(vec![value, gradient[0], gradient[1], gradient[2]])
}
