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
mkdir -p $GENERATED

echo "Beginning a bootstrap build for $TARGET_ARCH at $GENERATED..."

cd $GENERATED
$WORKPATH/src/haiku/configure -j4 --build-cross-tools $TARGET_ARCH $WORKPATH/src/buildtools \
	--bootstrap $WORKPATH/src/haikuporter/haikuporter $WORKPATH/src/haikuports.cross $WORKPATH/src/haikuports

echo "If everything was successful, your next step is 'TARGET_ARCH=$TARGET_ARCH make bootstrap'"
