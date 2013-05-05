# Raspberry Pi
* http://raspberrypi.org

# Hardware Information

* ARMv6 Architecture
* Broadcom BCM2835 (SoC)
  * Includes ARM1176JZF-S CPU @ 700 MHz
  * Includes VideoCore IV GPU
* SD Card Storage
* 256 or 512 MB RAM (depending on revision)
* Video Outputs
  * HDMI Video Output
  * Composite Video Output
* Ethernet

# Setting up the Haiku SD card

The Raspberry Pi SD card generally uses the MBR file system layout below. Partition 1 is all that is required to boot an OS.

*  partition 1 -- FAT32, bootable flag, type 'c'
*  partition 2 -- BeFS, Haiku filesystem, type 'eb'

## Boot Partition

### Required Files

*  bootcode.bin : 2nd stage bootloader
*  start.elf: The GPU binary firmware image
*  haiku_loader: Haiku Loader
*  haiku-floppyboot.tgz: Compressed image with Haiku kernel
*  config.txt: A configuration file read by the GPU.

### Optional Files

*  vlls directory: Additional GPU code, e.g. extra codecs.

# Compiling

*  Build an ARM toolchain using `./configure --build-cross-tools-gcc4 arm ../buildtools`
*  Build our loader using `jam -q -sHAIKU_BOOT_BOARD=raspberry_pi -sHAIKU_BOOT_PLATFORM=raspberrypi_arm haiku_loader`
*  Build our file system using `jam -q -sHAIKU_BOOT_BOARD=raspberry_pi -sHAIKU_BOOT_PLATFORM=raspberrypi_arm haiku-floppyboot.tgz`

# Booting

1. SOC finds bootcode.bin
2. bootcode.bin runs start.elf
2. start.elf reads config.txt and cmdline.txt
3. start.elf runs specified binary at specified address

## config.txt Options

    kernel=haiku_loader
    kernel_address=0x0
    disable_commandline_tags=1
    ramfsfile=haiku-floppyboot.tgz
    ramfsaddr=0x04000000

# Additional Information

* [Latest Raspberry Pi firmware](http://github.com/raspberrypi/firmware/tree/master/boot)
* [config.txt options](http://www.elinux.org/RPiconfig)

