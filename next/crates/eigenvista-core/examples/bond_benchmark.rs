use eigenvista_core::{Atom, BOHR_TO_ANGSTROM, bonds, import};
use std::time::Instant;

fn main() {
    let start = Instant::now();
    let pdb = import::pdb(include_str!("../../../../data/3POR.pdb"), "3POR.pdb").unwrap();
    println!(
        "3POR PDB parse + bonds: {} atoms, {} bonds, {:.2} ms",
        pdb.atoms.len(),
        pdb.bonds.len(),
        start.elapsed().as_secs_f64() * 1000.
    );
    for size in [10_000usize, 100_000] {
        let atoms: Vec<_> = (0..size)
            .map(|i| Atom {
                element: 6,
                position: [
                    ((i % 100) as f64) * 1.5 / BOHR_TO_ANGSTROM,
                    ((i / 100) % 100) as f64 * 1.5 / BOHR_TO_ANGSTROM,
                    (i / 10_000) as f64 * 1.5 / BOHR_TO_ANGSTROM,
                ],
            })
            .collect();
        let start = Instant::now();
        let result = bonds::perceive(&atoms, &[]).unwrap();
        println!(
            "{size} atom lattice: {} bonds, {} nearby visits vs {} all-pairs checks, {:.2} ms",
            result.bonds.len(),
            result.neighbor_visits,
            size * (size - 1) / 2,
            start.elapsed().as_secs_f64() * 1000.
        );
    }
}
