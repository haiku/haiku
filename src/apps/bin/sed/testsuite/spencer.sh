#! /bin/sh

sed=${1-sed}

# Each line in spencer.inp is converted to something like
#   echo ... | $sed -re ... | sed -ne "1s/.*/Test #xxx failed!/p",
#
# which uses the -r command line option to sed which changes the
# regex syntax used to that of POSIX egrep.
#
# The first sed is the real test, and the only one using $sed
# instead of the system sed; it runs a script designed so
# that it outputs a line if and only if there is an error:
#   0 = regexp must match, so that the d command executes
#   1 = regexp must not match, so that the d command executes
#   2 = regexp is wrong, so the script should not even run
#
# The second sed prints an error message if it ever receives
# a line from the first one.
#
# The conversion from spencer.inp to a shell script
# is handled by two sed scripts.
#
# sed1 convert each test to two lines:
#  echo ... | sed ... | sed
#  <line number>
#
# That is, sed1 leaves the second sed's command line incomplete:
# sed2 will group two lines and form the shell script.

sed1="
  /^#/d
  s,^0:\(.*\):\(.*\),echo '\2' | $sed -re '/\1/d' 2>\&1 | sed,
  s,^1:\(.*\):\(.*\),echo '\2' | $sed -re '/\1/!d' 2>\&1 | sed,
  s,^2:\(.*\):\(.*\),echo '\2' | $sed -re '/\1/h' 2>/dev/null | sed,
  p
  =
  d"

sed2='
  N
  s,\n\(.*\), -ne "1s/.*/Test #\1 failed!/p",
'

# We use standard sed for this because the test suite is not this
# file, but spencer.inp!
sed "$sed1" - | sed "$sed2"
