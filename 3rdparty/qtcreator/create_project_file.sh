#!/bin/bash
#
# Copyright 2017-2018, Haiku, Inc. All rights reserved.
# Distributed under the terms of the MIT license.
#

if [ "$#" -lt 2 ]; then
	echo "This script creates project files for Qt Creator to develop Haiku with."
	echo "It should only be used on a per-project basis, as Qt Creator is too slow"
	echo "when used on all of Haiku at once."
	echo ""
	echo "THIS SCRIPT *MUST* BE RUN FROM THE REPOSITORY ROOT."
	echo ""
	echo "Usage: <script> <project name> <path 1> <path 2> ..."
	echo "e.g: create_project_file.sh Tracker src/kits/tracker/ src/apps/tracker/"
	exit 1
fi

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
NAME=$1
ROOTDIRS=${@:2}

printf "// Add predefined macros for your project here. For example:\n// #define THE_ANSWER 42\n" \
	>$DIR/$NAME.config
printf "[General]\n" >$DIR/$NAME.creator

# Clear the old file lists
echo >$DIR/$NAME.files
echo >$DIR/$NAME.includes

# Build file lists
for rootdir in $ROOTDIRS; do
	find $rootdir -type f | sed "s@^@../../@" >>$DIR/$NAME.files
	find $rootdir -type d | sed "s@^@../../@" >>$DIR/$NAME.includes
done
find headers -type d | sed "s@^@../../@" >>$DIR/$NAME.includes

echo "Done. Project file: $DIR/$NAME.creator"
