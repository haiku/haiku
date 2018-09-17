#!/bin/bash

# THIS IS NOT THE FINAL SOLUTION
# DIRTY HAX FOR R1 BETA 1!

if [ $# -lt 2 ];  then
	echo "$0 <greasy profile> <greasy source dir>"
	exit 1
fi

PROFILE="$1"
SOURCE="$2"

HAIKU_BOOT_PLATFORM=efi jam -q haiku_loader.efi
unset HAIKU_BOOT_PLATFORM

EFI_LOADER=./objects/haiku/x86_64/release/system/boot/efi/haiku_loader.efi
ANYBOOT=./objects/linux/x86_64/release/tools/anyboot/anyboot

cat << EOF | patch -t -p1 -d${SOURCE}
diff --git a/build/jam/images/AnybootImage b/build/jam/images/AnybootImage
index b1bdca7783..6b71c16aaa 100644
--- a/build/jam/images/AnybootImage
+++ b/build/jam/images/AnybootImage
@@ -12,11 +12,14 @@ rule BuildAnybootImage anybootImage : mbrPart : isoPart : imageFile {
	Depends \$(anybootImage) : \$(mbrPart) ;
	Depends \$(anybootImage) : \$(imageFile) ;
 
-	BuildAnybootImage1 \$(anybootImage) : \$(anyboot) \$(mbrPart) \$(isoPart) \$(imageFile) ;
+	# WE HAX THE BEST HAX FOR R1BETA1
+	local efiPart = "efi.img" ;
+
+	BuildAnybootImage1 \$(anybootImage) : \$(anyboot) \$(mbrPart) \$(efiPart) \$(isoPart) \$(imageFile) ;
 }
 
 actions BuildAnybootImage1 {
-	\$(2[1]) -b \$(2[2]) \$(2[3]) \$(2[4]) \$(1)
+	\$(2[1]) -b \$(2[2]) -e \$(2[3]) \$(2[4]) \$(2[5]) \$(1)
 }
 
 local baseMBR = base_mbr.bin ;
EOF

EFI_OUT=efi.img
rm ${EFI_OUT}

echo "dd image..."
dd if=/dev/zero of=${EFI_OUT} bs=1M count=2

echo "mtools scripting..."
echo "drive i: file=\"${EFI_OUT}\" cylinders=8 heads=255 sectors=63 mformat_only" > ${EFI_OUT}.mtools

echo "mformat EFI partition..."
MTOOLSRC=${EFI_OUT}.mtools mformat -L 32 -v "UEFI" i:

echo "construct EFI filesystem..."
MTOOLSRC=${EFI_OUT}.mtools mmd i:/EFI
MTOOLSRC=${EFI_OUT}.mtools mmd i:/EFI/BOOT
MTOOLSRC=${EFI_OUT}.mtools mcopy ${EFI_LOADER} i:/EFI/BOOT/BOOTX64.EFI
MTOOLSRC=${EFI_OUT}.mtools mdir i:
MTOOLSRC=${EFI_OUT}.mtools mdir i:/EFI/BOOT

echo "cleanup..."
rm ${EFI_OUT}.mtools

jam -q @${PROFILE}-anyboot
git -C ${SOURCE} checkout build/jam/images/AnybootImage
