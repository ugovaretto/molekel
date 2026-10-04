use crate::*;
use lin_alg::f32::Vec3;
use mcubes::{MarchingCubes, MeshSide};
use sha2::{Digest, Sha256};
use std::sync::OnceLock;

fn buffer<T>(count: usize, label: &str) -> Result<Vec<T>> {
    let mut values = Vec::new();
    values
        .try_reserve_exact(count)
        .map_err(|_| format!("Not enough memory for {label}; reduce grid resolution"))?;
    Ok(values)
}

fn sample_count(dims: [usize; 3]) -> Result<usize> {
    dims.into_iter()
        .try_fold(1usize, |n, d| n.checked_mul(d))
        .filter(|n| *n <= MAX_TRANSIENT_SAMPLES)
        .ok_or_else(|| "Grid exceeds the 256^3 transient sample memory budget".into())
}

/// A deliberately transparent f64 reference evaluator, shared by native and WASM.
pub fn basis_value(b: &BasisFunction, p: [f64; 3]) -> (f64, [f64; 3]) {
    let r: [f64; 3] = std::array::from_fn(|k| p[k] - b.center[k]);
    let r2: f64 = r.iter().map(|x| x * x).sum();
    let mut radial = 0.;
    let mut radial_factor = 0.;
    for (&a, &c) in b.exponents.iter().zip(&b.coefficients) {
        let v = c * (-a * r2).exp();
        radial += v;
        radial_factor += -2. * a * v;
    }
    let mut poly = 0.;
    let mut dp = [0.; 3];
    for t in &b.terms {
        poly += t.weight
            * (0..3)
                .map(|k| r[k].powi(t.powers[k] as i32))
                .product::<f64>();
        for (axis, d) in dp.iter_mut().enumerate() {
            if t.powers[axis] > 0 {
                *d += t.weight
                    * t.powers[axis] as f64
                    * (0..3)
                        .map(|k| r[k].powi((t.powers[k] - u32::from(k == axis)) as i32))
                        .product::<f64>();
            }
        }
    }
    (
        poly * radial,
        std::array::from_fn(|k| dp[k] * radial + poly * radial_factor * r[k]),
    )
}

pub fn evaluate(doc: &Document, field: &str, p: [f64; 3]) -> Result<(f64, [f64; 3])> {
    if !p.into_iter().all(f64::is_finite) {
        return Err("Evaluation coordinates must be finite".into());
    }
    let ao: Vec<_> = doc.basis.iter().map(|b| basis_value(b, p)).collect();
    if let Some(o) = doc.orbitals.iter().find(|o| o.id == field) {
        return Ok((
            ao.iter().zip(&o.coefficients).map(|(a, c)| a.0 * c).sum(),
            std::array::from_fn(|k| {
                ao.iter()
                    .zip(&o.coefficients)
                    .map(|(a, c)| a.1[k] * c)
                    .sum()
            }),
        ));
    }
    if let Some(d) = doc.densities.iter().find(|d| d.id == field) {
        let mut v = 0.;
        let mut grad = [0.; 3];
        for (i, a) in ao.iter().enumerate() {
            for (j, b) in ao.iter().enumerate() {
                let c = d.matrix[i * ao.len() + j];
                v += c * a.0 * b.0;
                for (k, component) in grad.iter_mut().enumerate() {
                    *component += c * (a.1[k] * b.0 + a.0 * b.1[k]);
                }
            }
        }
        return Ok((v, grad));
    }
    Err("Analytic evaluation requires an orbital or density matrix".into())
}

fn basis_scalar_value(b: &BasisFunction, p: [f64; 3]) -> f64 {
    let r: [f64; 3] = std::array::from_fn(|k| p[k] - b.center[k]);
    let r2: f64 = r.iter().map(|x| x * x).sum();
    let radial: f64 = b
        .exponents
        .iter()
        .zip(&b.coefficients)
        .map(|(&a, &c)| c * (-a * r2).exp())
        .sum();
    let poly: f64 = b
        .terms
        .iter()
        .map(|t| {
            t.weight
                * (0..3)
                    .map(|k| r[k].powi(t.powers[k] as i32))
                    .product::<f64>()
        })
        .sum();
    poly * radial
}

