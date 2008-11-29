#! /bin/sh
# updatedb -- build a locate pathname database
# Copyright (C) 1994, 1996, 1997, 2000, 2001, 2003, 2004, 2005, 2006
# Free Software Foundation, Inc.
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# csh original by James Woods; sh conversion by David MacKenzie.

usage="\
Usage: $0 [--findoptions='-option1 -option2...']
       [--localpaths='dir1 dir2...'] [--netpaths='dir1 dir2...']
       [--prunepaths='dir1 dir2...'] [--prunefs='fs1 fs2...']
       [--output=dbfile] [--netuser=user] [--localuser=user] 
       [--old-format] [--version] [--help]

Report bugs to <bug-findutils@gnu.org>."
changeto=/
old=no
for arg
do
  # If we are unable to fork, the back-tick operator will 
  # fail (and the shell will emit an error message).  When 
  # this happens, we exit with error value 71 (EX_OSERR).
  # Alternative candidate - 75, EX_TEMPFAIL.
  opt=`echo $arg|sed 's/^\([^=]*\).*/\1/'`  || exit 71
  val=`echo $arg|sed 's/^[^=]*=\(.*\)/\1/'` || exit 71
  case "$opt" in
    --findoptions) FINDOPTIONS="$val" ;;
    --localpaths) SEARCHPATHS="$val" ;;
    --netpaths) NETPATHS="$val" ;;
    --prunepaths) PRUNEPATHS="$val" ;;
    --prunefs) PRUNEFS="$val" ;;
    --output) LOCATE_DB="$val" ;;
    --netuser) NETUSER="$val" ;;
    --localuser) LOCALUSER="$val" ;;
    --old-format) old=yes ;;
    --changecwd)  changeto="$val" ;;
    --version) echo "GNU updatedb version @VERSION@"; exit 0 ;;
    --help) echo "$usage"; exit 0 ;;
    *) echo "updatedb: invalid option $opt
$usage" >&2
       exit 1 ;;
  esac
done

if test "$old" = yes; then
    echo "Warning: future versions of findutils will shortly discontinue support for the old locate database format." >&2

    sort="@SORT@"
    print_option="-print"
    frcode_options=""
else
    if @SORT_SUPPORTS_Z@
    then
        sort="@SORT@ -z"
        print_option="-print0"
        frcode_options="-0"
    else
        sort="@SORT@"
        print_option="-print"
        frcode_options=""
    fi
fi

getuid() {
    # format of "id" output is ...
    # uid=1(daemon) gid=1(other)
    # for `id's that don't understand -u
    id | cut -d'(' -f 1 | cut -d'=' -f2
}

# figure out if su supports the -s option
select_shell() {
    if su "$1" -s $SHELL -c false < /dev/null  ; then
	# No.
	echo ""
    else
	if su "$1" -s $SHELL -c true < /dev/null  ; then
	    # Yes.
	    echo "-s $SHELL"
        else
	    # su is unconditionally failing.  We won't be able to 
	    # figure out what is wrong, so be conservative.
	    echo ""
	fi
    fi
}


# You can set these in the environment, or use command-line options,
# to override their defaults:

# Any global options for find?
: ${FINDOPTIONS=}

# What shell shoud we use?  We should use a POSIX-ish sh.
: ${SHELL="/bin/sh"}

# Non-network directories to put in the database.
: ${SEARCHPATHS="/"}

# Network (NFS, AFS, RFS, etc.) directories to put in the database.
: ${NETPATHS=}

# Directories to not put in the database, which would otherwise be.
: ${PRUNEPATHS="/tmp /usr/tmp /var/tmp /afs /amd /sfs"}

