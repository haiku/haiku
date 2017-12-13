#!/bin/bash
#
# Creates a sysroot from a running Haiku system suitable
# for bootstrapping / cross-compiling Haiku applications
# under other platforms.
#
# Resulting tar.gz is generally extracted at cross-tools-$ARCH/sysroot
#

OS=$(uname -o)
ARCH=$(uname -p)
REV=$(uname -v | awk '{ print $1 }')

EXCLUDE="/boot/system/packages /boot/system/var/swap"

OUTPUT="sysroot-$OS-$ARCH-$REV.tar.gz"

echo "Generating $ARCH sysroot..."

tar $(for i in $EXCLUDE; do echo "--exclude $i"; done) -cvzf $OUTPUT /boot/system /bin /etc /packages /system /tmp

if [ $? -ne 0 ]; then
	echo "Error creating sysroot package!"
	return 1;
fi

echo "sysroot $OUTPUT successfully created!"
