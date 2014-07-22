#!/bin/sh
DLLPATH=`pwd`/lib   	
if [ -z $LD_LIBRARY_PATH ]; then
  echo "LD_LIBRARY_PATH is empty"
  export LD_LIBRARY_PATH=$DLLPATH
  echo "Set LD_LIBRARY_PATH to $LD_LIBRARY_PATH"
else
  FOUND=`expr match "$LD_LIBRARY_PATH" "$DLLPATH"`
  DLLPATH_LENGTH=`expr length $DLLPATH`
  if [ "$FOUND" != "$DLLPATH_LENGTH" ]; then
    echo "LD_LIBRARY_PATH does not contain Molekel's lib directory"
    export LD_LIBRARY_PATH="$DLLPATH:$LD_LIBRARY_PATH"
    echo "Set LD_LIBRARY_PATH to $LD_LIBRARY_PATH"
  fi    	
fi
bin/Molekel &
