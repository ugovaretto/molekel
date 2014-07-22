REM Get this script with svn: svn export http://molekel.cscs.ch/svn/molekel/trunk/build/checkout.sh 
SET SVN_DIR=svn
md %SVN_DIR%
SET svn=\cygwin\bin\svn
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/build %SVN_DIR%\build
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/src %SVN_DIR%\src
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/data %SVN_DIR%\data
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/events %SVN_DIR%\events
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/shaders %SVN_DIR%\shaders
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/install %SVN_DIR%\install

REM Testing
%svn% export http://molekel.cscs.ch/svn/molekel/trunk/test %SVN_DIR%\test
