#!/bin/sh
# UNISRC_ID: @(#)shar.sh	27.1	84/12/17  
: Make a shell archive package

# Usage: $0 [-b] [-c] [-t] [-v] files... > package
# See the manual entry for details.


# Initialize:

diagnostic='eval echo >&2'	# diagnostics to stderr by default.
trap '$diagnostic "$0: quitting early"; exit 1' 1 2 3 15
base_option=FALSE		# use pathnames, not basenames.
check_option=FALSE		# don't generate integrity check.
USAGE='Usage: $0 \[-b] \[-c] \[-t] \[-v] files... \> package'


# Extract and digest options, if any:
#
# Un-comment the "-)" line below to treat single dashes as a no-op.
# Commented means single dashes elicit a usage diagnostic.

while [ -n "$1" ]	# while there are more arguments,
do			# digest them; stop when you find a non-option.
	case "$1" in
	-b)	base_option=TRUE;	shift;;
	-c)	check_option=TRUE;	shift;;
	-v)	verbose=TRUE;		shift;;
	-t)	verbose=TRUE; diagnostic='eval echo >/dev/tty'; shift;;
###	-)	shift;;		# if uncommented, eat single dashes.
	-*)	$diagnostic $USAGE; exit 1;;	 # die at illegal options.
	*)	break;;		# non-option found.
	esac
done


# Check remaining arguments, which should be just a list of files:

if [ $# = 0 ]
then	# no arguments left!
	$diagnostic $USAGE
	exit 1
fi


# Check the cupboard to see if the ingredients are all there:

contents=''	# no files so far.
contdirs=''	# no directories so far.

for arg		# for all files specified,
do		# establish the archive name.
	if [ -f "$arg" ]
	then	# file exists and is not a directory.
		case $base_option in
		TRUE)	unpack_name=`basename "$arg"` ;;
		FALSE)	unpack_name="$arg" ;;
		esac

		contents="$contents $unpack_name"

	elif [ -d "$arg" ]
	then	# file exists and is a directory.
		case $base_option in
		TRUE)   $diagnostic '$0: cannot archive directory "$arg" with -b option.'
			exit 1 ;;
		FALSE)  contdirs="$contdirs $arg/ " ;;
		esac

	else	# not a plain file and not a directory.
		$diagnostic '$0: cannot archive "$arg"'
		exit 1
	fi
done


# Emit the prologue:
# (The leading newline is for those who type csh instead of sh.)

cat <<!!!

# This is a shell archive.  Remove anything before this line,
# then unpack it by saving it in a file and typing "sh file".
#
# Wrapped by `who am i | sed 's/[ 	].*//'` on `date`
!!!


# Emit the list of ingredients:

# Simple version (breaks if you shar lots of files at once):
#	echo "# Contents: $contdirs$contents"
#
# Complex and cosmetic version to pack contents list onto lines that fit on
# one terminal line ("expr string : .*" prints the length of the string):

MAX=80
line='# Contents: '
for item in $contdirs $contents
do
	if [ `expr "$line" : '.*' + 1 + "$item" : '.*'` -lt $MAX ]
	then	# length of old line + new item is short enough,
		line="$line $item"	# so just append it.

	else	# new element makes line too long,
		echo "$line"		# so put it on a new line.
		line="#	$item"		# start a new line.
		MAX=74			# compensate for tab width.
	fi
done

echo "$line"
echo " "


# Emit the files and their separators:

for arg
do
	# Decide which name to archive under.
	case $base_option in
	TRUE)   unpack_name=`basename "$arg"`
		test $verbose && $diagnostic "a - $unpack_name [from $arg]" ;;
	FALSE)  unpack_name="$arg"
		test $verbose && $diagnostic "a - $arg" ;;
	esac

	# Emit either a mkdir or a cat/sed to extract the file.
	if [ -d "$arg" ]
	then
		echo "echo mkdir - $arg"
		echo "mkdir $arg"
	else
		echo "echo x - $unpack_name"
		separator="@//E*O*F $unpack_name//"
		echo "sed 's/^@//' > \"$unpack_name\" <<'$separator'"
		sed  -e 's/^[.~@]/@&/'  -e 's/^From/@&/'  "$arg"
		echo $separator
	fi

	# Emit chmod to set permissions on the extracted file;
	# this keels over if the filename contains "?".
	ls -ld $arg | sed \
		-e 's/^.\(...\)\(...\)\(...\).*/u=\1,g=\2,o=\3/' \
		-e 's/-//g' \
		-e 's?.*?chmod & '"$unpack_name?"
	echo " "
done


# If the -c option was given, emit the checking epilogue:
# (The sed script converts files to basenames so it works regardless of -b.)

if [ $check_option = TRUE ]
then
	echo 'echo Inspecting for damage in transit...'
	echo 'temp=/tmp/shar$$; dtemp=/tmp/.shar$$'
	echo 'trap "rm -f $temp $dtemp; exit" 0 1 2 3 15'
	echo 'cat > $temp <<\!!!'
	case $base_option in
	TRUE)   wc $@ | sed 's=[^ ]*/=='	;;
	FALSE)  wc $contents | sed 's=[^ ]*/=='	;;
	esac
	echo '!!!'
	echo "wc $contents | sed 's=[^ ]*/==' | "'diff -b $temp - >$dtemp'
	echo 'if [ -s $dtemp ]'
	echo 'then echo "Ouch [diff of wc output]:" ; cat $dtemp'
	echo 'else echo "No problems found."'
	echo 'fi'
fi


# Finish up:

echo 'exit 0'	# sharchives unpack even if junk follows.
exit 0
