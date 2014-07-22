#use like this: launch -events <file> <initial delay=3> <min delay=0> <time scaling=1.0>

#WIN/LINUX
MOLEKEL=./dist/bin/Molekel

#MAC
#MOLEKEL=./dist/bundle/Molekel.app/Contents/MacOS/Molekel

#MSMS executable
MSMS=~/local/bin/msms2.6.1

#PLATFORM
#PLATFORM=win_xp
#PLATFORM=mac_tiger
PLATFORM=unix_kde
#PLATFORM=unix_gnome

$MOLEKEL -settings io/indatadir ./test/data \
      io/ineventsdir ./test/test_cases/$PLATFORM \
      io/outdatadir ./test/out \
      io/outsnapshotsdatadir ./test/out \
      io/shaders ./test/data/shaders \
      msmsexecutable $MSMS $1 $2 $3 $4 $5
