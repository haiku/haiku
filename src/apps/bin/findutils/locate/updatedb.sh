#!/bin/sh
# updatedb -- build a locate pathname database
# Copyright (C) 1994 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2, or (at your option)
# any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

# csh original by James Woods; sh conversion by David MacKenzie.

usage="\
Usage: updatedb [--localpaths='dir1 dir2...'] [--netpaths='dir1 dir2...']
       [--prunepaths='dir1 dir2...'] [--output=dbfile] [--netuser=user]
       [--old-format] [--version] [--help]"

old=no
for arg
do
  opt=`echo $arg|sed 's/^\([^=]*\).*/\1/'`
  val=`echo $arg|sed 's/^[^=]*=\(.*\)/\1/'`
  case "$opt" in
    --localpaths) SEARCHPATHS="$val" ;;
    --netpaths) NETPATHS="$val" ;;
    --prunepaths) PRUNEPATHS="$val" ;;
    --output) LOCATE_DB="$val" ;;
    --netuser) NETUSER="$val" ;;
    --old-format) old=yes ;;
    --version) echo "GNU updatedb version @version@"; exit 0 ;;
    --help) echo "$usage"; exit 0 ;;
    *) echo "updatedb: invalid option $opt
$usage" >&2
       exit 1 ;;
  esac
done

# You can set these in the environment, or use command-line options,
# to override their defaults:

# Non-network directories to put in the database.
: ${SEARCHPATHS="/"}

# Network (NFS, AFS, RFS, etc.) directories to put in the database.
: ${NETPATHS=}

# Directories to not put in the database, which would otherwise be.
: ${PRUNEPATHS="/tmp /usr/tmp /var/tmp /afs"}

# The same, in the form of a regex that find can use.
test -z "$PRUNEREGEX" &&
  PRUNEREGEX=`echo $PRUNEPATHS|sed -e 's,^,\\\(^,' -e 's, ,$\\\)\\\|\\\(^,g' -e 's,$,$\\\),'`

# The database file to build.
: ${LOCATE_DB=@LOCATE_DB@}

# Directory to hold intermediate files.
if test -d /var/tmp; then
  : ${TMPDIR=/var/tmp}
else
  : ${TMPDIR=/usr/tmp}
fi

# The user to search network directories as.
: ${NETUSER=daemon}

# The directory containing the subprograms.
: ${LIBEXECDIR=@libexecdir@}

# The directory containing find.
: ${BINDIR=@bindir@}

# The names of the utilities to run to build the database.
: ${find=@find@}
: ${frcode=@frcode@}
: ${bigram=@bigram@}
: ${code=@code@}

PATH=$LIBEXECDIR:$BINDIR:/usr/ucb:/bin:/usr/bin:$PATH export PATH

# Make and code the file list.
# Sort case insensitively for users' convenience.

if test $old = no; then

# FIXME figure out how to sort null-terminated strings, and use -print0.
{
if test -n "$SEARCHPATHS"; then
  $find $SEARCHPATHS \
  \( -fstype nfs -o -fstype NFS -o -type d -regex "$PRUNEREGEX" \) -prune -o -print
fi

if test -n "$NETPATHS"; then
  su $NETUSER -c \
  "$find $NETPATHS \\( -type d -regex \"$PRUNEREGEX\" -prune \\) -o -print"
fi
} | sort -f | $frcode > $LOCATE_DB.n

# To avoid breaking locate while this script is running, put the
# results in a temp file, then rename it atomically.
if test -s $LOCATE_DB.n; then
  rm -f $LOCATE_DB
  mv $LOCATE_DB.n $LOCATE_DB
  chmod 644 $LOCATE_DB
else
  echo "updatedb: new database would be empty" >&2
  rm -f $LOCATE_DB.n
fi

else # old

bigrams=$TMPDIR/f.bigrams$$
filelist=$TMPDIR/f.list$$
trap 'rm -f $bigrams $filelist; exit' 1 15

# Alphabetize subdirectories before file entries using tr.  James says:
# "to get everything in monotonic collating sequence, to avoid some
# breakage i'll have to think about."
{
if test -n "$SEARCHPATHS"; then
  $find $SEARCHPATHS \
  \( -fstype nfs -o -fstype NFS -o -type d -regex "$PRUNEREGEX" \) -prune -o -print
fi
if test -n "$NETPATHS"; then
  su $NETUSER -c \
  "$find $NETPATHS \\( -type d -regex \"$PRUNEREGEX\" -prune \\) -o -print"
fi
} | tr / '\001' | sort -f | tr '\001' / > $filelist

# Compute the (at most 128) most common bigrams in the file list.
$bigram < $filelist | sort | uniq -c | sort -nr |
  awk '{ if (NR <= 128) print $2 }' | tr -d '\012' > $bigrams

# Code the file list.
$code $bigrams < $filelist > $LOCATE_DB
chmod 644 $LOCATE_DB

rm -f $bigrams $filelist $errs

fi
