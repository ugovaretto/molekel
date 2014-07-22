#!/bin/sh
#touch version_info.cmake
# DO export INCLUDE=  AND export LIB= BEFORE RUNNING SCRIPT
export BD=/c/mingw/devlib/release
cmake -G"MSYS Makefiles" /c/projects/molekel/svn/branches/5.1/src \
-DCMAKE_BUILD_TYPE:string=Release \
-DIV_INCLUDE_DIR:path=$BD/include \
-DIV_LINK_DIR:path=$BD/lib \
-DMOIV_INCLUDE_DIR:path=$BD/include \
-DMOIV_LINK_DIR:path=$BD/lib \
-DOPENBABEL_INCLUDE_DIR:path=$BD/include/openbabel-2.0 \
-DOPENBABEL_LINK_DIR:path=$BD/lib \
-DQT_QMAKE_EXECUTABLE:path=/c/Qt/4.2.2/bin/qmake \
-DVTK_DIR:path=$BD/lib/vtk-5.0
make
export MSVN=/c/projects/molekel/svn/branches/5.1
cp -R $MSVN/data dist/
rm -Rf dist/data/.svn
/c/cygwin/bin/unix2dos dist/*.txt
/c/cygwin/bin/unix2dos dist/data/*.*
#sh ./nsisetup.sh

