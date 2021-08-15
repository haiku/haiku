#!/bin/bash
#
# Creates a "firmware SD Card" with u-boot for the SiFive Unmatched board
# based on https://u-boot.readthedocs.io/en/latest/board/sifive/unmatched.html
#

if [[ $# -ne 1 ]]; then
	echo "Usage: $0 /dev/sdX"
	echo "WARNING: Everything on target block device will be erased!"
	exit 1
fi

if [[ $(whoami) != "root" ]]; then
	echo "Run me as root!  sudo $0"
	exit 1
fi

DEVICE=$1
URI_BASE="https://raw.githubusercontent.com/haiku/firmware/master/u-boot/riscv64/unmatched"

echo "Unmounting SD Card at $DEVICE..."
umount -q $DEVICE*

echo "Setting up SD Card at $DEVICE..."
sgdisk -g --clear -a 1 \
	--new=1:34:2081 --change-name=1:spl --typecode=1:5B193300-FC78-40CD-8002-E86C45580B47 \
	--new=2:2082:10273 --change-name=2:uboot --typecode=2:2E54B353-1271-4842-806F-E436D6AF6985 \
	$DEVICE

echo "Writing u-boot to $DEVICE..."
curl -s $URI_BASE/u-boot-spl.bin --output - | dd of=$DEVICE seek=34
curl -s $URI_BASE/u-boot.itb --output - | dd of=$DEVICE seek=2082

echo "Complete! Here are the next steps:"
echo " 1) Plug the SD Card into the Unmatched board"
echo " 2) Write haiku-mmc.image to a USB drive"
echo " 3) Plug the USB drive into the Unmatched"
echo "Enjoy Haiku!"
