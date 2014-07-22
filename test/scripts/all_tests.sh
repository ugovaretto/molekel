PLATFORM=$1

for f in ./test/test_cases/$PLATFORM/*.events
do
  sh "./test/scripts/launch_$PLATFORM.sh" -events $f 3 0 1.0
done
