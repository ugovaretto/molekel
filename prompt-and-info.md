# Molekel - molecular visualization for quantum chemistry

This is the repository of Molekel a molecular visualization tool for quantum chemistry.

This code is old developed up to 2010.

It's a multiplatform desktop application (MacOS, Linus, Windows) written in C++ using Qt and CMake using also
OpenGL and VTK for graphical rendering.

I need to rewrite so shat it works again on MacOS and ideally also as a web application.

There is no need to use the same frameworks or programming language.

All the documentaion is in the gh-pages branch and at this URL:  https://ugovaretto.github.io/molekel/

The important features for the v1.0 version are:

- Molecule rendering:
  - ball and stick representation
  - liqorice
  - Van der Waals radius
- Quantum chemistry:
  - molecular orbitals from basis set
  - density matrix surface
- Other:
  - surfaces from gaussian cube files .cube
  - any geometry from .obj file with the ability to specify the information
- file formats:
  - pdb
  - xyz
  - molden files (.molden) for quantum chemistry data
- Rendering:
  - read atom colors from text file
  - specify transparency and color for any surface (orbitals, density matrix, etc.)
  - implement rendering of molecular orbitals (which are volumes or F-Rep) using the following methods:
    1. isosurfaces with ability to specify the isovalue using marching cubes and shrinkwrap
    2. plain volume rendering with the ability to specify the isovalue and transfer function
    3. raycasting implemented in shaders

# Actions

1. Review documentaiton
2. Review code with focus on the important features from above
3. Research best methods to implement all the features
4. Decide which language and framework to use examples could include:
  1. full web application, client-server so that you can invoke tools (e.g. marching cubes) on the server and cache the results
  2. client web desktop application using e.g. Tauri, Rust and WebGPU with the Bevy engine
  3. rewrite using C++, Qt, VTK and Vulkan (with support for Metal using MoltenVK)
  4. create a MacOS-only application using only MacOS frameworks

After completing the above actions generate a documents detailing the findings, do not write code yet.
Also generate all the documents you might need to support the above actions and future development.
