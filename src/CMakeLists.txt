#
# Molekel - Molecular Visualization Program
# Copyright (C) 2006, 2007, 2008, 2009 Swiss National Supercomputing Centre (CSCS)
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
# MA  02110-1301, USA.
#
# $Author$
# $Date$
# $Revision$
#

##################################################################################
#  Molekel version 5.x CMake file
#
##################################################################################


CMAKE_MINIMUM_REQUIRED( VERSION 2.4 )
#CMAKE_POLICY( VERSION 2.4 )
IF( COMMAND cmake_policy )
  cmake_policy( SET CMP0003 OLD )
ENDIF( COMMAND cmake_policy )


PROJECT(Molekel)

# Set Molekel build info, set to defaults and try to read the values from include file
SET( MOLEKEL_VERSION_MAJOR 5 )
SET( MOLEKEL_VERSION_MINOR 4 )
SET( MOLEKEL_VERSION_PATCH 0 )
SET( MOLEKEL_VERSION_BUILD 0 )
SET( MOLEKEL_VERSION_TYPE "development" )

# Create version file if it doesn't exist
IF( NOT EXISTS ${CMAKE_BINARY_DIR}/version_info.cmake )
  FILE( WRITE ${CMAKE_BINARY_DIR}/version_info.cmake "" )
ENDIF( NOT EXISTS ${CMAKE_BINARY_DIR}/version_info.cmake )    
INCLUDE( ${CMAKE_BINARY_DIR}/version_info.cmake )
  

SET( MOLEKEL_COPYRIGHT_INFO "Copyright - (c) 2006, 2007, 2008, 2009 CSCS" )
SET( MOLEKEL_BUILD_DATE __DATE__ )

SET( MOLEKEL_APP_VERSION_STRING
       "${MOLEKEL_VERSION_MAJOR}.${MOLEKEL_VERSION_MINOR}.${MOLEKEL_VERSION_PATCH}.${MOLEKEL_VERSION_BUILD}.${MOLEKEL_VERSION_TYPE}" )

# Create directory tree for distribution of binary package
#
#  - dist (release notes, license)
#     - bin | bundle (executable)
#     - resources (logo, images,...)
#     - data (sample dataset)
#     - shaders (GLSL shaders)
#     - doc (currently empty)

SET( MOLEKEL_DIST_DIR dist )

IF( APPLE )
  SET( MOLEKEL_BIN_DIR bundle )
ELSE( APPLE )
  SET( MOLEKEL_BIN_DIR bin )
ENDIF( APPLE )

SET( MOLEKEL_RES_DIR resources )
SET( MOLEKEL_DOC_DIR doc )
SET( MOLEKEL_SHADERS_DIR shaders )


FILE( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}" )
FILE( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_BIN_DIR}" )
FILE( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_RES_DIR}" )
FILE( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_DOC_DIR}" )
FILE( MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_SHADERS_DIR}" )

SET( MOLEKEL_RESOURCES_DIR "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_RES_DIR}" )
SET( MOLEKEL_DISTRIBUTION_DIR "${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}" )

# Update and copy file with version info to build dir
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/versioninfo.cpp.in
                ${CMAKE_BINARY_DIR}/versioninfo.cpp @ONLY )
# Copy image used as application icon to build dir
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/resources/cscslogo64x64.png
                ${MOLEKEL_RESOURCES_DIR}/Molekel.png COPYONLY )
# Copy atoms data to build dir; this is required because the old molekel code needs this data.
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/resources/molekelatoms.data
                ${MOLEKEL_RESOURCES_DIR}/molekelatoms.data COPYONLY )
# If Windows also copy win installer script
IF( WIN32 )
IF( NOT MSVC )
  CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/../install/win/setup.nsi
                  ${CMAKE_BINARY_DIR}/setup.nsi COPYONLY )
ENDIF( NOT MSVC)                  
ENDIF( WIN32 )
# Assign file names to text files
IF( WIN32 )
  SET( LICENSE_FILE license.txt )
  SET( NOTES_FILE notes.txt )
ELSE( WIN32 )
  SET( LICENSE_FILE license )
  SET( NOTES_FILE notes )
ENDIF( WIN32 )

# Copy license file to main distribution dir.
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/license
                "${MOLEKEL_DISTRIBUTION_DIR}/${LICENSE_FILE}" COPYONLY )
# Copy notes file to main distribution dir.
CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/notes
                "${MOLEKEL_DISTRIBUTION_DIR}/${NOTES_FILE}" COPYONLY )

