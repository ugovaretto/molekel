#!/bin/bash

#Builds Molekel and generates a tar.gz package containing binary, libraries and data
#To simply build the executable use the run_unix_cmake.sh script.
#Invoke like this inside build directory:
# sh build_unix.sh <path to Molekel source distribution> ARCHITECTURE MAJOR MINOR PATCH_LEVEL BUILD [BUILD TYPE]

SVN_PATH=$1 #path to svn source tree
ARCH=$2     #architecture e.g. x86_32
MAJ=$3      #major version number
MIN=$4      #minor version number
PATCH=$5    #patch # 
BUILD=$6    #build #
BUILD_TYPE=$7 #build type e.g. development, release, test...

#set build type to "" in case not specified
if [ -z $7 ]; then
  BUILD_TYPE="\"\""
fi

#Generate version info file required by CMake
echo -e '\n' "SET( MOLEKEL_VERSION_MAJOR $MAJ )\n" \
     "SET( MOLEKEL_VERSION_MINOR $MIN )\n" \
     "SET( MOLEKEL_VERSION_PATCH $PATCH )\n" \
     "SET( MOLEKEL_VERSION_BUILD $BUILD )\n" \
     "SET( MOLEKEL_VERSION_TYPE $BUILD_TYPE )\n" > version_info.cmake

#Build Molekel
sh "$SVN_PATH/build/run_unix_cmake.sh"
#Create package
mv dist molekel_"$MAJ"_"$MIN"_"$PATCH"_linux_"$ARCH"
tar czf molekel_"$MAJ"_"$MIN"_"$PATCH"_linux_"$ARCH".tar.gz molekel_"$MAJ"_"$MIN"_"$PATCH"_linux_"$ARCH"

########## TEST ###########

#Copy test dir.
#cp -R $SVN_PATH/test .

#Make test output dir
#mkdir ./test/out

#Make GUI event scripts terminate the application upon completion.
#sh ./test/scripts/addexit.sh unix_kde
#sh ./test/scripts/addexit.sh unix_gnome

#Run all tests.
#TBD

