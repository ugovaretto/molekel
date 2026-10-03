# Analytical cube fixture

`signed-affine.cube` is first-party regression data, not a quantum-chemistry
calculation. Its two hydrogen atoms are 1.4 bohr apart so distance-based bond
inference must produce exactly one H-H bond. No AO basis, orbitals, or density
matrix are present.

The 7 x 7 x 7 grid uses bohr coordinates, origin (-3.6, -3.3, -3), and step
vectors (1, 0, 0), (0.2, 1, 0), (0, 0.1, 1). Each point stores the analytic
scalar

```text
f(x, y, z) = 0.8 (x + 0.25 y) exp[-0.7 (x^2 + 0.8 y^2 + 1.2 z^2)]
```

Values are rounded to 12 digits after the decimal in scientific notation.
The cube file traverses x outermost, then y, with z fastest; the imported
native grid has x fastest instead. Tests calculate the same explicit formula
at every known lattice point, independently of the application's importer,
and verify that save, conversion, and rendering retain the authoritative
scalar array and affine geometry.

The signed, localized, asymmetric lobes exercise both mesh signs and sampled
raycast/volume rendering. Hydrogen and its connecting bond are neutral gray,
so teal and pink canvas pixels are evidence of field rendering, not atoms.
This fixture has no third-party data or additional license requirements.