IF( WIN32 )
 # Copy icon used for the application shortcut.
 CONFIGURE_FILE( ${CMAKE_SOURCE_DIR}/resources/Molekel.ico
                 ${MOLEKEL_RESOURCES_DIR}/Molekel.ico COPYONLY )
ENDIF( WIN32 )

################################################################################

#### LIBRARIES/FRAMEWORKS ####

#-------------------------------------------------------------------------------
# Setup libraries:
# - VTK 5.x                                http://www.vtk.org
# - QT 4.2.x                               http://www.trolltech.com
# - OpenBabel 2.x                          http://openbabel.sourceforge.net
# - OpenInventor / Coin 2.x                http://www.coin3d.org/
# - OpenMOIV ( Molecular Inventor ) 1.0.x  http://www.tecn.upf.es/openMOIV
# - GLEW                                   http://glew.sourceforge.net
# - Qwt                                    http://qwt.sourceforge.net
#-------------------------------------------------------------------------------

#### NOTE: when building with MinGW add the -mwindows compiler command line switch
####       to get rid of the console

#### VTK ####
FIND_PACKAGE(VTK)
IF(NOT VTK_FOUND)
  MESSAGE(FATAL_ERROR "Please set VTK_DIR.")
ENDIF(NOT VTK_FOUND)
INCLUDE(${VTK_USE_FILE})

#### QT4 ####
FIND_PACKAGE(Qt4)
IF(NOT QT4_FOUND)
  MESSAGE(FATAL_ERROR "Please set path to qmake manually")
ENDIF(NOT QT4_FOUND)
INCLUDE(${QT_USE_FILE})

#### OPEN BABEL ####
# OpenBabel include and lib directories. OpenBabel include directory is
# the directory where the openbabel include dir resides i.e. source files
# must be able to refere to OpenBabel include as #include <openbabel/... .h>.
# Try to read from environment variables first.
IF( UNIX AND NOT APPLE )
  SET( OPENBABEL_INCLUDE_DIR /usr/local/include/openbabel-2.0 CACHE PATH "OpenBabel include directory" )
  SET( OPENBABEL_LINK_DIR /usr/local/lib CACHE PATH "OpenBabel lib directory" )
ELSE( UNIX AND NOT APPLE )
  SET( OPENBABEL_INCLUDE_DIR $ENV{OPENBABEL_INCLUDE_DIR} CACHE PATH "OpenBabel include directory" )
  SET( OPENBABEL_LINK_DIR $ENV{OPENBABEL_LINK_DIR} CACHE PATH "OpenBabel lib directory" )
ENDIF( UNIX AND NOT APPLE )

###
# OpenInventor include and lib directories.
# Try to read from environment variables first.
IF( UNIX AND NOT APPLE )
  SET( IV_INCLUDE_DIR  /usr/local/include CACHE PATH "OpenInventor/Coin include directory" )
  SET( IV_LINK_DIR /usr/local/lib CACHE PATH "OpenInventor/Coin lib directory" )
ELSE( UNIX AND NOT APPLE )
  SET( IV_INCLUDE_DIR $ENV{IV_INCLUDE_DIR} CACHE PATH "OpenInventor/Coin include directory" )
  SET( IV_LINK_DIR $ENV{IV_LINK_DIR} CACHE PATH "OpenInventor/Coin lib directory" )
ENDIF( UNIX AND NOT APPLE )
SET( IV_DYNAMIC ON CACHE BOOL "Use OpenInventor dynamic library" )

# Set OpenInventor library to Coin or Inventor
# on Apple Coin installs as Inventor.framework
IF( APPLE )
  SET( IV_LIB "-framework Inventor" )
ELSE ( APPLE )
  SET( IV_LIB "Coin" )
ENDIF ( APPLE )

# NOTE: assumes Coin is being used; need to check what happens for
# other OpenInventor implementations
IF( ${IV_DYNAMIC} )
  ADD_DEFINITIONS( -DCOIN_DLL )
ELSE( ${IV_DYNAMIC} )
  ADD_DEFINITIONS( -DCOIN_NOT_DLL )
ENDIF( ${IV_DYNAMIC} )

#### OPEN MOIV ####
# OpenMOIV (Molecular Inventor) include and lib directories.
# Try to read from environment variables first.
IF( UNIX AND NOT APPLE )
  SET( MOIV_INCLUDE_DIR /usr/local/include CACHE PATH "OpenMOIV installation directory" )
  SET( MOIV_LINK_DIR /usr/local/lib CACHE PATH "OpenMOIV installation directory" )
