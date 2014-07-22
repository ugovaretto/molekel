#Checks out the latest Molekel source code and builds a binary distribution. 
svn export http://molekel.cscs.ch/svn/molekel/trunk/build/checkout.sh
sh checkout.sh
#build code in ./svn dir and assign proper version # (maj min patch build)
sh svn/build/build_unix.sh ./svn x86_32 5 4 0 8