enum ScalarField<'a> {
    Orbital(&'a [f64]),
    Density(Vec<f64>),
    FullDensity(&'a [f64]),
}

impl<'a> ScalarField<'a> {
    fn new(doc: &'a Document, field: &str) -> Result<Self> {
        if let Some(o) = doc.orbitals.iter().find(|o| o.id == field) {
            return Ok(Self::Orbital(&o.coefficients));
        }
        if let Some(d) = doc.densities.iter().find(|d| d.id == field) {
            let n = doc.basis.len();
            let count = n
                .checked_add(1)
                .and_then(|next| n.checked_mul(next))
                .map(|count| count / 2)
                .ok_or("Density matrix allocation overflow")?;
            let mut upper = buffer(count, "density coefficients")?;
            for i in 0..n {
                for j in i..n {
                    // Real AO products commute, even for a nonsymmetric matrix.
                    let c = if i == j {
                        d.matrix[i * n + i]
                    } else {
                        d.matrix[i * n + j] + d.matrix[j * n + i]
                    };
                    if !c.is_finite() {
                        return Ok(Self::FullDensity(&d.matrix));
                    }
                    upper.push(c);
                }
            }
            return Ok(Self::Density(upper));
        }
        Err("Analytic evaluation requires an orbital or density matrix".into())
    }

    fn value(&self, ao: &[f64]) -> f64 {
        match self {
            Self::Orbital(c) => ao.iter().zip(*c).map(|(a, c)| a * c).sum(),
            Self::Density(c) => {
                let mut index = 0;
                let mut value = 0.;
                for (i, a) in ao.iter().enumerate() {
                    for b in &ao[i..] {
                        let weighted = c[index] * a;
                        // Folding coefficients must not overflow an otherwise finite product.
                        value += if weighted.is_infinite()
                            || (weighted == 0. && c[index] != 0. && *a != 0.)
                        {
                            c[index] * (a * b)
                        } else {
                            weighted * b
                        };
                        index += 1;
                    }
                }
                value
            }
            Self::FullDensity(c) => {
                let mut value = 0.;
                for (i, a) in ao.iter().enumerate() {
                    for (j, b) in ao.iter().enumerate() {
                        value += c[i * ao.len() + j] * a * b;
                    }
                }
                value
            }
        }
    }
}

pub fn source_hash(doc: &Document, field: &str) -> Result<String> {
    doc.check_field(field)?;
    let mut value = serde_json::json!({"basis":doc.basis, "orbital":doc.orbitals.iter().find(|o| o.id == field),
        "density":doc.densities.iter().find(|d| d.id == field), "grid":doc.grids.iter().find(|g| g.id == field)});
    // JSON transports can fold -0 into +0; neither changes a real Gaussian field.
    fn canonicalize_zero(value: &mut serde_json::Value) {
        match value {
            serde_json::Value::Number(n) if n.is_f64() && n.as_f64() == Some(0.) => {
                *value = serde_json::json!(0.0);
            }
            serde_json::Value::Array(values) => values.iter_mut().for_each(canonicalize_zero),
            serde_json::Value::Object(values) => values.values_mut().for_each(canonicalize_zero),
            _ => (),
        }
    }
    canonicalize_zero(&mut value);
    value.sort_all_objects();
    Ok(format!(
        "{:x}",
        Sha256::digest(serde_json::to_vec(&value).map_err(|e| e.to_string())?)
    ))
}

pub fn sample(doc: &Document, field: &str, resolution: usize) -> Result<Grid> {
    doc.validate()?;
    doc.check_field(field)?;
    if !(12..=256).contains(&resolution) {
        return Err("Preview resolution must be between 12 and 256".into());
    }
    if let Some(g) = doc.grids.iter().find(|g| g.id == field) {
        if g.values.iter().any(|v| !(*v as f32).is_finite()) {
            return Err("Grid values exceed rendering precision".into());
        }
        let dims = g.dims.map(|n| n.min(resolution));
        if dims == g.dims {
            let mut values = buffer(g.values.len(), "sampled field")?;
            values.extend_from_slice(&g.values);
            return Ok(Grid {
                id: g.id.clone(),
                label: g.label.clone(),
                quantity: g.quantity.clone(),
                origin: g.origin,
                axes: g.axes,
                dims,
                values,
            });
        }
        let scales: [f64; 3] =
            std::array::from_fn(|k| (g.dims[k] - 1) as f64 / (dims[k] - 1) as f64);
        let mut out = Grid {
            id: format!("samples-{field}"),
            label: g.label.clone(),
            quantity: g.quantity.clone(),
            origin: g.origin,
            axes: std::array::from_fn(|k| g.axes[k].map(|v| v * scales[k])),
            dims,
            values: buffer(sample_count(dims)?, "sampled field")?,
        };
        for z in 0..dims[2] {
            for y in 0..dims[1] {
                for x in 0..dims[0] {
                    let p = [
                        x as f64 * scales[0],
                        y as f64 * scales[1],
                        z as f64 * scales[2],
                    ];
                    let lo = p.map(|v| v.floor() as usize);
                    let hi = std::array::from_fn::<_, 3, _>(|k| (lo[k] + 1).min(g.dims[k] - 1));
                    let f = std::array::from_fn::<_, 3, _>(|k| p[k] - lo[k] as f64);
                    let mut v = 0.;
                    for dz in 0..2 {
                        for dy in 0..2 {
                            for dx in 0..2 {
                                let sides = [dx, dy, dz];
                                let i = std::array::from_fn::<_, 3, _>(|k| {
                                    if sides[k] == 0 { lo[k] } else { hi[k] }
                                });
                                let weight = (0..3)
                                    .map(|k| if sides[k] == 0 { 1. - f[k] } else { f[k] })
                                    .product::<f64>();
                                v +=
                                    weight * g.values[(i[2] * g.dims[1] + i[1]) * g.dims[0] + i[0]];
                            }
                        }
                    }
                    if !(v as f32).is_finite() {
                        return Err("Grid values exceed rendering precision".into());
                    }
                    out.values.push(v);
                }
            }
        }
        return Ok(out);
    }
    let mut lo = [f64::INFINITY; 3];
    let mut hi = [f64::NEG_INFINITY; 3];
    let min_exp = doc
        .basis
        .iter()
        .flat_map(|b| &b.exponents)
        .copied()
        .fold(f64::INFINITY, f64::min);
    let margin = (12. / min_exp).sqrt();
    if !margin.is_finite() || margin > 100. {
        return Err("Diffuse basis exceeds the preview domain budget".into());
    }
    let scalar = ScalarField::new(doc, field)?;
    for b in &doc.basis {
        for k in 0..3 {
            lo[k] = lo[k].min(b.center[k] - margin);
            hi[k] = hi[k].max(b.center[k] + margin);
        }
    }
    let axes = std::array::from_fn(|k| {
        std::array::from_fn(|j| {
            if k == j {
                (hi[k] - lo[k]) / (resolution - 1) as f64
            } else {
                0.
            }
        })
    });
    let mut grid = Grid {
        id: format!("samples-{field}"),
        label: field.into(),
        quantity: if doc.orbitals.iter().any(|o| o.id == field) {
            "orbital"
        } else {
            "density"
        }
        .into(),
        origin: lo,
        axes,
        dims: [resolution; 3],
        values: buffer(sample_count([resolution; 3])?, "sampled field")?,
    };
    let mut ao = buffer(doc.basis.len(), "basis values")?;
    ao.resize(doc.basis.len(), 0.);
    for z in 0..resolution {
        for y in 0..resolution {
            for x in 0..resolution {
                let p = grid.position([x as f64, y as f64, z as f64]);
                for (value, basis) in ao.iter_mut().zip(&doc.basis) {
                    *value = basis_scalar_value(basis, p);
                }
                let v = scalar.value(&ao);
                if !v.is_finite() || (v as f32).is_infinite() {
                    return Err("Field overflow".into());
                }
                grid.values.push(v);
            }
        }
    }
    Ok(grid)
}

const CORNERS: [[usize; 3]; 8] = [
    [0, 0, 0],
    [1, 0, 0],
    [1, 1, 0],
    [0, 1, 0],
    [0, 0, 1],
    [1, 0, 1],
    [1, 1, 1],
    [0, 1, 1],
];

fn case_vertex_counts() -> &'static [usize; 256] {
    static COUNTS: OnceLock<[usize; 256]> = OnceLock::new();
    COUNTS.get_or_init(|| {
        // The pinned mesher keeps its tables private; obtain counts through its public API.
        std::array::from_fn(|mask| {
            let mut values = vec![1.; 8];
            for (corner, [x, y, z]) in CORNERS.into_iter().enumerate() {
                if mask & (1 << corner) != 0 {
                    values[(z * 2 + y) * 2 + x] = 0.;
                }
            }
            MarchingCubes::new(
                (2, 2, 2),
                (1., 1., 1.),
                (1., 1., 1.),
                Vec3::new_zero(),
                values,
                0.5,
            )
            .expect("A 2^3 probe has exactly eight values")
            .generate(MeshSide::OutsideOnly)
            .vertices
            .len()
        })
    })
}

