#!/bin/sh -x
IMG=$HOME/floppy.img
jam -q zbeos && \
dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=9 conv=notrunc && \
dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=9 conv=notrunc skip=9 seek=$((9*180)) && \
dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=9 conv=notrunc skip=9 seek=$((2*9)) && \
dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=9 conv=notrunc skip=$((2*9)) seek=$((2*2*9)) && \
dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=9 conv=notrunc skip=$((3*9)) seek=$((3*2*9)) && \
src/system/boot/platform/atari_m68k/fixup_tos_floppy_chksum $IMG && \
generated.m68k/cross-tools/bin/m68k-unknown-haiku-ld -o haiku.prg -T src/system/ldscripts/m68k/boot_prg_atari_m68k.ld generated/objects/haiku/m68k/release/system/boot/boot_loader_atari_m68k && \
zip haiku.prg.zip haiku.prg



#dd if=generated/objects/haiku/m68k/release/system/boot/zbeos of=$IMG bs=512 count=500 conv=notrunc && \


