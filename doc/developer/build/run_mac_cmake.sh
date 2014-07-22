#!/bin/sh
touch version_info.cmake
export BD=/usr/local
cmake /Users/uvaretto/projects/molekel/branches/shaders/src \
-DIV_INCLUDE_DIR:path=$BD/include \
-DIV_LINK_DIR:path=$BD/lib \
-DMOIV_INCLUDE_DIR:path=$BD/include \
-DMOIV_LINK_DIR:path=$BD/lib \
-DOPENBABEL_INCLUDE_DIR:path=/opt/local/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=/opt/local/lib \
-DQT_QMAKE_EXECUTABLE:path=/usr/local/Trolltech/Qt-4.2.2/bin/qmake \
-DVTK_DIR:path=$BD/lib/vtk-5.0 \
-DGLEW_INCLUDE_DIR:path=/opt/local/include \
-DGLEW_LINK_DIR:path=/opt/local/lib
make
export MSVN=/Users/uvaretto/projects/molekel/branches/5.1
cp -R $MSVN/data dist/
rm -Rf dist/data/.svn
cp -R dist/resources/toolbar dist/bundle/Molekel.app/Contents/Resources
