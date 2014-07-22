#Script to create a web page with test output and reference output.
#Usage: sh generate_report.sh <reference path> <test output path> [header]
#NOTE: will not add images in <test path> subdirectories (i.e. animations)

REFERENCE_PATH=$1
TEST_PATH=$2
HEADER=$3

#### HEADER
function add_header
{
  if [ -n HEADER ]; then
    echo "<center><h1>$HEADER</h1></center>"
  fi
}

#### DATE, OUTPUT PATH, REFERENCE PATH
function print_top_table
{
  echo -e "<table>\n"
  echo -e "<tr><td>Date:</td><td>`date`</td></tr>\n"
  echo -e "<tr><td>Path:</td><td><i>`pwd`</i></td></tr>\n"
  echo -e "<tr><td>Test output:</td><td><i>$TEST_PATH</i></td></tr>\n"
  echo -e "<tr><td>Reference output:</td><td><i>$REFERENCE_PATH</i></td></tr>\n"
  echo -e "</table>\n"
}

#### TWO COLUMN TABLE (output, reference output)
function display_test_output
{
  echo -e "<table>\n"
  echo -e "<tr><td><h2>Output</h2></td><td><h2>Reference</h2></td></tr>\n"
  for f in $TEST_PATH/*.png
  do
    T=${f##"$TEST_PATH/"}
    R="$REFERENCE_PATH/$T"
    TESTNAME=${T%".png"}
    echo -e "<tr><td><b><i>$TESTNAME</i></b></td></tr>\n"
    echo -e "<tr><td><img src=\"$f\" alt=\"$T\" width=\"640\" height=\"480\" /></td><td><img src=\"$R\" width=\"640\" height=\"480\"/></td></tr>\n"
  done
  echo -e "</table>\n"
}
#### ENTRY POINT: create web page and render header, info and images
cat <<- _EOF_
  <!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
  <html xmlns="http://www.w3.org/1999/xhtml">
  <head>
    <title>Molekel test output</title>
  </head>
  <body>
    $(add_header)
    <br/>
    $(print_top_table)
    <br/>  
    $(display_test_output)
    <br/>
    <hr/>
    <a href="http://bioinformatics.org/molekel><i>Molekel home page</i></a>
    <br/>
  </body>
  </html>
_EOF_