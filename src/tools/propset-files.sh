#!/bin/bash

# A quick and dirty script, to help add the svn property to files.
# It is intended to be ran with a relative path from the desired directory.
# e.g.,
# haiku/trunk/docs> sh ../src/tools/propset-files.sh

tmpDir="`finddir B_COMMON_TEMP_DIRECTORY`"
tmpFile="${tmpDir}/propset-html"
find . -type d -name .svn -prune -o -print > "$tmpFile"

while read file ; do
	hasProperty=`svn propget svn:mime-type "$file"`

	if ! [ $hasProperty ] ; then
		case "$file" in
			*.html)
				mimetype="text/html"
				svn propset svn:mime-type "$mimetype" "$file"
				;;
			#*.png)
			#	mimetype="image/png"
			#	#svn propset svn:mime-type "$mimetype" "$file"
			#	;;
			#*)
			#	echo $file
			#	;;
		esac
	fi
done < "$tmpFile"

rm "$tmpFile"
