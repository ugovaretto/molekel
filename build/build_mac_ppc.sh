SVN_PATH=$1
MAJ=$2
MIN=$3
PATCH=$4
BUILD=$5
BUILD_TYPE=$6


#Used for debugging only
#Pause()
#{
#    echo
#    echo -n Hit Enter to continue....
#    read
#}



if [ -z $6 ]; then
  BUILD_TYPE="\"\""
fi

#Generate version info file required by CMake
echo -e '\n' "SET( MOLEKEL_VERSION_MAJOR $MAJ )\n" \
     "SET( MOLEKEL_VERSION_MINOR $MIN )\n" \
     "SET( MOLEKEL_VERSION_PATCH $PATCH )\n" \
     "SET( MOLEKEL_VERSION_BUILD $BUILD )\n" \
     "SET( MOLEKEL_VERSION_TYPE $BUILD_TYPE )\n" > version_info.cmake

#Build Molekel
sh "$SVN_PATH/build/run_mac_ppc_cmake.sh"
#Pause
mv Molekel.app/Contents/Info.plist ./dist/bundle/Molekel.app/Contents/
#Pause
mv Molekel.app/Contents/MacOS ./dist/bundle/Molekel.app/Contents/

#Fix dylib paths
#Pause
sh "$SVN_PATH/install/mac/fixpath_ppc.sh"

#Prepare for packaging
mv ./dist/bundle/Molekel.app ./dist/
rm -rf ./dist/resources
rmdir ./dist/bundle

#Build dmg
sh "$SVN_PATH/install/mac/create_mac_dmg.sh" molekel_MacOS_"$MAJ"_"$MIN"_"$PATCH"_`arch`

# Zip it up: not required since the image is already compressed but needed
# to upload the file to websites not allowing the upload of files with .dmg extension
zip molekel_MacOS_"$MAJ"_"$MIN"_"$PATCH"_`arch`.dmg.zip molekel_MacOS_"$MAJ"_"$MIN"_"$PATCH"_`arch`.dmg
rm molekel_MacOS_"$MAJ"_"$MIN"_"$PATCH"_`arch`.dmg 

########## TEST ###########

#Copy test dir.
cp -R $SVN_PATH/test .

#Make test output dir
mkdir ./test/out

#Make GUI event scripts terminate the application upon completion.
sh ./test/scripts/addexit.sh mac_tiger

#Run all tests.
#TBD









