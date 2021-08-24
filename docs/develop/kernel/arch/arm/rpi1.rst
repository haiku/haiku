Raspberry Pi
############

-  http://raspberrypi.org

Hardware Information
====================

-  ARMv6 Architecture
-  Broadcom BCM2835 (SoC)

   -  Includes ARM1176JZF-S CPU @ 700 MHz
   -  Includes VideoCore IV GPU

-  SD Card Storage
-  256 or 512 MB RAM (depending on revision)
-  Video Outputs

   -  HDMI Video Output
   -  Composite Video Output

-  Ethernet

Setting up the Haiku SD card
============================

The Raspberry Pi SD card generally uses the MBR file system layout
below. Partition 1 is all that is required to boot an OS.

-  partition 1 – FAT32, bootable flag, type ‘c’
-  partition 2 – BeFS, Haiku filesystem, type ‘eb’

Boot Partition
--------------

Required Files
~~~~~~~~~~~~~~

-  bootcode.bin : 2nd stage bootloader
-  start.elf: The GPU binary firmware image
-  config.txt: A configuration file read by the Pi to start u-boot.bin
-  u-boot.bin: u-boot loader for the Pi 2
-  bcm2835-rpi-b.dtb: FDT binary for the Raspberry Pi 2
-  haiku_loader_linux.ub: Haiku Loader
-  haiku-floppyboot.tgz.ub: Compressed initial ram image with Haiku
   kernel

Optional Files
~~~~~~~~~~~~~~

-  vlls directory: Additional GPU code, e.g. extra codecs.
-  uEnv.txt: u-boot configuration script to automate boot.

Compiling
=========

-  Create your ARM work directory
   ``mkdir generated.arm; cd generated.arm``
-  Build an ARM toolchain using
   ``../configure --build-cross-tools arm ../../buildtools --target-board=rpi1``
-  Build our loader using ``jam -q haiku_loader_linux.ub``
-  Build our initial ram disk using ``jam -q haiku-floppyboot.tgz.ub``

Booting
=======

1. SOC finds bootcode.bin
2. bootcode.bin runs start.elf
3. start.elf reads config.txt and start u-boot
4. u-boot.bin starts the Haiku loader
5. Haiku loader boots Haiku kernel

config.txt Options
------------------

::

   kernel=u-boot.bin

u-boot startup
--------------

These will be condensed and automated long-term via uEnv.txt :-)

-  ``fatload mmc 0 ${fdt_addr_r} bcm2835-rpi-b.dtb``
-  ``fdt addr ${fdt_addr_r}``
-  ``fatload mmc 0 ${ramdisk_addr_r} haiku-floppyboot.tgz.ub``
-  ``fatload mmc 0 ${kernel_addr_r} haiku_loader_linux.ub``
-  ``bootm ${kernel_addr_r} ${ramdisk_addr_r} ${fdt_addr_r}``

Additional Information
======================

-  `Latest Raspberry Pi
   firmware <http://github.com/raspberrypi/firmware/tree/master/boot>`__
-  `config.txt options <http://www.elinux.org/RPiconfig>`__
