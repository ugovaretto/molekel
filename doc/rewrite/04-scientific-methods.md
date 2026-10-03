# Scientific computation and rendering methods

Use one validated scalar-field model for all rendering methods. A field can be defined analytically by basis functions and coefficients, or by samples on an affine grid. Meshing, volume rendering, and raycasting consume that model without changing its units or scientific meaning.

The choices below are proposed implementation methods. Library performance and target-device support still require the experiments in the [validation plan](07-validation-and-delivery.md).

## Basis functions and orbitals

For real orbitals, evaluate

\[
\psi_i(\mathbf r)=\sum_\mu C_{i\mu}\phi_\mu(\mathbf r).
\]

The proposed native representation defines each Gaussian AO component explicitly:

\[
\phi_\mu(\mathbf r)=
\left(\sum_t w_{\mu t}x^{a_t}y^{b_t}z^{c_t}\right)
\left(\sum_p d_{\mu p}e^{-\alpha_p|\mathbf r-\mathbf R_\mu|^2}\right),
\]

where `x,y,z` are displacements from its center in bohr. The stored angular weights and radial coefficients include the intended normalization. The reader applies no additional normalization. Shared exponent arrays keep shells compact. An optimized library adapter may transform this representation internally, but must produce the same values.

For a simple Cartesian primitive, the familiar normalization is

\[
N(\alpha,l,m,n)=
\left(\frac{2\alpha}{\pi}\right)^{3/4}
\sqrt{\frac{(4\alpha)^{l+m+n}}
{(2l-1)!!(2m-1)!!(2n-1)!!}},
\]

with `(-1)!! = 1`. Primitive normalization and contracted-function normalization are separate. A converter must determine which factors its source already contains. Multiplying all coefficients by another plausible normalization can produce attractive but wrong orbitals.

