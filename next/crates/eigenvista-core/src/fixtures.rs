use crate::*;

pub fn hydrogen_pair() -> Document {
    let mut d = Document::empty("Hydrogen pair / Gaussian model");
    d.id = "analytic-h2-v1".into();
    d.atoms = vec![
        Atom {
            element: 1,
            position: [-0.7, 0., 0.],
        },
        Atom {
            element: 1,
            position: [0.7, 0., 0.],
        },
    ];
    d.bonds = vec![[0, 1]];
    let norm = (2. / std::f64::consts::PI).powf(0.75);
    d.basis = d
        .atoms
        .iter()
        .map(|a| BasisFunction {
            center: a.position,
            exponents: vec![1.],
            coefficients: vec![norm],
            terms: vec![Term {
                powers: [0, 0, 0],
                weight: 1.,
            }],
        })
        .collect();
    let overlap = (-0.98_f64).exp();
    let c = 1. / (2. * (1. + overlap)).sqrt();
    let v = 1. / (2. * (1. - overlap)).sqrt();
    d.orbitals = vec![
        Orbital {
            id: "bonding".into(),
            label: "Bonding sigma".into(),
            spin: "spatial".into(),
            occupation: Some(2.),
            energy: None,
            coefficients: vec![c, c],
        },
        Orbital {
            id: "antibonding".into(),
            label: "Antibonding sigma*".into(),
            spin: "spatial".into(),
            occupation: Some(0.),
            energy: None,
            coefficients: vec![v, -v],
        },
    ];
    d.densities = vec![Density {
        id: "total".into(),
        label: "Total electron density".into(),
        kind: "total".into(),
        matrix: vec![2. * c * c; 4],
    }];
    d.view.field = Some("antibonding".into());
    d.provenance=vec!["Analytic two-center normalized s-Gaussian fixture, exponent 1 bohr^-2, separation 1.4 bohr. Not an ab initio calculation; no orbital energies assigned.".into()];
    d
}

pub fn open_shell() -> Document {
    let mut d = Document::empty("Fractional open-shell / Gaussian model");
    d.id = "analytic-open-shell-v1".into();
    d.atoms = vec![Atom {
        element: 1,
        position: [0.; 3],
    }];
    d.basis = vec![BasisFunction {
        center: [0.; 3],
        exponents: vec![1.],
        coefficients: vec![(2. / std::f64::consts::PI).powf(0.75)],
        terms: vec![Term {
            powers: [0, 0, 0],
            weight: 1.,
        }],
    }];
    d.orbitals = vec![
        Orbital {
            id: "alpha-s".into(),
            label: "Alpha s".into(),
            spin: "alpha".into(),
            occupation: Some(1.),
            energy: None,
            coefficients: vec![1.],
        },
        Orbital {
            id: "beta-s".into(),
            label: "Beta s".into(),
            spin: "beta".into(),
            occupation: Some(0.25),
            energy: None,
            coefficients: vec![1.],
        },
    ];
    d.densities = vec![
        Density {
            id: "total".into(),
            label: "Total density".into(),
            kind: "total".into(),
            matrix: vec![1.25],
        },
        Density {
            id: "spin".into(),
            label: "Spin density".into(),
            kind: "spin".into(),
            matrix: vec![0.75],
        },
    ];
    d.view.field = Some("spin".into());
    d.provenance = vec![
        "Analytic fractional-occupation test fixture, not an electronic-structure calculation."
            .into(),
    ];
    d
}
