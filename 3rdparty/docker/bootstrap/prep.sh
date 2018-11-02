#!/bin/bash

if [ ! -d $WORKPATH ]; then
	echo "$WORKPATH is not available!"
	exit 1
fi

if [ ! -d $WORKPATH/src ]; then
	mkdir $WORKPATH/src
fi

# These we clone or pull updates on
if [ ! -d $WORKPATH/src/buildtools ]; then
	git clone $GIT_BUILDTOOLS $WORKPATH/src/buildtools
else
	git -C $WORKPATH/src/buildtools pull --rebase
fi

if [ ! -d $WORKPATH/src/haikuporter ]; then
	git clone $GIT_HAIKUPORTER $WORKPATH/src/haikuporter
else
	git -C $WORKPATH/src/buildtools pull --rebase
fi

if [ ! -d $WORKPATH/src/haikuports ]; then
	git clone $GIT_HAIKUPORTS $WORKPATH/src/haikuports
else
	git -C $WORKPATH/src/haikuports pull --rebase
fi


# These we just clone since modifications are likely
if [ ! -d $WORKPATH/src/haikuports.cross ]; then
	git clone $GIT_HAIKUPORTS_CROSS $WORKPATH/src/haikuports.cross
fi

if [ ! -d $WORKPATH/src/haiku ]; then
	git clone $GIT_HAIKU $WORKPATH/src/haiku
fi

mkdir $WORKPATH/bin
if [ ! -f $WORKPATH/bin/jam ]; then
	cd $WORKPATH/src/buildtools/jam
	make
	cp -f bin.linuxx86/jam $WORKPATH/bin/jam
	cd -
fi
