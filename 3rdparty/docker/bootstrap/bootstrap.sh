#!/bin/bash

if [ ! -d $WORKPATH ]; then
	echo "$WORKPATH is not available!"
	exit 1
fi

if [ ! -d $WORKPATH/src ]; then
	echo "$WORKPATH/src is not available!"
	exit 1
fi

if [ -z $TARGET_ARCH ]; then
	echo "TARGET_ARCH isn't set!"
	exit 1
fi

GENERATED=$WORKPATH/generated.$TARGET_ARCH

cd $GENERATED
HAIKU_PORTER_EXTRA_OPTIONS="--debug" jam -q @bootstrap-raw
