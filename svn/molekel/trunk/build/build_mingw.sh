#Builds Molekel and generates windows setup executable then execute a set of automated tests.
#To simply build Molekel without packaging and testing use the run_mingw_cmake.sh script.
#Invoke like this inside build directory:
# sh build_mingw.sh <path to Molekel source distribution> MAJOR MINOR PATCH_LEVEL BUILD [BUILT TYPE]
#
# e.g. to build a development version of Molekel 5.1 after having checked out the Molekel SVN tree
# to /c/projects/molelekel/svn/branches/5.1 run:
# sh build_mingw.sh /c/projects/molelekel/svn/branches/5.1 5 1 0 0 Development


########## BUILD ###########

SVN_PATH=$1   #path to svn source tree
MAJ=$2        #major version number
MIN=$3        #minor version number 
PATCH=$4      #patch # 
BUILD=$5      #build #
BUILD_TYPE=$6 #build type e.g. development, release, test...

#set build type to "" in case not specified
if [ -z $6 ]; then
  BUILD_TYPE="\"\""
fi

#Copy script to run CMake to current directory
cp "$SVN_PATH/build/run_mingw_cmake.sh" .

#Generate version info file required by CMake
echo -e '\n' "SET( MOLEKEL_VERSION_MAJOR $MAJ )\n" \
     "SET( MOLEKEL_VERSION_MINOR $MIN )\n" \
     "SET( MOLEKEL_VERSION_PATCH $PATCH )\n" \
     "SET( MOLEKEL_VERSION_BUILD $BUILD )\n" \
     "SET( MOLEKEL_VERSION_TYPE $BUILD_TYPE )\n" > version_info.cmake

#In case MS VC++ is installed reset include and lib variables
export INCLUDE=
export LIB=

#Build Molekel
sh run_mingw_cmake.sh

#Generate setup file
/c/Program\ Files/NSIS/makensis.exe setup.nsi

#Rename generated setup file
mv ./setup.exe molekel_"$MAJ"_"$MIN"_"$PATCH"_win.exe

########## TEST ###########

#Copy test dir.
cp -R $SVN_PATH/test .

#Make test output dir
mkdir ./test/out

#Fix EOLs in test/data
unix2dos ./test/data/*.* 

##### Temporarily disabled, need to update tests for new UI #### 
#Make GUI event scripts terminate the application upon completion.
sh ./test/scripts/addexit.sh win_xp

#Run all tests. 
sh ./test/scripts/all_tests.sh win_xp

#Generate report - will not add images from animations
#RHEADER="Molekel $MAJ.$MIN.$PATCH.$BUILD.$BUILD_TYPE test"
sh ./test/scripts/generate_report.sh ./test/test_cases/reference_output ./test/out "$RHEADER" > tests.html

#Display results in browser
/c/Program\ Files/Mozilla\ Firefox/firefox.exe tests.html






