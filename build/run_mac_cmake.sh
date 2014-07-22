#!/bin/sh
#touch version_info.cmake

#Set svn root
MSVN=./svn

#Set include, library paths and any cmake variable
BD=/usr/local
#QTPATH=/usr/local/Trolltech/Qt-4.4.3
#QWT_PATH=/usr/local/qwt-5.1.1
# Go back to Qt 4.2.2 to ease deployment, since 4.4. requires a lot of additional work
# to deal with plugins
QTPATH=/usr/local/Trolltech/Qt-4.2.2
QWT_PATH=/usr/local/qwt-5.0.2
cmake $MSVN/src \
-DCMAKE_BUILD_TYPE:string=Release \
-DIV_INCLUDE_DIR:path=$BD/include \
-DIV_LINK_DIR:path=$BD/lib \
-DMOIV_INCLUDE_DIR:path=$BD/include \
-DMOIV_LINK_DIR:path=$BD/lib \
-DOPENBABEL_INCLUDE_DIR:path=$BD/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=$BD/lib \
-DQT_QMAKE_EXECUTABLE:path=$QTPATH/bin/qmake \
-DVTK_DIR:path=$BD/lib/vtk-5.1 \
-DGLEW_INCLUDE_DIR:path=/usr/local/include \
-DGLEW_LINK_DIR:path=/usr/local/lib \
-DQWT_INCLUDE_DIR:path=$QWT_PATH/include \
-DQWT_LINK_DIR:path=$QWT_PATH/lib \
-DENABLE_DEPTH_PEELING:int=1
 

#Make
make

#Copy data
cp -R $MSVN/data dist/
rm -Rf dist/data/issues

#Copy shaders
cp -R $MSVN/shaders dist/

#Copy events - uncomment when testing
#cp -R $MSVN/events dist/
#rm -Rf dist/events/.svn

#Copy resources
cp -R dist/resources/toolbar dist/bundle/Molekel.app/Contents/Resources
