use eigenvista_core::{Document, Grid, compute, fixtures};
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
    eigenvista_format::encode(&parse(json)?).map_err(err)
}
#[wasm_bindgen]
pub fn decode(bytes: &[u8]) -> Result<String, JsValue> {
    serde_json::to_string(&eigenvista_format::decode(bytes).map_err(err)?).map_err(err)
}
#[wasm_bindgen]
pub fn import_text(text: &str, name: &str) -> Result<String, JsValue> {
    let result = eigenvista_import::import_bytes(text.as_bytes(), name).map_err(err)?;
    serde_json::to_string(&result.document).map_err(err)
}
#[wasm_bindgen]
pub fn import_document(bytes: &[u8], name: &str) -> Result<String, JsValue> {
    serde_json::to_string(&eigenvista_import::import_bytes(bytes, name).map_err(err)?).map_err(err)
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
    mesh_surfaces(&d, &g, field, &hash, iso)
}

fn mesh_surfaces(
    d: &Document,
    g: &Grid,
    field: &str,
    hash: &str,
    iso: f64,
) -> Result<String, JsValue> {
    let mut surfaces = vec![];
    for (level, color) in [
        (iso, &d.view.positive_color),
        (-iso, &d.view.negative_color),
    ] {
        let s = compute::mesh(g, level, field, hash, color, d.view.opacity).map_err(err)?;
        if !s.indices.is_empty() {
            surfaces.push(s);
        }
    }
    serde_json::to_string(&surfaces).map_err(err)
}

/// Retain the authoritative f64 samples inside WASM instead of roundtripping grid JSON.
#[wasm_bindgen]
pub struct SampledField {
    grid: Grid,
    field: String,
    hash: String,
}

#[wasm_bindgen]
impl SampledField {
    #[wasm_bindgen(constructor)]
    pub fn new(json: &str, field: &str, resolution: usize) -> Result<SampledField, JsValue> {
        let document = parse(json)?;
        let hash = compute::source_hash(&document, field).map_err(err)?;
        Ok(Self {
            grid: compute::sample(&document, field, resolution).map_err(err)?,
            field: field.into(),
            hash,
        })
    }

    pub fn metadata(&self) -> Result<String, JsValue> {
        let grid = &self.grid;
        serde_json::to_string(&serde_json::json!({
            "id": grid.id,
            "label": grid.label,
            "quantity": grid.quantity,
            "origin": grid.origin,
            "axes": grid.axes,
            "dims": grid.dims,
        }))
        .map_err(err)
    }

    pub fn display_values(&self) -> Result<Vec<f32>, JsValue> {
        let mut values = Vec::new();
        values
            .try_reserve_exact(self.grid.values.len())
            .map_err(|_| {
                err("Insufficient memory for the display grid; reduce the grid resolution")
            })?;
        values.extend(self.grid.values.iter().map(|value| *value as f32));
        Ok(values)
    }

    pub fn surfaces(&self, json: &str, iso: f64) -> Result<String, JsValue> {
        let document = parse(json)?;
        if compute::source_hash(&document, &self.field).map_err(err)? != self.hash {
            return Err(err(
                "Cached samples do not match the current scientific inputs",
            ));
        }
        mesh_surfaces(&document, &self.grid, &self.field, &self.hash, iso)
    }
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
