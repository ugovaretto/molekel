setenv OPENBABEL_INCLUDE_DIR /usr/local/include
setenv OPENBABEL_LINK_DIR /usr/local/lib
setenv IV_INCLUDE_DIR /usr/local/include
setenv IV_LINK_DIR /usr/local/lib
setenv MOIV_INCLUDE_DIR /usr/local/include
setenv MOIV_LINK_DIR /usr/local/lib
# Set VTK_DIR to the directory where VTKConfig.cmake resides
setenv VTK_DIR /usr/local/lib/vtk-5.0
# QT: CMake looks for the qmake executable so make sure that
# the first qmake in the path is the one matching the Qt version
# you want to use