REM Builds Molekel executable inside dist directory and copies
REM all the required dlls and data into distribution directory

REM Set source root dir default is <current path>\svn
SET MSVN=%CD%\svn

REM Set QT dir
SET QT_DIR=C:\Qt\4.4

REM Set library and include paths and invoke cmake
REM The following variables need to be set:
REM CMAKE_BUILD_TYPE:string=Release OR Debug - build type
REM IV_INCLUDE_DIR:path= OpenInventor\Coin include dir
REM IV_LINK_DIR:path= OpenInventor\Coin library dir
REM MOIV_INCLUDE_DIR:path= OpenMOIV include dir 
REM MOIV_LINK_DIR:path= OpenMOIV library dir
REM OPENBABEL_INCLUDE_DIR:path= OpenBabel include dir
REM OPENBABEL_LINK_DIR:path= OpenBabel library dir
REM QT_QMAKE_EXECUTABLE:path= path of qmake executable
REM VTK_DIR:path=  VTK lib path e.g. $BD\lib\vtk-5.1
REM GLEW_INCLUDE_DIR:path= GLEW include dir
REM GLEW_LINK_DIR:path= GLEW library dir
REM QWT_INCLUDE_DIR:path= QWT include dir
REM QWT_LINK_DIR:path= QWT library dir
REM ENABLE_DEPTH_PEELING:int=1 OR 0 enable\disable depth peeling; enabled only if VTK version >= 5.1

SET BD=%CD%\deps

SET CMAKE_EXECUTABLE="C:\Program Files\CMake 2.6\bin"\cmake.exe

%CMAKE_EXECUTABLE% -G"Visual Studio 9 2008" %MSVN%\src -DCMAKE_BUILD_TYPE:string=Release -DIV_INCLUDE_DIR:path=%BD%\include -DIV_LINK_DIR:path=%BD%\lib -DMOIV_INCLUDE_DIR:path=%BD%\include -DMOIV_LINK_DIR:path=%BD%\lib -DOPENBABEL_INCLUDE_DIR:path=%BD%\include -DOPENBABEL_LINK_DIR:path=%BD%\lib -DQT_QMAKE_EXECUTABLE:path=%QT_DIR%\bin\qmake -DVTK_DIR:path=%BD%\lib\vtk-5.2_devenv -DGLEW_INCLUDE_DIR:path=%BD%\include -DGLEW_LINK_DIR:path=%BD%\lib -DQWT_INCLUDE_DIR:path=%BD%\include -DQWT_LINK_DIR:path=%BD%\lib -DENABLE_DEPTH_PEELING:int=1


REM Make
devenv Molekel.sln /build Release /project "Molekel.vcproj" /projectconfig Release 

move Release\Molekel.exe dist\bin

REM Set dll directory and copy all dlls to bin dir
SET DLLDIR=%BD%\bin
copy %DLLDIR%\*.dll dist\bin

REM Copy data
xcopy /s /i %MSVN%\data dist\data
rd /s /q dist\data\issues

REM Copy events - uncomment when making a test build
REM xcopy /s /i $MSVN\events dist\events

REM Convert end of line to win format - requires unix2dos in path
REM unix2dos dist\*.txt
REM unix2dos dist\data\*.*
REM unix2dos dist\shaders\*.*
