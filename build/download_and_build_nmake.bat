REM 1) Downloads export script from svn repository.
REM 2) Checks out code using downloaded script.
REM 3) Downloads dependencies using wget; wget has to be in path.
REM    Note: a version of wget that works with mingw is included in the bin
REM    dir of the dependencies archive.
REM 4) Uncompress dependencies
REM 5) Build redistributable archive
REM All operations logged to 'build.log'

SET LOG_FILE=build.log
SET svn=\cygwin\bin\svn.exe
SET wget=\cygwin\bin\wget.exe
SET unzip="C:\Program Files\7-Zip"\7z.exe x

%svn% export http://molekel.cscs.ch/svn/molekel/trunk/build/checkout.bat >> %LOG_FILE%
call checkout.bat >> %LOG_FILE%
%wget% ftp://ftp.cscs.ch/out/molekel/molekel_5.3.1/dep/molekel_5_3_1_vc_deps.zip >> %LOG_FILE%
%unzip% ./molekel_5_3_1_vc_deps.zip >> %LOG_FILE%
move deps\lib\vtk-5.2_nmake deps\lib\vtk-5.2
svn\build\build_nmake.bat %CD%\svn 5 4 0 8 "%DATE%" >> %LOG_FILE%
