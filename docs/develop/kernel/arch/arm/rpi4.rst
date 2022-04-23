Raspberry Pi 4
##############

-  http://raspberrypi.org

Hardware Information
====================

-  ARMv8 Architecture (64-bit)
-  Broadcom BCM2711 (SoC)

   -  Includes Quad ARM Cortex-A72 CPU @ 1.8 GHz
   -  Includes VideoCore VI GPU

-  SD Card Storage
-  1, 2, 4 or 8 GB RAM
-  Video Outputs

   -  2x HDMI Video Output via micro-HDMI
   -  Composite Video Output

-  Ethernet 10/100/1000 Mbit/s
-  Wifi b/g/n/ac dual band 2.4/5 GHz
-  Bluetooth 5.0
-  2x USB 2.0, 2x USB 3.0

Setting up the Haiku SD card
============================

The Raspberry Pi SD card generally uses the MBR file system layout
below. Partition 1 is all that is required to boot an OS.

-  partition 1 – FAT32, bootable flag, type ‘ef’
-  partition 2 – BeFS, Haiku filesystem, type ‘eb’

Boot Partition
--------------

Required Files
~~~~~~~~~~~~~~

-  start4.elf: The GPU binary firmware image
-  fixup4.dat: Additional code for the GPU
-  bcm2711-rpi-4-b.dtb: FDT binary for the Raspberry Pi 4B
-  bcm2711-rpi-400.dtb: FDT binary for the Raspberry Pi 400
-  config.txt: A configuration file read by the Pi to start u-boot.bin
-  u-boot.bin: u-boot loader for the Pi 4
-  boot.scr: u-boot loader script for starting Haiku Loader
-  uEnv.txt: configuration file for the boot.scr script
-  haiku_loader.efi: Haiku Loader

Note: unlike previous Raspberry Pi models, there's no bootcode.bin on the
Raspberry Pi 4 as it's replaced by a boot EEPROM.

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
-  Prepare customized SD card using the ``rune`` tool. E.g. ``rune -b rpi4 -i haiku-mmc.image /dev/mmcblk0``

Booting
=======

1. SOC starts the boot EEPROM
2. Boot EEPROM runs start4.elf
3. start4.elf reads config.txt and start u-boot
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
