#!/bin/bash
# Generates test suite runners, one for each test suite
#
# \TODO don't replace existing file if nothing changed, otherwise "make" will rebuild test suite every time
#
# \author Richard Nusser
# \copyright 2017-2018 Richard Nusser
# \license GPLv3 (see http://www.gnu.org/licenses/)
# \link https://github.com/rinusser/UEFIStarter
#

suitesdir=UEFIStarter/tests/suites/

generate() {
  testsdir=$1
  if [ ! -d "$testsdir" ]; then
    echo "internal error: $testsdir doesn't exist"
    exit
  fi

  mkdir -p $testsdir/generated
  runner=$testsdir/generated/runner.c
  echo generating $runner
  funcs=`\grep -hoE "BOOLEAN run_.*_tests\(\)" $testsdir/*.c | colrm 1 7 | tr '()' ' '`
  now=`date +"%Y-%m-%d %H:%M:%S %Z (UTC%:::z)"`

  echo "//DO NOT EDIT" > $runner
  echo "//This file was generated automatically by $0 at $now" >> $runner
  echo "" >> $runner

  echo "#include <UEFIStarter/tests/tests.h>" >> $runner
  echo "" >> $runner

  for func in $funcs; do
    echo "BOOLEAN $func();" >> $runner
  done

  echo "" >> $runner

  echo -e "void run_tests()\n{" >> $runner
  for func in $funcs; do
    echo "  run_group($func);" >> $runner
  done
  echo "}" >> $runner
}

for dir in `find $suitesdir -maxdepth 1 -mindepth 1 -type d `; do
  generate $dir
done