fn mesh_vertex_count(values: &[f32], dims: [usize; 3], iso: f32) -> Result<usize> {
    let count = sample_count(dims)?;
    if dims.iter().any(|n| *n < 2) || values.len() != count {
        return Err("Invalid mesh sample dimensions/count".into());
    }
    let [nx, ny, nz] = dims;
    let offsets = CORNERS.map(|[x, y, z]| (z * ny + y) * nx + x);
    let counts = case_vertex_counts();
    let mut vertices = 0usize;
    for z in 0..nz - 1 {
        for y in 0..ny - 1 {
            for x in 0..nx - 1 {
                let base = (z * ny + y) * nx + x;
                let mut mask = 0;
                for (corner, offset) in offsets.into_iter().enumerate() {
                    if values[base + offset] < iso {
                        mask |= 1 << corner;
                    }
                }
                vertices = vertices
                    .checked_add(counts[mask])
                    .filter(|count| *count <= MAX_VERTICES)
                    .ok_or("Surface exceeds the 2,000,000-vertex memory budget; reduce resolution or change the isovalue")?;
            }
        }
    }
    Ok(vertices)
}

pub fn mesh(
    grid: &Grid,
    iso: f64,
    field: &str,
    hash: &str,
    color: &str,
    opacity: f64,
) -> Result<Surface> {
    grid.validate_transient()?;
    if !iso.is_finite() || iso == 0. || !(iso as f32).is_finite() {
        return Err("A finite nonzero isovalue is required".into());
    }
    let [nx, ny, nz] = grid.dims;
    // mcubes get_value() uses x-fastest storage, independent of its loop order.
    let sign = iso.signum();
    let mut values = buffer(grid.values.len(), "marching-cubes input")?;
    for value in &grid.values {
        let value = (sign * value) as f32;
        if !value.is_finite() {
            return Err("Grid values exceed rendering precision".into());
        }
        values.push(value);
    }
    let vertex_count = mesh_vertex_count(&values, grid.dims, iso.abs() as f32)?;
    let coordinates = vertex_count
        .checked_mul(3)
        .ok_or("Surface coordinate allocation overflow")?;
    let positions = buffer(coordinates, "surface positions")?;
    let normals = buffer(coordinates, "surface normals")?;
    let mut indices = buffer(vertex_count, "surface indices")?;
    let result = if vertex_count == 0 {
        mcubes::Mesh {
            vertices: vec![],
            indices: vec![],
        }
    } else {
        MarchingCubes::new(
            (nx, ny, nz),
            (1., 1., 1.),
            (1., 1., 1.),
            Vec3::new_zero(),
            values,
            iso.abs() as f32,
        )
        .map_err(|e| e.to_string())?
        .generate(MeshSide::OutsideOnly)
    };
    if result.vertices.len() != vertex_count || result.indices.len() != vertex_count {
        return Err("Marching-cubes output disagrees with its allocation preflight".into());
    }
    for index in &result.indices {
        indices.push(u32::try_from(*index).map_err(|_| "Surface index overflow")?);
    }
    let mut surface = Surface {
        id: format!(
            "surface-{field}-{}",
            if iso > 0. { "positive" } else { "negative" }
        ),
        label: format!("{field} {iso:+.4}"),
        field: Some(field.into()),
        source_hash: Some(hash.into()),
        isovalue: Some(iso),
        algorithm: "mcubes-0.1.7-classic".into(),
        resolution: grid.dims,
        grid_origin: grid.origin,
        grid_axes: grid.axes,
        precision: "f64 field; f32 marching-cubes interpolation".into(),
        positions,
        normals,
        indices,
        color: color.into(),
        opacity,
        visible: true,
    };
    let a = grid.axes;
    let det = determinant(a);
    // Reciprocal basis maps grid gradients into physical-space normals.
    let cross = |u: [f64; 3], v: [f64; 3]| {
        [
            u[1] * v[2] - u[2] * v[1],
            u[2] * v[0] - u[0] * v[2],
            u[0] * v[1] - u[1] * v[0],
        ]
    };
    let reciprocal = [cross(a[1], a[2]), cross(a[2], a[0]), cross(a[0], a[1])];
    for v in result.vertices {
        surface.positions.extend(grid.position([
            v.posit.x as f64,
            v.posit.y as f64,
            v.posit.z as f64,
        ]));
        let g = [v.normal.x as f64, v.normal.y as f64, v.normal.z as f64];
        let n: [f64; 3] =
            std::array::from_fn(|k| (0..3).map(|j| reciprocal[j][k] * g[j] / det).sum());
        let len = n.iter().map(|v| v * v).sum::<f64>().sqrt();
        surface.normals.extend(n.map(|v| {
            if len > 1e-15 && len.is_finite() {
                v / len
            } else {
                0.
            }
        }));
    }
    if det < 0. {
        for t in surface.indices.chunks_exact_mut(3) {
            t.swap(1, 2);
        }
    }
    Ok(surface)
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn mesh_preflight_matches_all_library_cases_including_rounded_thresholds() {
        for mask in 0..256 {
            for (low, high, iso) in [(0., 1., 0.5_f32), (-1., 0., -0.5), (0.5 - 1e-12, 0.75, 0.5)] {
                let values: Vec<f32> = (0..8)
                    .map(|i| if mask & (1 << i) != 0 { low } else { high } as f32)
                    .collect();
                let expected = MarchingCubes::new(
                    (2, 2, 2),
                    (1., 1., 1.),
                    (1., 1., 1.),
                    Vec3::new_zero(),
                    values.clone(),
                    iso,
                )
                .unwrap()
                .generate(MeshSide::OutsideOnly);
                assert_eq!(
                    mesh_vertex_count(&values, [2; 3], iso).unwrap(),
                    expected.vertices.len(),
                    "case {mask}, threshold {iso}"
                );
                assert_eq!(expected.vertices.len(), expected.indices.len());
            }
        }
        assert_eq!(case_vertex_counts().iter().copied().max(), Some(15));
    }

    #[test]
    fn exact_mesh_preflight_accepts_the_largest_complete_triangle_budget() {
        let cells = MAX_VERTICES / 6;
        let mut dims = [2, 2, cells + 1];
        let mut values: Vec<f32> = (0..dims.into_iter().product())
            .map(|i| (i % 2) as f32)
            .collect();
        assert_eq!(mesh_vertex_count(&values, dims, 0.5).unwrap(), cells * 6);
        dims[2] += 1;
        values.extend([0., 1., 0., 1.]);
        let error = mesh_vertex_count(&values, dims, 0.5).unwrap_err();
        assert!(error.contains("2,000,000-vertex memory budget"), "{error}");
    }

    #[test]
    fn fallible_buffers_reject_capacity_overflow() {
        let error = buffer::<f64>(usize::MAX, "sampled field").unwrap_err();
        assert!(
            error.contains("Not enough memory for sampled field"),
            "{error}"
        );
    }

    #[test]
    fn scientific_hash_ignores_zero_sign_but_not_nonzero_changes() {
        let mut positive = fixtures::hydrogen_pair();
        positive.orbitals[0].coefficients[1] = 0.;
        positive.densities[0].matrix[1] = 0.;
        let mut negative = positive.clone();
        negative.basis[0].center[1] = -0.;
        negative.orbitals[0].coefficients[1] = -0.;
        negative.densities[0].matrix[1] = -0.;
        for field in ["bonding", "total"] {
            assert_eq!(source_hash(&positive, field), source_hash(&negative, field));
        }
        negative.orbitals[0].coefficients[1] = 1e-15;
        assert_ne!(
            source_hash(&positive, "bonding"),
            source_hash(&negative, "bonding")
        );
    }
    #[test]
    fn electron_count_uses_overlap_and_grid_integral() {
        let d = fixtures::hydrogen_pair();
        let overlap = (-0.98_f64).exp();
        let p = &d.densities[0].matrix;
        let trace_ps = p[0] + p[3] + overlap * (p[1] + p[2]);
        assert!((trace_ps - 2.).abs() < 1e-14);
        let grid = sample(&d, "total", 48).unwrap();
        let integral = grid.values.iter().sum::<f64>() * determinant(grid.axes).abs();
        assert!((integral - 2.).abs() < 1e-6);
    }
    #[test]
    fn resampling_preserves_affine_extent_and_linear_values() {
        let mut d = Document::empty("affine test");
        let mut g = Grid {
            id: "g".into(),
            label: "g".into(),
            quantity: "test".into(),
            origin: [-1., 2., 3.],
            axes: [[0.2, 0., 0.], [0.1, 0.3, 0.], [0., 0., -0.4]],
            dims: [20, 16, 14],
            values: vec![],
        };
        for z in 0..14 {
            for y in 0..16 {
                for x in 0..20 {
                    g.values.push(x as f64 + 2. * y as f64 + 3. * z as f64);
                }
            }
        }
        let extent = g.position([19., 15., 13.]);
        d.grids.push(g);
        let sampled = sample(&d, "g", 12).unwrap();
        for (actual, expected) in sampled.position([11., 11., 11.]).iter().zip(extent) {
            assert!((actual - expected).abs() < 1e-12);
        }
        assert_eq!(*sampled.values.last().unwrap(), 88.);
    }
    #[test]
    fn imported_samples_reject_rendering_overflow_without_altering_source() {
        for dims in [[2; 3], [13; 3]] {
            let mut d = Document::empty("precision test");
            d.grids.push(Grid {
                id: "g".into(),
                label: "g".into(),
                quantity: "test".into(),
                origin: [0.; 3],
                axes: [[1., 0., 0.], [0., 1., 0.], [0., 0., 1.]],
                dims,
                values: vec![0.; dims.iter().product()],
            });
            for value in [f64::MAX, -f64::MAX, 2. * f64::from(f32::MAX)] {
                d.grids[0].values[0] = value;
                assert!(d.validate().is_ok());
                assert_eq!(
                    sample(&d, "g", 12).unwrap_err(),
                    "Grid values exceed rendering precision"
                );
                assert_eq!(d.grids[0].values[0], value);
            }
            d.grids[0].values[0] = f64::from(f32::MAX);
            let sampled = sample(&d, "g", 12).unwrap();
            assert_eq!(sampled.values[0], f64::from(f32::MAX));
            assert!(sampled.values.iter().all(|v| (*v as f32).is_finite()));
        }
    }
    #[test]
    fn normalized_gaussian_value_and_derivative() {
        let doc = fixtures::hydrogen_pair();
        let b = &doc.basis[0];
        let p = [b.center[0] + 0.3, 0.4, -0.2];
        let (v, g) = basis_value(b, p);
        let expected = (2.0 / std::f64::consts::PI).powf(0.75) * (-0.29_f64).exp();
        assert!((v - expected).abs() < 1e-14);
        for (k, r) in [0.3, 0.4, -0.2].iter().enumerate() {
            assert!((g[k] + 2. * r * v).abs() < 1e-14);
        }
    }
    #[test]
    fn density_matches_fractional_occupation_orbitals() {
        let doc = fixtures::hydrogen_pair();
        for p in [[0.; 3], [1.2, 0.4, -0.3], [-0.4, 0.2, 0.1]] {
            let psi = evaluate(&doc, "bonding", p).unwrap().0;
            let rho = evaluate(&doc, "total", p).unwrap().0;
            assert!((rho - 2. * psi * psi).abs() < 1e-14);
        }
        let open = fixtures::open_shell();
        let p = [0.4, 0.1, 0.7];
        let psi = evaluate(&open, "alpha-s", p).unwrap().0;
        assert!((evaluate(&open, "spin", p).unwrap().0 - 0.75 * psi * psi).abs() < 1e-14);
    }
    #[test]
    fn polynomial_gradients_include_nodes() {
        let mut b = fixtures::hydrogen_pair().basis.remove(0);
        b.terms = vec![
            Term {
                powers: [1, 0, 1],
                weight: 2.,
            },
            Term {
                powers: [0, 2, 0],
                weight: -1.,
            },
        ];
        for p in [[0.; 3], [0.4, 0.2, 0.8]] {
            let (_, g) = basis_value(&b, p);
            for k in 0..3 {
                let mut plus = p;
                let mut minus = p;
                plus[k] += 1e-5;
                minus[k] -= 1e-5;
                let numerical = (basis_value(&b, plus).0 - basis_value(&b, minus).0) / 2e-5;
                assert!((numerical - g[k]).abs() < 1e-8);
            }
        }
    }
    #[test]
    fn mesh_axis_order_and_reflection() {
        for sx in [0.2, -0.2] {
            let mut g = Grid {
                id: "g".into(),
                label: "plane".into(),
                quantity: "test".into(),
                origin: [1., 2., 3.],
                axes: [[sx, 0., 0.], [0.1, 0.3, 0.], [0., 0., 0.4]],
                dims: [4, 5, 6],
                values: vec![],
            };
            for _z in 0..6 {
                for _y in 0..5 {
                    for x in 0..4 {
                        g.values.push(x as f64);
                    }
                }
            }
            let s = mesh(&g, 1.5, "g", "hash", "#ffffff", 1.).unwrap();
            assert!(!s.indices.is_empty());
            for p in s.positions.chunks_exact(3) {
                let y = (p[1] - 2.) / 0.3;
                assert!((p[0] - (1. + 1.5 * sx + y * 0.1)).abs() < 1e-6);
            }
        }
    }
    #[test]
    fn invalid_inputs_are_rejected() {
        let doc = fixtures::hydrogen_pair();
        for invalid in [f64::NAN, f64::INFINITY, f64::NEG_INFINITY] {
            assert!(evaluate(&doc, "bonding", [0., invalid, 0.]).is_err());
        }
        let mut doc = fixtures::hydrogen_pair();
        doc.basis[0].exponents[0] = -1.;
        assert!(doc.validate().is_err());
        let mut doc = fixtures::hydrogen_pair();
        doc.densities[0].matrix.pop();
        assert!(doc.validate().is_err());
        let mut doc = fixtures::hydrogen_pair();
        doc.view.opacity = f64::NAN;
        assert!(doc.validate().is_err());
    }
}
