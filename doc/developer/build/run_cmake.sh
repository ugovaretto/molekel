#!/bin/sh
touch version_info.cmake
export BD=/home/uvaretto/local/devlib/release
cmake /home/uvaretto/projects/molekel/svn/branches/5.1/src \
-DIV_INCLUDE_DIR:path=$BD/include \
-DIV_LINK_DIR:path=$BD/lib \
-DMOIV_INCLUDE_DIR:path=$BD/include \
-DMOIV_LINK_DIR:path=$BD/lib \
-DOPENBABEL_INCLUDE_DIR:path=$BD/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=$BD/lib \
-DQT_QMAKE_EXECUTABLE:path=/home/uvaretto/local/devlib/src/qt-x11-opensource-src-4.2.2/bin/qmake \
-DVTK_DIR:path=$BD/lib/vtk-5.0
make
export MSVN=/home/uvaretto/projects/molekel/svn/branches/5.1
cp -R $MSVN/data dist/
rm -Rf dist/data/.svn
cp -R $MSVN/data/shaders dist/
rm -Rf dist/shaders/.svn
cp $MSVN/install/linux/INSTALL dist/
cp $MSVN/install/linux/start_molekel.sh.sample dist/
