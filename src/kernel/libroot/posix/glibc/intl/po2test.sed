# po2test.sed - Convert Uniforum style .po file to C code for testing.
# Copyright (C) 2000 Free Software Foundation, Inc.
# Ulrich Drepper <drepper@cygnus.com>, 2000.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
#
#
# We copy the original message as a comment into the .msg file.  But enclose
# them with INPUT ( ).
#
/^msgid/ {
  s/msgid[ 	]*"\(.*\)"/INPUT ("\1")/
# Clear flag from last substitution.
  tb
# Append the next line.
  :b
  N
# Look whether second part is a continuation line.
  s/\(.*\)")\(\n\)"\(.*\)"/\1\\\2\3")/
# Yes, then branch.
  ta
  P
  D
# Note that `D' includes a jump to the start!!
# We found a continuation line.  But before printing insert '\'.
  :a
  s/\(.*\)")\(\n.*\)/\1\\\2/
  P
# We cannot use the sed command `D' here
  s/.*\n\(.*\)/\1/
  tb
}
#
# Copy the translations as well and enclose them with OUTPUT ( ).
#
/^msgstr/ {
  s/msgstr[ 	]*"\(.*\)"/OUTPUT ("\1")/
# Clear flag from last substitution.
  tb
# Append the next line.
  :b
  N
# Look whether second part is a continuation line.
  s/\(.*\)")\(\n\)"\(.*\)"/\1\\\2\3")/
# Yes, then branch.
  ta
  P
  D
# Note that `D' includes a jump to the start!!
# We found a continuation line.  But before printing insert '\'.
  :a
  s/\(.*\)")\(\n.*\)/\1\\\2/
  P
# We cannot use the sed command `D' here
  s/.*\n\(.*\)/\1/
  tb
}
d
