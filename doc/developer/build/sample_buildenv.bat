SET OPENBABEL_INCLUDE_DIR=c:\include
SET OPENBABEL_LINK_DIR=c:\lib
SET IV_INCLUDE_DIR=c:\include
SET IV_LINK_DIR=c:\lib
SET MOIV_INCLUDE_DIR=c:\include
SET MOIV_LINK_DIR=c:\lib
REM VTK: set to directory where VTKConfig.cmake resides
SET VTK_DIR=c:\lib\vtk-5.0
REM QT: CMake looks for the qmake executable so make sure that
REM the first qmake in the path is the one matching the Qt version
REM you want to use