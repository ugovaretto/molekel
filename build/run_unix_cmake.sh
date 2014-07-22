#!/bin/sh

####Set svn root dir
MSVN=./svn

#### Requires version_info.cmake

####Set include and libraries path and any cmake related variable
# default library path; make './local' link in build directory to point at e.g. /usr/local 
LIBPATH=`pwd`/local
# Qt
QT_PATH=$LIBPATH/Trolltech/Qt-4.2.2
# Qwt
QWT_PATH=$LIBPATH/qwt-5.0.2
# VTK lib path 
VTK_PATH=$LIBPATH/lib/vtk-5.1
# DLL directory: all files in this directory will be copied into dist/lib
# this is usually a link to where the libraries reside
DLLDIR=molekel_dll

#The following variables need to be set:
#CMAKE_BUILD_TYPE:string=Release OR Debug - build type
#IV_INCLUDE_DIR:path= OpenInventor/Coin include dir
#IV_LINK_DIR:path= OpenInventor/Coin library dir
#MOIV_INCLUDE_DIR:path= OpenMOIV include dir 
#MOIV_LINK_DIR:path= OpenMOIV library dir
#OPENBABEL_INCLUDE_DIR:path= OpenBabel include dir
#OPENBABEL_LINK_DIR:path= OpenBabel library dir
#QT_QMAKE_EXECUTABLE:path= path of qmake executable
#VTK_DIR:path=  VTK lib path e.g. $BD/lib/vtk-5.1
#GLEW_INCLUDE_DIR:path= GLEW include dir
#GLEW_LINK_DIR:path= GLEW library dir
#QWT_INCLUDE_DIR:path= QWT include dir
#QWT_LINK_DIR:path= QWT library dir
#ENABLE_DEPTH_PEELING:int=1 OR 0 enable/disable depth peeling; enabled only if VTK version >= 5.1

####Invoke CMake
cmake svn/src \
-DCMAKE_BUILD_TYPE:string=Release \
-DIV_INCLUDE_DIR:path=$LIBPATH/include \
-DIV_LINK_DIR:path=$LIBPATH/lib \
-DMOIV_INCLUDE_DIR:path=$LIBPATH/include \
-DMOIV_LINK_DIR:path=$LIBPATH/lib \
-DOPENBABEL_INCLUDE_DIR:path=$LIBPATH/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=$LIBPATH/lib \
-DQT_QMAKE_EXECUTABLE:path=$QT_PATH/bin/qmake \
-DVTK_DIR:path=$VTK_PATH \
-DGLEW_INCLUDE_DIR:path=$LIBPATH/include \
-DGLEW_LINK_DIR:path=$LIBPATH/lib \
-DQWT_INCLUDE_DIR:path=$QWT_PATH/include \
-DQWT_LINK_DIR:path=$QWT_PATH/lib \
-DENABLE_DEPTH_PEELING:int=1

####Make
make
cp Molekel dist/bin/

####Copy libs
mkdir dist/lib
cp $DLLDIR/*.* dist/lib

####Copy shaders
cp -R $MSVN/shaders dist/
rm -Rf dist/shaders/.svn

####Copy data
cp -R $MSVN/data dist/
rm -Rf dist/data/issues
rm -Rf dist/data/.svn

####Copy events (for ui testing) 
#cp -R $MSVN/events dist/
#rm -Rf dist/events/.svn

####Copy launch scripts
cp $MSVN/install/linux/INSTALL dist/
cp $MSVN/install/linux/start_molekel.sh.sample dist/
cp $MSVN/install/linux/launch_molekel.* dist/



