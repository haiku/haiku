#! /bin/sh
# Test of bind_textdomain_codeset.
# Copyright (C) 2001, 2002 Free Software Foundation, Inc.
# This file is part of the GNU C Library.
#

# The GNU C Library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.

# The GNU C Library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.

# You should have received a copy of the GNU Lesser General Public
# License along with the GNU C Library; if not, write to the Free
# Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
# 02111-1307 USA.

common_objpfx=$1
objpfx=$2

LC_ALL=C
export LC_ALL

# Generate the test data.
test -d ${objpfx}domaindir || mkdir ${objpfx}domaindir
# Create the domain directories.
test -d ${objpfx}domaindir/de_DE || mkdir ${objpfx}domaindir/de_DE
test -d ${objpfx}domaindir/de_DE/LC_MESSAGES || mkdir ${objpfx}domaindir/de_DE/LC_MESSAGES
# Populate them.
msgfmt -o ${objpfx}domaindir/de_DE/LC_MESSAGES/codeset.mo tstcodeset.po

GCONV_PATH=${common_objpfx}iconvdata
export GCONV_PATH
LOCPATH=${common_objpfx}localedata
export LOCPATH

${common_objpfx}elf/ld.so --library-path $common_objpfx \
${objpfx}tst-codeset > ${objpfx}tst-codeset.out

exit $?