# Trailing slashes result in regex items that are never matched, which 
# is not what the user will expect.   Therefore we now reject such 
# constructs.
for p in $PRUNEPATHS; do
    case "$p" in
	/*/)   echo "$0: $p: pruned paths should not contain trailing slashes" >&2
	       exit 1
    esac
done

# The same, in the form of a regex that find can use.
test -z "$PRUNEREGEX" &&
  PRUNEREGEX=`echo $PRUNEPATHS|sed -e 's,^,\\\(^,' -e 's, ,$\\\)\\\|\\\(^,g' -e 's,$,$\\\),'`

# The database file to build.
: ${LOCATE_DB=@LOCATE_DB@}

# Directory to hold intermediate files.
if test -d /var/tmp; then
  : ${TMPDIR=/var/tmp}
elif test -d /usr/tmp; then
  : ${TMPDIR=/usr/tmp}
else
  : ${TMPDIR=/tmp}
fi
export TMPDIR

# The user to search network directories as.
: ${NETUSER=daemon}

# The directory containing the subprograms.
if test -n "$LIBEXECDIR" ; then
    : LIBEXECDIR already set, do nothing
else
    : ${LIBEXECDIR=@libexecdir@}
fi

# The directory containing find.
if test -n "$BINDIR" ; then
    : BINDIR already set, do nothing
else
    : ${BINDIR=@bindir@}
fi

# The names of the utilities to run to build the database.
: ${find:=${BINDIR}/@find@}
: ${frcode:=${LIBEXECDIR}/@frcode@}
: ${bigram:=${LIBEXECDIR}/@bigram@}
: ${code:=${LIBEXECDIR}/@code@}


PATH=/bin:/usr/bin:${BINDIR}; export PATH

: ${PRUNEFS="nfs NFS proc afs proc smbfs autofs iso9660 ncpfs coda devpts ftpfs devfs mfs sysfs shfs"}

if test -n "$PRUNEFS"; then
prunefs_exp=`echo $PRUNEFS |sed -e 's/\([^ ][^ ]*\)/-o -fstype \1/g' \
 -e 's/-o //' -e 's/$/ -o/'`
else
  prunefs_exp=''
fi

# Make and code the file list.
# Sort case insensitively for users' convenience.

rm -f $LOCATE_DB.n
trap 'rm -f $LOCATE_DB.n; exit' HUP TERM

if test $old = no; then

# FIXME figure out how to sort null-terminated strings, and use -print0.
if {
cd "$changeto"
if test -n "$SEARCHPATHS"; then
  if [ "$LOCALUSER" != "" ]; then
    # : A1
    su $LOCALUSER `select_shell $LOCALUSER` -c \
    "$find $SEARCHPATHS $FINDOPTIONS \
     \\( $prunefs_exp \
     -type d -regex '$PRUNEREGEX' \\) -prune -o $print_option"
  else
    # : A2
    $find $SEARCHPATHS $FINDOPTIONS \
     \( $prunefs_exp \
     -type d -regex "$PRUNEREGEX" \) -prune -o $print_option
  fi
fi

if test -n "$NETPATHS"; then
myuid=`getuid` 
if [ "$myuid" = 0 ]; then
    # : A3
    su $NETUSER `select_shell $NETUSER` -c \
     "$find $NETPATHS $FINDOPTIONS \\( -type d -regex '$PRUNEREGEX' -prune \\) -o $print_option" ||
    exit $?
  else
    # : A4
    $find $NETPATHS $FINDOPTIONS \( -type d -regex "$PRUNEREGEX" -prune \) -o $print_option ||
    exit $?
  fi
fi
} | $sort -f | $frcode $frcode_options > $LOCATE_DB.n
then
    # OK so far
    true
else
    rv=$?
    echo "Failed to generate $LOCATE_DB.n" >&2
    rm -f $LOCATE_DB.n
    exit $rv
fi

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

if ! bigrams=`mktemp -t updatedbXXXXXXXXX`; then
    echo tempfile failed
    exit 1
fi

if ! filelist=`mktemp -t updatedbXXXXXXXXX`; then
    echo tempfile failed
    exit 1
fi

rm -f $LOCATE_DB.n
trap 'rm -f $bigrams $filelist $LOCATE_DB.n; exit' HUP TERM

# Alphabetize subdirectories before file entries using tr.  James Woods says:
# "to get everything in monotonic collating sequence, to avoid some
# breakage i'll have to think about."
{
cd "$changeto"
if test -n "$SEARCHPATHS"; then
  if [ "$LOCALUSER" != "" ]; then
    # : A5
    su $LOCALUSER `select_shell $LOCALUSER` -c \
    "$find $SEARCHPATHS $FINDOPTIONS \
     \( $prunefs_exp \
     -type d -regex '$PRUNEREGEX' \) -prune -o $print_option" || exit $?
  else
    # : A6
    $find $SEARCHPATHS $FINDOPTIONS \
     \( $prunefs_exp \
     -type d -regex "$PRUNEREGEX" \) -prune -o $print_option || exit $?
  fi
fi

if test -n "$NETPATHS"; then
  myuid=`getuid`
  if [ "$myuid" = 0 ]; then
    # : A7
    su $NETUSER `select_shell $NETUSER` -c \
     "$find $NETPATHS $FINDOPTIONS \\( -type d -regex '$PRUNEREGEX' -prune \\) -o $print_option" ||
    exit $?
  else
    # : A8
    $find $NETPATHS $FINDOPTIONS \( -type d -regex "$PRUNEREGEX" -prune \) -o $print_option ||
    exit $?
  fi
fi
} | tr / '\001' | $sort -f | tr '\001' / > $filelist

# Compute the (at most 128) most common bigrams in the file list.
$bigram $bigram_opts < $filelist | sort | uniq -c | sort -nr |
  awk '{ if (NR <= 128) print $2 }' | tr -d '\012' > $bigrams

# Code the file list.
$code $bigrams < $filelist > $LOCATE_DB.n

rm -f $bigrams $filelist

# To reduce the chances of breaking locate while this script is running,
# put the results in a temp file, then rename it atomically.
if test -s $LOCATE_DB.n; then
  rm -f $LOCATE_DB
  mv $LOCATE_DB.n $LOCATE_DB
  chmod 644 $LOCATE_DB
else
  echo "updatedb: new database would be empty" >&2
  rm -f $LOCATE_DB.n
fi

fi

exit 0