ELSE( UNIX AND NOT APPLE )
  SET( MOIV_INCLUDE_DIR $ENV{MOIV_INCLUDE_DIR} CACHE PATH "OpenMOIV installation directory" )
  SET( MOIV_LINK_DIR $ENV{MOIV_LINK_DIR} CACHE PATH "OpenMOIV installation directory" )
ENDIF( UNIX AND NOT APPLE )
SET( MOIV_DYNAMIC ON CACHE BOOL "Use OpenMOIV dynamic library" )

IF( ${MOIV_DYNAMIC} )
  ADD_DEFINITIONS( -DCHEMKIT_DLL )
ELSE( ${MOIV_DYNAMIC} )
  ADD_DEFINITIONS( -DCHEMKIT_NOT_DLL )
ENDIF( ${MOIV_DYNAMIC} )

#### GLEW ####
# GLEW include and lib directories
# NOTE: glew.h must be in <GLEW include dir>/GL directory
SET( GLEW_INCLUDE_DIR  /usr/local/include CACHE PATH "GLEW include dir" )
SET( GLEW_LINK_DIR  /usr/local/lib CACHE PATH "GLEW link dir" )

### QWT ####
SET( QWT_INCLUDE_DIR /usr/local/include CACHE PATH "Qwt include dir" )
SET( QWT_LINK_DIR  /usr/local/lib CACHE PATH "Qwt link dir" )

#### OpenGL ####
IF( APPLE OR UNIX )
  FIND_PACKAGE( OpenGL )
ENDIF ( APPLE OR UNIX )


#### INCLUDE DIRS ####
INCLUDE_DIRECTORIES( ${OPENBABEL_INCLUDE_DIR}
                     ${IV_INCLUDE_DIR}
                     ${MOIV_INCLUDE_DIR} )

INCLUDE_DIRECTORIES( ${GLEW_INCLUDE_DIR} )

IF( APPLE OR UNIX )
  SET( OPENGL_INCLUDE_DIR /usr/X11R6/include )
ENDIF( APPLE OR UNIX )

INCLUDE_DIRECTORIES( ${OPENGL_INCLUDE_DIR} )

INCLUDE_DIRECTORIES( ${QWT_INCLUDE_DIR} )

#### PROBLEM: on Mac OS and Windows everything works fine; on linux
#### in case the environment variables are set all the variables are set and
#### do contain the right values at this point in the script,
#### the values are not however stored in the CACHE nor are
#### the values displayed in the ccmake window.
#### MESSAGE( ${OPENBABEL_INCLUDE_DIR} )
#### MESSAGE( ${IV_INCLUDE_DIR} )
#### MESSAGE( ${MOIV_INCLUDE_DIR} )


#### LIBRARY DIRS ####
LINK_DIRECTORIES( ${GLEW_LINK_DIR} )
LINK_DIRECTORIES( ${OPENBABEL_LINK_DIR}
                  ${IV_LINK_DIR}
                  ${MOIV_LINK_DIR} )
LINK_DIRECTORIES( ${QWT_LINK_DIR} )                  

################################################################################

## Used to speedup compilation in mingw environments
IF( DATA_FORMATS_SUBSET )
  ADD_DEFINITIONS( -DMOLEKEL_USE_COMMON_FILE_FORMATS )
ENDIF( DATA_FORMATS_SUBSET )

## Depth peeling support; works with VTK version >= 5.1
SET( ENABLE_DEPTH_PEELING ON CACHE BOOL "Enable depth peeling (VTK > 5.1 only" )
IF( ENABLE_DEPTH_PEELING )
  ADD_DEFINITIONS( -DENABLE_DEPTH_PEELING )	
ENDIF( ENABLE_DEPTH_PEELING )


#### MOC headers - read from external file####
SET( MOC_HEADER_FILES molekel_moc_headers.cmake CACHE PATH "Molekel Qt moc headers" )
INCLUDE( ${MOC_HEADER_FILES} )
QT4_WRAP_CPP( MOC_SRCS ${MOC_HEADERS} )

#### SOURCES - read from external file
SET( MOLEKEL_SRCS molekel_sources.cmake CACHE PATH "Molekel source file list" )
INCLUDE( ${MOLEKEL_SRCS} )


#### COMPILER FLAGS ####
# Avoid warnings about using scanf_s (non portable) instead of scanf; should replace
# with custom built checked alternative to scanf.
IF( MSVC )
  ADD_DEFINITIONS( -D_CRT_SECURE_NO_WARNINGS )
