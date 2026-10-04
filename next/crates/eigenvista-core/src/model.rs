use crate::Result;
use serde::{Deserialize, Serialize};
use std::collections::HashSet;

pub const BOHR_TO_ANGSTROM: f64 = 0.529177210903;
pub const MAX_SAMPLES: usize = 128 * 128 * 128;
pub const MAX_TRANSIENT_SAMPLES: usize = 256 * 256 * 256;
pub const MAX_VERTICES: usize = 2_000_000;
pub const MAX_BONDS: usize = 600_000;

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Atom {
    pub element: u16,
    pub position: [f64; 3],
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Term {
    pub powers: [u32; 3],
    pub weight: f64,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct BasisFunction {
    pub center: [f64; 3],
    pub exponents: Vec<f64>,
    pub coefficients: Vec<f64>,
    pub terms: Vec<Term>,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Orbital {
    pub id: String,
    pub label: String,
    pub spin: String,
    pub occupation: Option<f64>,
    pub energy: Option<f64>,
    pub coefficients: Vec<f64>,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Density {
    pub id: String,
    pub label: String,
    pub kind: String,
    pub matrix: Vec<f64>,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Grid {
    pub id: String,
    pub label: String,
    pub quantity: String,
    pub origin: [f64; 3],
    /// Step vectors for x, y, z in bohr; values are x-fastest.
    pub axes: [[f64; 3]; 3],
    pub dims: [usize; 3],
    pub values: Vec<f64>,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Surface {
    pub id: String,
    pub label: String,
    pub field: Option<String>,
    pub source_hash: Option<String>,
    pub isovalue: Option<f64>,
    pub algorithm: String,
    pub resolution: [usize; 3],
    pub grid_origin: [f64; 3],
    pub grid_axes: [[f64; 3]; 3],
    pub precision: String,
    pub positions: Vec<f64>,
    pub normals: Vec<f64>,
    pub indices: Vec<u32>,
    pub color: String,
    pub opacity: f64,
    pub visible: bool,
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct View {
    pub representation: String,
    pub positive_color: String,
    pub negative_color: String,
    pub opacity: f64,
    pub isovalue: f64,
    pub field: Option<String>,
}

impl Default for View {
    fn default() -> Self {
        Self {
            representation: "ball-stick".into(),
            positive_color: "#259d86".into(),
            negative_color: "#cb4e72".into(),
            opacity: 0.78,
            isovalue: 0.08,
            field: None,
        }
    }
}

#[derive(Clone, Debug, Serialize, Deserialize, PartialEq)]
pub struct Document {
    pub id: String,
    pub title: String,
    pub atoms: Vec<Atom>,
    pub bonds: Vec<[usize; 2]>,
    pub basis: Vec<BasisFunction>,
    pub orbitals: Vec<Orbital>,
    pub densities: Vec<Density>,
    pub grids: Vec<Grid>,
    pub surfaces: Vec<Surface>,
    pub view: View,
    pub provenance: Vec<String>,
}

fn finite(values: impl IntoIterator<Item = f64>) -> bool {
    values.into_iter().all(f64::is_finite)
}
pub fn valid_color(s: &str) -> bool {
    s.len() == 7 && s.starts_with('#') && s.as_bytes()[1..].iter().all(u8::is_ascii_hexdigit)
}
pub fn determinant(a: [[f64; 3]; 3]) -> f64 {
    a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1])
        - a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0])
        + a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0])
}
impl Grid {
    pub fn validate(&self) -> Result<()> {
        self.validate_samples(MAX_SAMPLES, "128^3 sample")
    }
    /// Calculation grids are transient; imported and saved grids retain their smaller budget.
    pub fn validate_transient(&self) -> Result<()> {
        self.validate_samples(MAX_TRANSIENT_SAMPLES, "256^3 transient sample")
    }
    fn validate_samples(&self, limit: usize, label: &str) -> Result<()> {
        let n = self.dims.iter().try_fold(1usize, |n, d| n.checked_mul(*d));
        if self.dims.iter().any(|d| *d < 2) || n.is_none_or(|n| n > limit || n != self.values.len())
        {
            return Err(format!(
                "Grid dimensions/count exceed the {label} budget or disagree"
            ));
        }
        if !finite(self.origin)
            || !finite(self.axes.into_iter().flatten())
            || determinant(self.axes).abs() < 1e-15
            || !finite(self.values.iter().copied())
        {
            return Err("Grid has invalid axes or non-finite values".into());
        }
        Ok(())
    }
    pub fn position(&self, ijk: [f64; 3]) -> [f64; 3] {
        std::array::from_fn(|k| {
            self.origin[k] + (0..3).map(|a| ijk[a] * self.axes[a][k]).sum::<f64>()
        })
    }
}
impl Document {
    pub fn empty(title: &str) -> Self {
        Self {
            id: title.into(),
            title: title.into(),
            atoms: vec![],
            bonds: vec![],
            basis: vec![],
            orbitals: vec![],
            densities: vec![],
            grids: vec![],
            surfaces: vec![],
            view: View::default(),
            provenance: vec![],
        }
    }
    pub fn validate(&self) -> Result<()> {
        if self.atoms.len() > 100_000
            || self.bonds.len() > MAX_BONDS
            || self.basis.len() > 256
            || self.orbitals.len() > 512
            || self.surfaces.len() > 128
            || self.grids.len() > 16
            || self.densities.len() > 32
        {
            return Err("Document exceeds the current preview's resource budget".into());
        }
        if self
            .atoms
            .iter()
            .any(|a| a.element == 0 || a.element > 118 || !finite(a.position))
        {
            return Err("Invalid atom or coordinate".into());
        }
        if self
            .bonds
            .iter()
            .any(|b| b[0] == b[1] || b.iter().any(|i| *i >= self.atoms.len()))
        {
            return Err("Invalid bond indices".into());
        }
        for b in &self.basis {
            if !finite(b.center)
                || b.exponents.is_empty()
                || b.exponents.len() > 64
                || b.terms.is_empty()
                || b.terms.len() > 32
                || b.exponents.len() != b.coefficients.len()
                || b.exponents.iter().any(|v| !v.is_finite() || *v <= 0.)
                || !finite(b.coefficients.iter().copied())
                || b.terms.iter().any(|t| {
                    !t.weight.is_finite()
                        || t.powers.iter().any(|p| *p > 4)
                        || t.powers.iter().sum::<u32>() > 4
                })
            {
                return Err(
                    "Invalid Gaussian basis; only explicit polynomials through G are supported"
                        .into(),
                );
            }
        }
        let n = self.basis.len();
        for o in &self.orbitals {
            let max_occ = if o.spin == "spatial" { 2. } else { 1. };
            if n == 0
                || o.coefficients.len() != n
                || !finite(o.coefficients.iter().copied())
                || !["spatial", "alpha", "beta"].contains(&o.spin.as_str())
                || o.occupation
                    .is_some_and(|v| !v.is_finite() || v < 0. || v > max_occ)
                || o.energy.is_some_and(|v| !v.is_finite())
            {
                return Err("Invalid orbital dimensions, spin, occupation, or energy".into());
            }
        }
        for d in &self.densities {
            if n == 0
                || d.matrix.len() != n * n
                || !finite(d.matrix.iter().copied())
                || !["total", "alpha", "beta", "spin", "difference", "transition"]
                    .contains(&d.kind.as_str())
            {
                return Err("Invalid density matrix dimensions or kind".into());
            }
        }
        let mut ids = HashSet::new();
        for id in self
            .orbitals
            .iter()
            .map(|o| &o.id)
            .chain(self.densities.iter().map(|d| &d.id))
            .chain(self.grids.iter().map(|g| &g.id))
            .chain(self.surfaces.iter().map(|s| &s.id))
        {
            if id.is_empty() || !ids.insert(id) {
                return Err("Empty or duplicate object ID".into());
            }
        }
        for g in &self.grids {
            g.validate()?;
        }
        for s in &self.surfaces {
            if s.positions.len() % 3 != 0
                || s.positions.len() / 3 > MAX_VERTICES
                || s.normals.len() != s.positions.len()
                || s.indices.len() % 3 != 0
                || s.indices.len() > MAX_VERTICES * 3
                || !finite(s.positions.iter().copied())
                || !finite(s.normals.iter().copied())
                || s.indices
                    .iter()
                    .any(|i| *i as usize >= s.positions.len() / 3)
                || s.isovalue.is_some_and(|v| !v.is_finite())
                || !s.opacity.is_finite()
                || !(0. ..=1.).contains(&s.opacity)
                || !valid_color(&s.color)
                || !finite(s.grid_origin)
                || !finite(s.grid_axes.into_iter().flatten())
                || determinant(s.grid_axes).abs() < 1e-15
            {
                return Err("Invalid surface geometry or material".into());
            }
            if let Some(f) = &s.field {
                self.check_field(f)?;
                if s.source_hash.as_deref() != Some(crate::compute::source_hash(self, f)?.as_str())
                {
                    return Err(
                        "Saved surface source hash does not match its scientific inputs".into(),
                    );
                }
            }
        }
        if !["ball-stick", "liquorice", "space-fill"].contains(&self.view.representation.as_str())
            || !valid_color(&self.view.positive_color)
            || !valid_color(&self.view.negative_color)
            || !self.view.opacity.is_finite()
            || !(0. ..=1.).contains(&self.view.opacity)
            || !self.view.isovalue.is_finite()
            || self.view.isovalue <= 0.
        {
            return Err("Invalid view settings".into());
        }
        if let Some(f) = &self.view.field {
            self.check_field(f)?;
        }
        Ok(())
    }
    pub fn check_field(&self, id: &str) -> Result<()> {
        if self.orbitals.iter().any(|o| o.id == id)
            || self.densities.iter().any(|d| d.id == id)
            || self.grids.iter().any(|g| g.id == id)
        {
            Ok(())
        } else {
            Err(format!("Unknown field: {id}"))
        }
    }
}
