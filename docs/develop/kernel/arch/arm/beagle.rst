BeagleBone Black
================

-  http://beagleboard.org
-  TODO: This is a WIP

Hardware information (Rev A5A)
==============================

-  ARMv7 Architecture
-  Sitara AM3359AZCZ100 Cortex-A8 CPU @ 1 Ghz
-  PowerVR SGX530 3D GPU
-  eMMC Onboard Storage 2GB (MMC1)
-  SD Card Storage (MMC0)
-  512 MB DDR3L RAM
-  Video Outputs

   -  HDMI Video Output (with audio)

-  SMSC LAN8710A Ethernet

Setting up the Haiku SD card
============================

The BeagleBone Black supports booting from an microSD card while the
boot switch is pressed at power on. A MBR file system layout is normally
used as seen below. Partition 1 is all that is required to boot an OS.

-  partition 1 – FAT32, bootable flag, type ‘c’
-  partition 2 – BeFS, Haiku filesystem, type ‘eb’

Boot Partition
--------------

Required files
~~~~~~~~~~~~~~

-  MLO
-  u-boot.img: u-Boot image
-  uEnv.txt: u-Boot Environment settings

Optional files
~~~~~~~~~~~~~~

-  ID.txt: Unknown

Compiling
=========

-  Create your work directory
   ``mkdir generated.beagle; cd generated.beagle``
-  Build an ARM toolchain using
   ``../configure --build-cross-tools arm ../../buildtools --target-board beagle``
-  TODO

Booting
=======

1. If the boot switch is not depressed: MMC1, MMC0, UART0, USB0
2. If the boot switch is depressed: SPI0, MMC0, USB0, UART0

Emulation
=========

The Linaro Fork of QEmu has beagle board (and other OMAP3) support.
https://launchpad.net/qemu-linaro

It seems you get this as the default QEmu install on some, but not all,
Ubuntu versions. For other distros (or Haiku), you’ll have to compile it
yourself.

Additional information
======================

-  `CircutCo
   WikiPage <http://circuitco.com/support/index.php?title=BeagleBoneBlack>`__
-  `BeagleBone Black A5A
   SRM <https://github.com/CircuitCo/BeagleBone-Black/blob/master/BBB_SRM.pdf?raw=true>`__