ENDIF(MSVC)  


#### EXECUTABLE ####
# WIN32: required to get rid of the console window, equivalent to MinGW's -mwindows
# MACOSX_BUNDLE: required to create a .app bundle
IF( APPLE )
  SET( MACOSX_BUNDLE_INFO_STRING "Molekel - molecular visualization program" )
  # Change following line to point to actual icns file in bundle.
  SET( MACOSX_BUNDLE_ICON_FILE "Molekel.icns" )
  EXEC_PROGRAM( "mkdir -p ${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_BIN_DIR}/Molekel.app/Contents/Resources" )
  EXEC_PROGRAM( "cp ${CMAKE_SOURCE_DIR}/resources/Molekel.icns ${CMAKE_BINARY_DIR}/${MOLEKEL_DIST_DIR}/${MOLEKEL_BIN_DIR}/Molekel.app/Contents/Resources/Molekel.icns" )
  SET( MACOSX_BUNDLE_GUI_IDENTIFIER "Molekel" ) # <-- ???
  SET( MACOSX_BUNDLE_LONG_VERSION_STRING "Molekel ${MOLEKEL_APP_VERSION_STRING} - Copyright (c) 2006, 2007, 2008, 2009 CSCS" )
  SET( MACOSX_BUNDLE_BUNDLE_NAME "Molekel" )
  SET( MACOSX_BUNDLE_SHORT_VERSION_STRING "${MOLEKEL_VERSION_MAJOR}.${MOLEKEL_VERSION_MINOR}.${MOLEKEL_VERSION_PATCH}" )
  SET( MACOSX_BUNDLE_BUNDLE_VERSION ${MOLEKEL_APP_VERSION_STRING} )
  SET( MACOSX_BUNDLE_COPYRIGHT ${MOLEKEL_COPYRIGHT_INFO} )
ENDIF( APPLE )

#Copy toolbar resources
FILE( MAKE_DIRECTORY "${MOLEKEL_RESOURCES_DIR}/toolbar" )
FILE( GLOB PNG_FILES ${CMAKE_SOURCE_DIR}/resources/toolbar/*.png  )
FOREACH( PNG ${PNG_FILES} )
  CONFIGURE_FILE( ${PNG} ${MOLEKEL_RESOURCES_DIR}/toolbar/ COPYONLY )
ENDFOREACH( PNG )  

#Copy Shaders
FILE( GLOB SHADERS_FILES ${CMAKE_SOURCE_DIR}/../shaders/*.frag
                         ${CMAKE_SOURCE_DIR}/../shaders/*.vert
                         ${CMAKE_SOURCE_DIR}/../shaders/*.params   )
FOREACH( SHADER ${SHADERS_FILES} )
  CONFIGURE_FILE( ${SHADER} ${MOLEKEL_DIST_DIR}/shaders/ COPYONLY )
ENDFOREACH( SHADER )
#EXEC_PROGRAM( "cp ${CMAKE_SOURCE_DIR}/../shaders/*.* ${MOLEKEL_DIST_DIR}/shaders" )


#Executable name
SET( MOLEKEL_EXECUTABLE Molekel )

# Source files
IF( MSVC )
  SET( SOURCE ${SRCS} obms.cpp )
ELSE(MSVC)
  SET( SOURCE ${SRCS} )
ENDIF( MSVC )    

# Executable
ADD_EXECUTABLE( ${MOLEKEL_EXECUTABLE} WIN32 MACOSX_BUNDLE ${SOURCE} )

#### LIBRARIES ####
TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} ${QT_LIBRARIES} )
TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} openbabel )
TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} ${IV_LIB} )
TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} ChemKit2 )
TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE}
                       QVTK
                       vtkHybrid
                       vtkWidgets
                       vtkImaging
                       vtkRendering
                       vtkGraphics
                       vtkIO
                       vtkCommon )
IF( WIN32 )
  TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} qwt5 )
ELSE( WIN32 )
  TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} qwt )
ENDIF( WIN32)
IF( WIN32 )
  TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} glew32 )
ELSE ( WIN32 )
  TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} GLEW )
ENDIF ( WIN32 )

IF( MSVC )
  TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} debug qtmaind optimized qtmain )
ENDIF(MSVC) 


TARGET_LINK_LIBRARIES( ${MOLEKEL_EXECUTABLE} ${OPENGL_LIBRARIES} )

################################################################################
