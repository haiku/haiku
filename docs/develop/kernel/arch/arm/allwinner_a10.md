# Allwinner A10
* http://linux-sunxi.org

# Hardware Information

The A10 is a system-on chip. There are many devices based on it, for example
the CubieBoard and the Rikomagic mk802 (versions I and II).

* ARMv7 Architecture (Cortex-A8)
* Mali 400MP GPU
* CedarX VPU
* SD Card Storage
* 1GB RAM (DDR)
* 4GB NAND Flash
* Video Outputs
  * HDMI Video Output
* Ethernet
* USB

# Setting up the Haiku SD card

Not so fun layout here. The A10 boot ROM reads raw blocks from the SD card
(MBR style), so the bootloader can't just be dropped in a FAT32 partition.

*  8KB partition table
*  24KB SPL loader
*  512KB u-boot
*  128KB u-boot environment variables
*  352KB unused
*  partition 1 -- FAT32 or ext2 (anything u-boot can read is fine)
*  partition 2 -- BeFS, Haiku filesystem, type 'eb'

Note this layout can be a bit different depending on the u-boot version used,
some versions will store the environment in uEnv.txt in the FAT32 partition
instead. Since everything is loaded from the SD Card, we are free to customize
the u-boot or even remove it and get haiku_loader booting directly.

## Boot Partition

### Required Files

*  haiku_loader: Haiku Loader
*  haiku-floppyboot.tgz: Compressed image with Haiku kernel

# Booting

1. SOC load SPL
2. SPL loads u-boot
2. u-boot loads and run the kernel

SPL is a small binary (24K) loaded from a fixed location on the SD card. It
does minimal hardware initializations, then loads u-boot, also from the SD
card. From there on things go as usual.

In the long term, we can make haiku_loader be an SPL executable on this
platform, if it fits the 24K size limit, or have a custom stage1 that loads it.
For now, u-boot can be an useful debugging tool.

## Script.bin

In order to work on different devices (RAM timings, PIO configs, ...), the
Linux kernels for Allwinner chips use a "script.bin" file. This is loaded to
RAM at a fixed address by u-boot, then the Kernel parses it and uses it to
configure the hardware (similar to FDT).

We should probably NOT use this, and convert the script.bin file to an FDT
instead. The format is known and there are tools to convert the binary file
to an editable text version and back (bin2fex and fex2bin).

This FEX stuff isn't merged in mainline Linux, and lives on as Allwinner
patches. The mainline Linux kernel has some A10 support, rewritten to use
FDT. We may use the FDT files from there for the most common boards.

# Emulation support

qemu 1.0 has a Cubieoard target which emulates this chip.

# Useful links

Arch Linux instructions on creating a bootable SD card (partition layout, etc)
http://archlinuxarm.org/platforms/armv7/allwinner/cubieboard#qt-platform_tabs-ui-tabs2

Linux SunXi: mainline Linux support for the Allwinner chips. Lots of docs on the hardware.
http://linux-sunxi.org/
