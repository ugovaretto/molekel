echo off
REM Builds Molekel and generates windows setup executable then execute a set of automated tests.
REM To simply build Molekel without packaging and testing use the run_mingw_cmake.sh script.
REM Invoke like this inside build directory:
REM sh build_mingw.sh <path to Molekel source distribution> MAJOR MINOR PATCH_LEVEL BUILD [BUILT TYPE]
REM
REM e.g. to build a development version of Molekel 5.1 after having checked out the Molekel SVN tree
REM to /c/projects/molelekel/svn/branches/5.1 run:
REM sh build_mingw.sh /c/projects/molelekel/svn/branches/5.1 5 1 0 0 Development


REM ########## BUILD ###########

REM path to svn source tree
SET SVN_PATH=%1

REM major version number
SET MAJ=%2

REM minor version number 
SET MIN=%3

REM patch 
SET PATCH=%4

REM build #
SET BUILD=%5

REM build type: release, date, debug...
SET BUILD_TYPE=%6

REM Copy script to run CMake to current directory
echo on
copy %SVN_PATH%\build\run_nmake_cmake.bat .

REM Generate version info file required by CMake
echo SET( MOLEKEL_VERSION_MAJOR %MAJ% ) > version_info.cmake
more %SVN_PATH%\build\win_eol.txt >> version_info.cmake
echo SET( MOLEKEL_VERSION_MINOR %MIN% ) >> version_info.cmake
more %SVN_PATH%\build\win_eol.txt >> version_info.cmake
echo SET( MOLEKEL_VERSION_PATCH %PATCH% ) >> version_info.cmake
more %SVN_PATH%\build\win_eol.txt >> version_info.cmake
echo SET( MOLEKEL_VERSION_BUILD %BUILD% ) >> version_info.cmake
more %SVN_PATH%\build\win_eol.txt >> version_info.cmake
echo SET( MOLEKEL_VERSION_TYPE %BUILD_TYPE% ) >> version_info.cmake
more %SVN_PATH%\build\win_eol.txt >> version_info.cmake


REM Build Molekel
call run_nmake_cmake.bat

REM Generate setup file
SET makensis="C:\Program Files\NSIS\makensis.exe"
copy %SVN_PATH%\install\win\setup.nsi .
copy %SVN_PATH%\src\resources\Molekel.ico .
%makensis% setup.nsi

REM Rename generated setup file
rename setup.exe molekel_%MAJ%_%MIN%_%PATCH%_win.exe

REM ########## TEST ###########

REM #Copy test dir.
REM cp -R $SVN_PATH/test .

REM #Make test output dir
REM mkdir ./test/out

REM #Fix EOLs in test/data
REM unix2dos ./test/data/*.* 

REM ##### Temporarily disabled, need to update tests for new UI #### 
REM #Make GUI event scripts terminate the application upon completion.
REM sh ./test/scripts/addexit.sh win_xp

REM #Run all tests. 
REM sh ./test/scripts/all_tests.sh win_xp

REM #Generate report - will not add images from animations
REM #RHEADER="Molekel $MAJ.$MIN.$PATCH.$BUILD.$BUILD_TYPE test"
REM sh ./test/scripts/generate_report.sh ./test/test_cases/reference_output ./test/out "$RHEADER" > tests.html

REM #Display results in browser
REM /c/Program\ Files/Mozilla\ Firefox/firefox.exe tests.html






