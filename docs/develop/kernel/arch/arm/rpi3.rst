Raspberry Pi 3
##############

-  http://raspberrypi.org

Hardware Information
====================

-  ARMv8 Architecture (64-bit)
-  Broadcom BCM2837 (SoC)

   -  Includes Quad ARM Cortex-A53 CPU @ 1.2 GHz
   -  Includes VideoCore IV GPU

-  SD Card Storage
-  1 GB RAM
-  Video Outputs

   -  HDMI Video Output
   -  Composite Video Output

-  Ethernet 10/100 Mbit/s
-  Wifi b/g/n single band 2.4 GHz
-  Bluetooth 4.1 BLE
-  4x USB 2.0

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

-  bootcode.bin: 2nd stage bootloader
-  start.elf: The GPU binary firmware image
-  fixup.dat: Additional code for the GPU
-  bcm2710-rpi-3-b.dtb: FDT binary for the Raspberry Pi 3B
-  bcm2710-rpi-3-b-plus.dtb: FDT binary for the Raspberry Pi 3B+
-  config.txt: A configuration file read by the Pi to start u-boot.bin
-  u-boot.bin: u-boot loader for the Pi 3
-  boot.scr: u-boot loader script for starting Haiku Loader
-  uEnv.txt: configuration file for the boot.scr script
-  haiku_loader.efi: Haiku Loader

Optional Files
~~~~~~~~~~~~~~

-  vlls directory: Additional GPU code, e.g. extra codecs.
-  uEnv.txt: u-boot configuration script to automate boot.

Compiling Haiku and preparing SD card
=====================================

-  Create your ARM work directory
   ``mkdir generated.arm; cd generated.arm``
-  Build an ARM toolchain using
   ``../configure --build-cross-tools arm --cross-tools-source ../../buildtools``
-  Build SD card image using ``jam -q -j6 @minimum-mmc``
-  Prepare customized SD card using the ``rune`` tool. E.g. ``rune -b rpi3 -i haiku-mmc.image /dev/mmcblk0``
-  Ensure that partition 1 has partition type fat16: ``parted /dev/mmcblk0 set 1 esp off``

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

   uart_2ndstage=1
   enable_uart=1
   kernel=u-boot.bin


Additional Information
======================

-  `Latest Raspberry Pi
   firmware <http://github.com/raspberrypi/firmware/tree/master/boot>`__
-  `config.txt options <http://www.elinux.org/RPiconfig>`__
