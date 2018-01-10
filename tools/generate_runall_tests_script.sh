#!/bin/bash
# Generates a script to run all testsuites
# Outputs script to stdout - gets used by Makefile.edk to create tests/run.nsh
#
# \author Richard Nusser
# \copyright 2017-2018 Richard Nusser
# \license GPLv3 (see http://www.gnu.org/licenses/)
# \link https://github.com/rinusser/UEFIStarter
#

dir=$1
[ "$dir" == "" ] && dir=tests/suites
suites=`\grep -hE "^[[:space:]]*BASE_NAME[[:space:]=]*" $dir/*/*.inf | sed 's/^[^=]*=[[:space:]*]//'`
scriptfile=/dev/stdout

echo "@echo -off" > $scriptfile

#run self test suite first to make sure test framework actually works as expected and so (usually useless) results are the first thing to go off-screen
for suite in $suites; do
  [ "$suite" == "testself" ] && echo $suite -skip runner >> $scriptfile -verbosity 3
done

#run other test suites
for suite in $suites; do
  [ "$suite" == "testself" ] || echo $suite >> $scriptfile -verbosity 3
done
