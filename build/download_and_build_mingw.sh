# 1) Downloads export script from svn repository.
# 2) Checks out code using downloaded script.
# 3) Downloads dependencies using wget; wget has to be in path.
#    Note: a version of wget that works with mingw is included in the bin
#    dir of the dependencies archive.
# 4) Uncompress dependencies
# 5) Build redistributable archive
# All operations logged to 'build.log'
LOG_FILE=build.log
svn export http://molekel.cscs.ch/svn/molekel/trunk/build/checkout.sh | tee $LOG_FILE
sh ./checkout.sh | tee -a $LOG_FILE
wget ftp://ftp.cscs.ch/out/molekel/molekel_5.3/dep/molekel_5_3_mingw_deps.tar.gz | tee -a $LOG_FILE
tar xzf ./molekel_5_3_mingw_deps.tar.gz | tee -a $LOG_FILE
sh ./svn/build/build_mingw.sh ./svn 5 4 0 8 `date +%Y%m%d` | tee -a $LOG_FILE
