#Get this script with svn: svn export http://molekel.cscs.ch/svn/molekel/trunk/build/checkout.sh 
SVN_DIR=./svn
mkdir $SVN_DIR
svn export http://molekel.cscs.ch/svn/molekel/trunk/build $SVN_DIR/build
svn export http://molekel.cscs.ch/svn/molekel/trunk/install $SVN_DIR/install
svn export http://molekel.cscs.ch/svn/molekel/trunk/src $SVN_DIR/src
svn export http://molekel.cscs.ch/svn/trunk/data $SVN_DIR/data
svn export http://molekel.cscs.ch/svn/molekel/trunk/events $SVN_DIR/events
svn export http://molekel.cscs.ch/svn/molekel/trunk/shaders $SVN_DIR/shaders

#Testing
svn export http://molekel.cscs.ch/svn/molekel/trunk/test $SVN_DIR/test