Use [gau2grid](https://gau2grid.readthedocs.io/en/stable/index.html) as the first optimized Gaussian collocation candidate. Its documented [component orders](https://gau2grid.readthedocs.io/en/stable/order.html) make clear that AO order is part of the interface. Its derivatives can supply normals and shrinkwrap gradients. Confirm general contractions, angular momentum, normalization, and native/WASM behavior with fixtures before adoption. The explicit polynomial representation also supplies a small, independently checkable reference evaluator and the shader input definition.

Proposed compute sequence:

1. Validate finite coefficients, positive exponents, AO count, centers, component definitions, and unit conversions.
2. Evaluate AOs in spatial blocks. Reuse center displacements and exponentials across components of a shell.
3. Contract only the selected orbital coefficients, or a batch of requested orbitals.
4. Use conservative shell screening tied to a stated error budget. An exponent-only radial cutoff is insufficient for high-order polynomials, diffuse shells, or large coefficients.
5. Retain original data and reference calculations in f64. Quantize derived rendering grids to f32 only after checking the error.
6. Cancel between blocks and publish only complete artifacts.

Do not allocate an AO value matrix for every point of a large volume. At 256 cubed points and 1,000 AOs, f64 AO values alone would need about 125 GiB. Bounded blocks avoid this unnecessary allocation.

## Density and electron count

For orbitals with explicit occupations `f_i`,

\[
P_{\mu\nu}=\sum_i f_i C_{i\mu}C_{i\nu},\qquad
\rho(\mathbf r)=\sum_{\mu\nu}P_{\mu\nu}
\phi_\mu(\mathbf r)\phi_\nu(\mathbf r).
\]

The equivalent orbital form is `rho = sum_i f_i psi_i^2`. Use it when it is cheaper and the density actually comes from those orbitals. For real symmetric matrices, use the diagonal plus twice the lower triangle; for a supplied correlated density, use that matrix directly. PySCF's [cube-generation source](https://pyscf.org/_modules/pyscf/tools/cubegen.html) is one independent implementation to compare against.

Restricted spatial-orbital occupations can range from zero to two. Unrestricted alpha and beta occupations are recorded separately; total density adds them and spin density subtracts them. Do not add another factor of two to occupations that already contain it. Fractional occupation values remain fractional.

AO bases are generally nonorthogonal. Electron count is `trace(P S)`, using the AO overlap matrix `S`, not `trace(P)`. Also check the spatial integral of density on converged grids. These checks depend on the represented electron population, including effective core potentials and incomplete orbital sets. Record whether a matrix is total, alpha, beta, spin, difference, transition, or another supported quantity; positivity is not a valid test for every density type.

Complex orbitals require conjugation and a different storage/evaluation profile. They are not covered by the initial real-valued formulas.

## Units and coordinate systems

Use bohr and hartree in authoritative quantum data. The renderer uses angstrom coordinates. Transform positions once at the scene boundary, and preserve the field's physical units. For normalized orbitals, amplitude has units `bohr^-3/2`; electron density has units `electrons/bohr^3`. Unknown cube quantities retain unknown scalar units until the user or converter resolves them.

A sampled grid has origin `o`, three step vectors forming matrix `A`, and dimensions `nx,ny,nz`:

\[
\mathbf r = \mathbf o + A(i,j,k)^T.
\]

Each dimension counts samples; there are one fewer cells in that direction. Canonical storage is x-fastest: `offset = (k * ny + j) * nx + i`. Cube input commonly uses a different traversal and must be transposed explicitly. Grid normals transform with the inverse transpose of `A`, and reflected axes require consistent triangle winding.

For a 3D texture, map a grid sample coordinate `i` to texel-center coordinate `(i + 0.5) / n`; handle the last sample and interpolation boundary explicitly. Test asymmetric fields to expose flips and half-voxel shifts.

## Molecular representations

Use instanced sphere and cylinder meshes first, sharing geometry and materials. Ball-and-stick has independent atom and bond scale. Liquorice uses rounded cylinder ends and matching join spheres. Space filling uses a documented van der Waals radius table rather than the covalent radii used for bond inference.

Use explicit bonds when available. For missing bonds, apply a tested chemistry-library adapter or a documented radius/distance heuristic using a spatial index. Preserve inferred status. A heuristic cannot reliably determine aromaticity, metal coordination, or all bond orders from XYZ coordinates. Allow corrections and persist them.

Only move to sphere/cylinder impostors after measurements justify them. Their intersection depth, near-plane clipping, picking, and silhouettes need additional tests; ordinary instancing is a useful correctness baseline.

## Marching cubes

Select the [MC33 C library](https://github.com/dvega68/MC33_c_library) as the first meshing candidate. It is associated with the published corrected interior-test implementation and exposes precision choices. Audit and pin a version, compile without aggressive unsafe floating-point assumptions, and test its native/WASM outputs. A library name is not a substitute for topology tests.

Use separate extractions for `psi = +tau` and `psi = -tau`. Share the same sampled field. Extract in grid coordinates and apply the affine transform once. Weld shared cell/chunk edges by stable indices, not loose positional tolerances that could merge nearby lobes.

A topology-aware extractor can preserve the topology of the chosen interpolant. It cannot recover a narrow lobe missed by the samples. Validate convergence as the grid is refined and the box expands. Detect intersections with the domain boundary and report clipping.

Use [scikit-image's Lewiner implementation](https://scikit-image.org/docs/stable/api/skimage.measure.html#skimage.measure.marching_cubes) as an independent meshing comparator. [VTK Flying Edges](https://vtk.org/doc/nightly/html/classvtkFlyingEdges3D.html) is a strong native/server performance candidate, but performance and topology behavior should be assessed separately.

Default smoothing is off. Optional mesh smoothing must be followed by projection to the original field and an error check. Pure Laplacian smoothing changes the geometry and can shrink surfaces away from the requested value.

## Shrinkwrap

The original [van Overveld and Wyvill method](https://cspages.ucalgary.ca/~blob/ps/shrinkwrp.pdf) deforms an initial triangulation and analyzes a restricted topology. A single enclosing sphere does not cover arbitrary disconnected orbital lobes and handles.

Proposed Molekel method: **projected adaptive shrinkwrap**. Start from all components of a topology-checked coarse MC33 mesh at the requested isovalue. Move vertices onto the analytic field, or the cube's interpolated field, and adapt edges to curvature and error. This is an engineering adaptation, not a claim to reproduce the paper's algorithm unchanged.

For `g(r) = f(r) - tau`, the projection step is

\[
\mathbf r' = \mathbf r - \frac{g(\mathbf r)}{\|\nabla g(\mathbf r)\|^2}\nabla g(\mathbf r).
\]

Use a displacement limit and line search. Near a zero gradient, refine or reject the step. Split long edges, collapse only when topology and error tests permit, apply tangential relaxation, and reproject. Check intersections, minimum triangle quality, boundary status, and component preservation.

Reinitialize when an isovalue change alters topology; do not keep stretching an incompatible previous mesh. If the initial sampling misses a component, shrinkwrap cannot discover it reliably. Repeat the seed extraction at higher resolution and compare components. A failed wrap keeps the validated seed mesh visible with a convergence message and never labels the seed as a successful wrapped result.

This interpretation delivers a distinct refinement mode but depends on marching cubes for topology discovery. A requirement for a fully independent shrinkwrap extractor would be a larger research task and should be specified explicitly.

## Direct volume rendering

Use shader ray integration through a 3D field texture. A signed transfer function maps scalar values to color and extinction. Threshold controls mask values or create a band around an isovalue; they do not create a mesh. [GPU Gems' volume-rendering chapter](https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-39-volume-rendering-techniques) supplies the emission/absorption and sampling foundations.

For step length `ds` and extinction `sigma`, opacity is `1 - exp(-sigma * ds)`. With premultiplied sample color `c`, accumulate front to back as `C += (1-A) * c` and `A += (1-A) * alpha`. Correct opacity for changes in sample spacing; otherwise changing quality changes apparent material density.

Intersect rays with the field domain, stop at opaque geometry, use early termination once transmittance is sufficiently small, and skip empty bricks using conservative min/max data and the current transfer function. Prefer gradients computed on demand initially to avoid a large gradient texture. Preserve negative values and a neutral zero region for signed orbital fields.

## Shader isosurface raycasting

Provide two explicit evaluation sources:

| Source | Method | Tradeoff |
| --- | --- | --- |
| Sampled field | Traverse cells, find the first or subsequent level-set intersections of the interpolated field, shade with gradients | Fast interaction after grid creation; limited by sampling resolution |
| Analytic orbital | Evaluate the Gaussian expression and derivatives along the ray using uploaded basis/coefficient data | Avoids a sampled-volume approximation; cost grows with primitives and ray evaluations |

For the sampled mode, trilinear interpolation restricted to a line within a cell is cubic. Cell traversal plus root isolation using stationary points and bracket refinement can identify crossings that a coarse uniform step misses. Handle near-tangent roots, multiple intersections, exact-zero plateaus, and numerical tolerances explicitly. A simple sign-change march is a useful prototype, not sufficient validation for thin lobes.

For the analytic mode, bound contributions over a ray interval and subdivide intervals that might contain a root. Polynomial interval bounds and radial exponential bounds can conservatively reject empty intervals; their sum must include coefficient signs. Refine candidate roots using bracketed methods, and use gradients for shading. Conservative interval arithmetic on f32 needs rounding/error margins verified against f64 reference samples. If uncertainty or work limits are exceeded, report it and offer the sampled mode instead of silently omitting intersections.

An orbital is not a signed distance field. Advancing by `abs(psi - tau)` as in naive sphere tracing can jump over the surface. Direct F-Rep raycasting does not require hardware triangle-raytracing support.

For the positive region `psi >= tau`, outward normals point along `-grad(psi)`; for the negative region `psi <= -tau`, they point along `+grad(psi)`. Reconcile this convention with library triangle winding and reflected transforms. Write the actual hit depth for correct atom and mesh occlusion.

Direct analytic rendering is a required mode for the supported workload envelope, but it is not assumed to be the fastest mode. Small reference molecules must pass first; larger fields can use the sampled mode with the choice visible to the user.

## Transparency and compositing

Ordinary object sorting is insufficient for intersecting translucent lobes and meshes. Use depth peeling as the accuracy reference and evaluate weighted blended order-independent transparency for interactive mesh previews. Label approximate compositing where visible differences matter.

Volumes complicate this further: a translucent mesh can lie inside a volume. For the validated quality mode, collect ordered surface layers and integrate the volume between layer depths, compositing each layer in order. Multiple overlapping volumes require joint integration of their optical contributions, not sorting bounding boxes. Cap interactive work, report layer truncation, and let a still-image render use a larger budget.

This combined scene is a first-stage architecture test. A material opacity slider alone does not demonstrate correct transparency.

## Memory and interaction budgets

| Cubic grid | f32 scalar data | f64 scalar data |
| --- | ---: | ---: |
| 128 cubed | 8 MiB | 16 MiB |
| 256 cubed | 64 MiB | 128 MiB |
| 512 cubed | 512 MiB | 1 GiB |

These are arithmetic sizes, not measured resident memory. CPU copies, GPU uploads, gradients, acceleration structures, and meshes add to them. A worst-case mesh can be much larger than the scalar field. Reserve memory before starting and cap both scalar and triangle allocations.

Use coarse previews during parameter changes, cancel superseded work, and refine after interaction stops. Initial target: 30 frames per second at a 1280 x 800 internal render size for the defined benchmark scene, with low-latency control feedback. These targets must be calibrated on actual Macs and browsers; they are not promises of current performance.
