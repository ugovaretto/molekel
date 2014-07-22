#Adds exit events to event files.

PLATFORM=$1

INPATH=./svn/test/test_cases/$PLATFORM
OUTPATH=./test/test_cases/$PLATFORM

for f in $INPATH/*.events
do
  O=${f##"$INPATH/"}
  echo $O
  cat $f > "$OUTPATH/$O"; cat "$INPATH/exit.events" >> "$OUTPATH/$O"
done
