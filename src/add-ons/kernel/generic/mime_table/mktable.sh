#!/bin/sh

function dump_mime () {
	while [ ! -z "$1" ]; do
		case "$1" in
		-set)
			shift;
			mime="$1"
			;;
		-extension)
			shift
			extension="$1"
			echo "	{ \"$extension\", \"$mime\" },"
			;;
		esac
		shift
	done
}

# header
echo "struct ext_mime mimes[] = {"

setmime -dump | grep extension | sed 's/^[^ ]*setmime /dump_mime /' | while read L; do
	eval $L
done

#footer
echo ""
echo "	{ 0, 0 }"
echo "};"

