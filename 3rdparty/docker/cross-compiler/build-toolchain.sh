#!/bin/bash
# Copyright 2016 The Rust Project Developers. See the COPYRIGHT
# file at the top-level directory of this distribution and at
# http://rust-lang.org/COPYRIGHT.
#
# Licensed under the Apache License, Version 2.0 <LICENSE-APACHE or
# http://www.apache.org/licenses/LICENSE-2.0> or the MIT license
# <LICENSE-MIT or http://opensource.org/licenses/MIT>, at your
# option. This file may not be copied, modified, or distributed
# except according to those terms.

set -ex

ARCH=$1
SECONDARY_ARCH=$2

TOP=$(pwd)

BUILDTOOLS=$TOP/buildtools
HAIKU=$TOP/haiku
OUTPUT=/tools
SYSROOT=$OUTPUT/cross-tools-$ARCH/sysroot
SYSROOT_SECONDARY=$OUTPUT/cross-tools-$SECONDARY_ARCH/sysroot
PACKAGE_ROOT=/system

# First up, build a cross-compiler
git clone --depth=1 https://git.haiku-os.org/haiku
git clone --depth=1 https://git.haiku-os.org/buildtools
cd $BUILDTOOLS/jam
make && ./jam0 install
mkdir -p $OUTPUT
cd $OUTPUT
configureArgs="--build-cross-tools $ARCH $TOP/buildtools"
if [ -n "$SECONDARY_ARCH" ]; then
	configureArgs="$configureArgs --build-cross-tools $SECONDARY_ARCH"
fi
$HAIKU/configure $configureArgs

# Set up sysroot to redirect to /system
mkdir -p $SYSROOT/boot
mkdir -p $PACKAGE_ROOT
ln -s $PACKAGE_ROOT $SYSROOT/boot/system
if [ -n "$SECONDARY_ARCH" ]; then
	mkdir -p $SYSROOT_SECONDARY/boot
	ln -s $PACKAGEROOT $SYSROOT_SECONDARY/boot/system
fi

# Build needed packages and tools for the cross-compiler
jam -q haiku.hpkg haiku_devel.hpkg '<build>package'
if [ -n "$SECONDARY_ARCH" ]; then
	jam -q haiku_${SECONDARY_ARCH}.hpkg haiku_${SECONDARY_ARCH}_devel.hpkg
fi

# Set up our sysroot
cp $OUTPUT/objects/linux/lib/*.so /lib/x86_64-linux-gnu
cp $OUTPUT/objects/linux/x86_64/release/tools/package/package /bin/
for file in $SYSROOT/../bin/*; do
	ln -s $file /bin/$(basename $file)
done
#find $SYSROOT/../bin/ -type f -exec ln -s {} /bin/ \;
if [ -n "$SECONDARY_ARCH" ]; then
	for file in $SYSROOT_SECONDARY/../bin/*; do
		ln -s $file /bin/$(basename $file)-$SECONDARY_ARCH
	done
	#find $SYSROOT_SECONDARY/../bin/ -type f -exec ln -s {} /bin/{}-$SECONDARY_ARCH \;
fi

# Extract packages
package extract -C $PACKAGE_ROOT $OUTPUT/objects/haiku/$ARCH/packaging/packages/haiku.hpkg
package extract -C $PACKAGE_ROOT $OUTPUT/objects/haiku/$ARCH/packaging/packages/haiku_devel.hpkg
if [ -n "$SECONDARY_ARCH" ]; then
	package extract -C $PACKAGE_ROOT $OUTPUT/objects/haiku/$ARCH/packaging/packages/haiku_${SECONDARY_ARCH}.hpkg
	package extract -C $PACKAGE_ROOT $OUTPUT/objects/haiku/$ARCH/packaging/packages/haiku_${SECONDARY_ARCH}_devel.hpkg
fi
find $OUTPUT/download/ -name '*.hpkg' -exec package extract -C $PACKAGE_ROOT {} \;
cd $PACKAGE_ROOT/develop/lib
ln -s ../../lib/libgcc_s.so libgcc_s.so
if [ -n "$SECONDARY_ARCH" ]; then
	cd $PACKAGE_ROOT/develop/lib/$SECONDARY_ARCH
	ln -s ../../../lib/$SECONDARY_ARCH/libgcc_s.so libgcc_s.so
fi

# Clean up
rm -rf $BUILDTOOLS
rm -rf $HAIKU
rm -rf $OUTPUT/Jamfile $OUTPUT/attributes $OUTPUT/build $OUTPUT/build_packages $OUTPUT/download $OUTPUT/objects

if [ -n "$SECONDARY_ARCH" ]; then
	echo "Cross compilers for $ARCH-unknown-haiku and $SECONDARY_ARCH-unknown-haiku built and configured"
else
	echo "Cross compiler for $ARCH-unknown-haiku built and configured"
fi
