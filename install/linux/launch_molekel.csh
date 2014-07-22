#!/bin/csh
set DLLPATH = `pwd`/lib   	
if ( ${LD_LIBRARY_PATH} == '' ) then
  echo "LD_LIBRARY_PATH is empty"
  setenv LD_LIBRARY_PATH ${DLLPATH}
  echo "Set LD_LIBRARY_PATH to ${LD_LIBRARY_PATH}"
else
  set FOUND = `expr match "${LD_LIBRARY_PATH}" "${DLLPATH}"`
  set DLLPATH_LENGTH = `expr length ${DLLPATH}`
  if ( ${FOUND} !=  ${DLLPATH_LENGTH} ) then
    echo "LD_LIBRARY_PATH does not contain Molekel's lib directory"
    setenv LD_LIBRARY_PATH "${DLLPATH}:${LD_LIBRARY_PATH}"
    echo "Set LD_LIBRARY_PATH to ${LD_LIBRARY_PATH}"
  endif    	
endif
bin/Molekel &
