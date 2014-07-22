#Builds Molekel executable inside dist directory and copies
#all the required dlls and data into distribution directory

#DO export INCLUDE=  AND export LIB= BEFORE RUNNING SCRIPT

#Set source root dir default is <current path>/svn
MSVN=`pwd`/svn

#Set QT dir
QT_DIR=/c/Qt/4.2.2

#Set library and include paths and invoke cmake
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

BD=`pwd`/deps

cmake -G"MSYS Makefiles" $MSVN/src \
-DCMAKE_BUILD_TYPE:string=Release \
-DIV_INCLUDE_DIR:path=$BD/include \
-DIV_LINK_DIR:path=$BD/lib \
-DMOIV_INCLUDE_DIR:path=$BD/include \
-DMOIV_LINK_DIR:path=$BD/lib \
-DOPENBABEL_INCLUDE_DIR:path=$BD/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=$BD/lib \
-DQT_QMAKE_EXECUTABLE:path=$QT_DIR/bin/qmake \
-DVTK_DIR:path=$BD/lib/vtk-5.1 \
-DGLEW_INCLUDE_DIR:path=$BD/include \
-DGLEW_LINK_DIR:path=$BD/lib \
-DQWT_INCLUDE_DIR:path=$BD/include \
-DQWT_LINK_DIR:path=$BD/lib \
-DENABLE_DEPTH_PEELING:int=1


#Make
make

#Set dll directory and copy all dlls to bin dir
DLLDIR=$BD/bin
cp $DLLDIR/*.dll dist/bin

#Copy data
cp -R $MSVN/data dist/
rm -Rf dist/data/.svn
rm -Rf dist/data/issues

#Copy shaders
cp -R $MSVN/shaders dist/
rm -Rf dist/shaders/.svn

#Copy events - uncomment when making a test build
#cp -R $MSVN/events dist/
#rm -Rf dist/events/.svn

#Convert end of line to win format - requires unix2dos in path
unix2dos dist/*.txt
unix2dos dist/data/*.*
unix2dos dist/shaders/*.*
