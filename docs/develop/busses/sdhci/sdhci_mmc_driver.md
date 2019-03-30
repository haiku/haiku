# SDHCI MMC Driver

This driver project is a part of GSoC'18 and is aimed at providing support for
PCI devices with class 8 and subclass 5 over x86 architecture. This document
will make you familiar with the [code produced during GSoC](https://review.haiku-os.org/#/c/haiku/+/318/),
loading and testing the driver(including hardware emulation), insight into the
code and future tasks.

For detailed explanations about the project, you can refer the
[weekly reports](https://www.haiku-os.org/blog/krish_iyer) and comment issues
if any. For this project we have referred [SD Host Controller Spec Version 1.00](https://www.sdcard.org/downloads/pls/pdf/index.php?p=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.jpg&f=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.pdf&e=EN_A2100)
and [Physical Layer Spec Version 1.10](https://www.sdcard.org/downloads/pls/pdf/index.php?p=Part1_Physical_Layer_Simplified_Specification_Ver1.10.jpg&f=Part1_Physical_Layer_Simplified_Specification_Ver1.10.pdf&e=EN_P1110).

## Loading and testing the  driver
### Emulating the hardware

We will emulate a SDHC device using qemu as all system may not have the device.
These days systems provide transfer to SD/ MMC card over USB. The document will
not instruct you on how to build haiku but you can refer the link to
[compile and build the haiku images](https://www.haiku-os.org/guides/building/)
or the [week #1 and #2](https://www.haiku-os.org/blog/krish_iyer/2018-05-06_gsoc_2018_sdhci_mmc_driver_week_1_and_2/)
project report will also work.

After building the image, we will emulate the hardware and host haiku on top of that.

#### Emulation
For emulating a sdhci-pci device

    qemu-img create sd-card.img 10G
    qemu-system-x86_64 ~/haiku/generated.x86_64/haiku.image -hdd haiku_drive_2.img -device sdhci-pci -device sd-card,drive=mydrive -drive if=sd,index=0,file=sd.img,format=raw,id=mydrive -m 512M -enable-kvm

### Testing and loading the driver
The code is merged but not part of the default build
because it's not useful for end users currently. In order to add the drivers
to the image and load them, we need to adjust some buildfiles.

The required changes are in [this changeset](https://review.haiku-os.org/c/haiku/+/448/8).

## Insight into the code and future tasks
### Directory and files where all the code related to the project resides

#### MMC Bus
*    src/add-ons/kernel/busses/mmc
    *    sdhci-pci.cpp
    *    sdhci-pci.h
    *    Jamfile
#### The Bus Manager
* src/add-ons/kernel/bus_managers/mmc
    * mmc_bus.cpp
    * mmc_module.cpp
    * mmc_bus.h
    * Jamfile
#### Disk Driver
* src/add-ons/kernel/drivers/disk/mmc
    * mmc_disk.cpp
    * mmc_disk.h
    * Jamfile
#### Hardcoding the driver
* src/system/kernel/device_manager
    * device_manager.cpp
### Insight into the code
#### MMC Bus management overview

The device tree for MMC support looks like this:

* PCI bus manager
  * (other PCI devices)
  * SDHCI controller
    * SDHCI bus
      * MMC bus manager
	    * MMC device
          * mmc\_disk device
		* MMC device
          * (other SDIO driver)
      * MMC bus manager (second MMC bus)
	    * MMC device
		  * mmc\_disk device

At the first level, the PCI bus manager publishes a device node for each device
found. One of them is our SDHCI controller, identified either by the PCI device
class and subclass, or for not completely SDHCI compatible device, by the
device and vendor IDs.

The SDHCI bus driver attaches to this device and publishes his own node. It
then scans the device and publishes an MMC bus node for each slot (there may
be multiple SD slots attached to a single PCI controller).

The MMC bus manager then attach to each of these slots, and send the appropriate
commands for enumerating the SD cards (there may be multiple cards in a "slot"),
and publishes a device node for each of them. Finally, the mmc\_disk driver can
bind itself to one of these device nodes, and publish the corresponding disk
node, which is also be made available in /dev/disk/mmc.

Currently the mmc bus does not publish anything in the devfs, but this could be
added if sending raw SD/MMC commands to SD cards from userland is considered
desirable.

#### SDHCI driver

The SDHCI driver is the lowest level of the MMC stack. It provides abstraction
of the SDHCI device. Later on, different way to access an SD bus may be added,
for example for ARM devices which decided to use a different register interface.

The entry point is as usual **supports\_device()**. This method is called only
for devices which may be SDHCI controllers, thanks to filtering done in the
device manager to probe only the relevant devices. The probing is done on-demand,
currently when the system is enumerating /dev/disk in the devfs. Later on, when
we have SDIO support, probing will also be triggered in other cases.

The function identifies the device by checking the class and subclass, as well
as a limited set of hardcoded PCI device and vendor IDs for devices that do not
use the assigned subclass.

Once a compatible device is found, **register\_child\_devices()** is used to
publish device nodes for each slot to be controlled by the mmc bus manager.
The registers for each device are mapped into virtual memory, using the
information from the PCI bar registers. **struct registers** is defined so that
it matches the register layout, and provide a little abstraction to raw register
access.

An SdhciBus object is created to manage each of these busses at the SDHCI level.
It will be responsible for executing SD commands on that bus, and dealing with
the resulting interrupts.

#### The Bus Manager

The MMC bus manager manages the MMC bus (duh). Its tasks are:

* enumerating SD cards on the bus
* assigning RCAs to the cards for identifying them when sending commands
* setting the bus clock speed according to what the cards can handle
* remember which SD card is currently active (CMD7)
* manage cards state
* publish device nodes for each card

#### Disk Driver

The disk driver is attached to devices implementing SDSC or SDHC/SDXC commands.
There will be other drivers for non-storage (SDIO) cards.

To help with this, the MMC bus manager provides the device with the information
it gathered while initializing the device. According to the commands recognized
by the card during the initialization sequence, it's possible to know if it's
SDSC, SDHC/SDXC, or something else (SDIO, legacy MMC, etc).

The disk driver publishes devfs entries in /dev/disk/mmc and implements the
usual interface for disk devices. From this point on, the device can be used
just like any other mass storage device.

#### Getting everything loaded

The device manager is not completely implemented yet. As a result, some
decisions about which drivers to load are hardcoded in device\_manager.cpp.

It has been adjusted to handover SDHCI devices to the MMC bus. Whenever a
"disk" device is requested, the MMC busses are searched, which results in
loading the SDHCI driver and probing for SD cards. When we get support for
other types of SDIO devices, we will need to adjust the device manager to
probe the SDHCI bus when these type of devices are requested, too.


### Tasks to be completed

The SDHCI driver is able to send and receive commands. However it does not
handle card insertion and removal interrupts yet, so the card must be already
inserted when the driver is loaded.

As a proof of concept, the initialization sequence has been implemented up
until assigning an RCA to a single card (SDHC only).

Once we get everything in place at the mmc bus level, we can proceed as
documented in the GSoC project [third phase outline](https://www.haiku-os.org/blog/krish_iyer/2018-07-12_gsoc_2018_sdhci_mmc_driver_third_phase_plan/).

If you find it difficult to understand the driver development and it's
functioning and role, please refer *docs/develop/kernel/device_manager_introduction.html*
