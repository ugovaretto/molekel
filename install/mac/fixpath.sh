#!/usr/bin/sh
QTPATH=/usr/local/Trolltech/Qt-4.2.2
LIB=/usr/local/lib
MOIV_LIB=/Users/uvaretto/projects/tmp/OpenMOIV.src.1.0.3
GLEW_LIB=/usr/local/lib
QWT_LIB=/usr/local/qwt-5.0.2/lib

mkdir dist/bundle/Molekel.app/Contents/Frameworks

echo copying...

cp -R $QTPATH/lib/QtCore.framework dist/bundle/Molekel.app/Contents/Frameworks/
cp -R $QTPATH/lib/QtGui.framework dist/bundle/Molekel.app/Contents/Frameworks/
cp -R /Library/Frameworks/Inventor.framework dist/bundle/Molekel.app/Contents/Frameworks
cp $LIB/libopenbabel.2.dylib dist/bundle/Molekel.app/Contents/Frameworks
cp $LIB/libinchi.0.dylib dist/bundle/Molekel.app/Contents/Frameworks
cp $MOIV_LIB/libChemKit2.dylib dist/bundle/Molekel.app/Contents/Frameworks
cp $GLEW_LIB/libGLEW.1.5.0.dylib dist/bundle/Molekel.app/Contents/Frameworks
cp $QWT_LIB/libqwt.5.dylib dist/bundle/Molekel.app/Contents/Frameworks

#
echo "calling otool"
install_name_tool -id @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore dist/bundle/Molekel.app/Contents/Frameworks/QtCore.framework/Versions/4/QtCore
install_name_tool -id @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui dist/bundle/Molekel.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -id @executable_path/../Frameworks/Inventor.framework/Versions/C/Inventor dist/bundle/Molekel.app/Contents/Frameworks/Inventor.framework/Versions/C/Inventor
install_name_tool -id @executable_path/../Frameworks/libinchi.0.dylib dist/bundle/Molekel.app/Contents/Frameworks/libinchi.0.dylib
install_name_tool -id @executable_path/../Frameworks/libopenbabel.2.dylib dist/bundle/Molekel.app/Contents/Frameworks/libopenbabel.2.dylib
install_name_tool -id @executable_path/../Frameworks/libChemKit2.dylib dist/bundle/Molekel.app/Contents/Frameworks/libChemKit2.dylib
install_name_tool -id @executable_path/../Frameworks/libGLEW.1.5.0.dylib dist/bundle/Molekel.app/Contents/Frameworks/libGLEW.1.5.0.dylib
install_name_tool -id @executable_path/../Frameworks/libqwt.5.dylib dist/bundle/Molekel.app/Contents/Frameworks/libqwt.5.dylib

#
install_name_tool -change $QTPATH/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore dist/bundle/Molekel.app/Contents/MacOS/Molekel
install_name_tool -change $QTPATH/lib/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui dist/bundle/Molekel.app/Contents/MacOS/Molekel
install_name_tool -change Inventor.framework/Versions/C/Inventor @executable_path/../Frameworks/Inventor.framework/Versions/C/Inventor dist/bundle/Molekel.app/Contents/MacOS/Molekel
install_name_tool -change $QTPATH/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore dist/bundle/Molekel.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui
install_name_tool -change libqwt.5.dylib @executable_path/../Frameworks/libqwt.5.dylib dist/bundle/Molekel.app/Contents/MacOS/Molekel

#
install_name_tool -change $LIB/libopenbabel.2.dylib @executable_path/../Frameworks/libopenbabel.2.dylib dist/bundle/Molekel.app/Contents/MacOS/Molekel
install_name_tool -change $MOIV_LIB/libChemKit2.dylib @executable_path/../Frameworks/libChemKit2.dylib dist/bundle/Molekel.app/Contents/MacOS/Molekel
install_name_tool -change $GLEW_LIB/libGLEW.1.5.0.dylib @executable_path/../Frameworks/libGLEW.1.5.0.dylib dist/bundle/Molekel.app/Contents/MacOS/Molekel

#
install_name_tool -change Inventor.framework/Versions/C/Inventor @executable_path/../Frameworks/Inventor.framework/Versions/C/Inventor dist/bundle/Molekel.app/Contents/Frameworks/libChemKit2.dylib
install_name_tool -change $LIB/libinchi.0.dylib @executable_path/../Frameworks/libinchi.0.dylib dist/bundle/Molekel.app/Contents/Frameworks/libopenbabel.2.dylib
install_name_tool -change $QTPATH/lib/QtCore.framework/Versions/4/QtCore @executable_path/../Frameworks/QtCore.framework/Versions/4/QtCore dist/bundle/Molekel.app/Contents/Frameworks/libqwt.5.dylib
install_name_tool -change $QTPATH/lib/QtGui.framework/Versions/4/QtGui @executable_path/../Frameworks/QtGui.framework/Versions/4/QtGui dist/bundle/Molekel.app/Contents/Frameworks/libqwt.5.dylib

#
echo "removing debug libs"
rm dist/bundle/Molekel.app/Contents/Frameworks/QtCore.framework/QtCore_debug.prl
rm dist/bundle/Molekel.app/Contents/Frameworks/QtCore.framework/QtCore_debug
rm dist/bundle/Molekel.app/Contents/Frameworks/QtCore.framework/Versions/4/QtCore_debug
rm dist/bundle/Molekel.app/Contents/Frameworks/QtGui.framework/QtGui_debug.prl
rm dist/bundle/Molekel.app/Contents/Frameworks/QtGui.framework/QtGui_debug
rm dist/bundle/Molekel.app/Contents/Frameworks/QtGui.framework/Versions/4/QtGui_debug




