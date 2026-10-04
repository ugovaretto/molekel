//! Nonperiodic display connectivity, not bond-order or aromaticity perception.
use crate::{Atom, BOHR_TO_ANGSTROM, MAX_BONDS, Result};
use kiddo::{ImmutableKdTree, SquaredEuclidean};
use pdbtbx::Element;
use std::collections::BTreeSet;

pub const TOLERANCE_ANGSTROM: f64 = 0.45;
pub const MIN_DISTANCE_ANGSTROM: f64 = 0.4;
const MAX_CANDIDATES: usize = 2_000_000;
const MAX_NEIGHBOR_VISITS: usize = 10_000_000;

pub struct PerceivedBonds {
    pub bonds: Vec<[usize; 2]>,
    pub neighbor_visits: usize,
    pub candidates: usize,
    pub coordination_rejections: usize,
}

impl PerceivedBonds {
    pub fn description(&self) -> String {
        format!(
            "Automatic bonds: {} connections; exact k-d-tree search, Pyykko-Atsumi single-bond covalent radii + 0.45 angstrom, minimum distance 0.4 angstrom. {} candidates rejected by light-element coordination limits. Connectivity only; bond orders, aromaticity and periodic images not inferred.",
            self.bonds.len(),
            self.coordination_rejections
        )
    }
}

pub fn covalent_radius(element: u16) -> Option<f64> {
    Element::new(element as usize).map(|e| e.atomic_radius().covalent_single)
}

fn coordination_limit(element: u16) -> usize {
    match element {
        1 | 9 => 1,
        5..=7 => 4,
        8 => 3,
        _ => usize::MAX,
    }
}

/// Explicit connections take precedence; remaining pairs are accepted shortest-first.
pub fn perceive(atoms: &[Atom], explicit: &[[usize; 2]]) -> Result<PerceivedBonds> {
    if atoms.len() > 100_000 || explicit.len() > MAX_BONDS {
        return Err("Bond detection exceeds the atom/connection budget".into());
    }
    let mut radii = Vec::with_capacity(atoms.len());
    let mut positions = Vec::with_capacity(atoms.len());
    for atom in atoms {
        radii.push(covalent_radius(atom.element).ok_or("Invalid element for bond detection")?);
        let p = atom.position.map(|v| v * BOHR_TO_ANGSTROM);
        if p.iter().any(|v| !v.is_finite() || v.abs() > 1e9) {
            return Err("Bond detection requires finite coordinates within 1e9 angstrom".into());
        }
        positions.push(p);
    }
    let mut bonds = BTreeSet::new();
    let mut degree = vec![0; atoms.len()];
    for &[a, b] in explicit {
        if a == b || a >= atoms.len() || b >= atoms.len() {
            return Err("Invalid explicit bond endpoints".into());
        }
        if bonds.insert([a.min(b), a.max(b)]) {
            degree[a] += 1;
            degree[b] += 1;
        }
    }
    let mut result = PerceivedBonds {
        bonds: vec![],
        neighbor_visits: 0,
        candidates: 0,
        coordination_rejections: 0,
    };
    if atoms.len() < 2 {
        return Ok(result);
    }
    let max_radius = radii.iter().copied().fold(0., f64::max);
    let tree = ImmutableKdTree::<f64, 3>::new_from_slice(&positions);
    let mut candidates = Vec::new();
    for (i, p) in positions.iter().enumerate() {
        let search_radius = radii[i] + max_radius + TOLERANCE_ANGSTROM;
        // The index uses squared Euclidean distances, including its radius argument.
        for neighbor in tree.within_unsorted::<SquaredEuclidean>(
            p,
            search_radius.powi(2) * (1. + 8. * f64::EPSILON),
        ) {
            result.neighbor_visits += 1;
            if result.neighbor_visits > MAX_NEIGHBOR_VISITS {
                return Err("Bond detection exceeded the neighbor budget; coordinates may be excessively dense".into());
            }
            let j = neighbor.item as usize;
            if j <= i || bonds.contains(&[i, j]) {
                continue;
            }
            let sum = radii[i] + radii[j];
            if neighbor.distance >= MIN_DISTANCE_ANGSTROM.powi(2)
                && neighbor.distance <= (sum + TOLERANCE_ANGSTROM).powi(2)
            {
                if candidates.len() == MAX_CANDIDATES {
                    return Err("Bond detection exceeded the candidate budget; coordinates may be excessively dense".into());
                }
                candidates.push((neighbor.distance / sum.powi(2), i, j));
            }
        }
    }
    result.candidates = candidates.len();
    candidates.sort_unstable_by(|a, b| a.0.total_cmp(&b.0).then(a.1.cmp(&b.1)).then(a.2.cmp(&b.2)));
    for (_, i, j) in candidates {
        if degree[i] >= coordination_limit(atoms[i].element)
            || degree[j] >= coordination_limit(atoms[j].element)
        {
            result.coordination_rejections += 1;
            continue;
        }
        if bonds.len() == MAX_BONDS {
            return Err("Bond count exceeds the preview budget".into());
        }
        bonds.insert([i, j]);
        degree[i] += 1;
        degree[j] += 1;
    }
    result.bonds = bonds.into_iter().collect();
    Ok(result)
}
