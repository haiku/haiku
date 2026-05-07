SDHCI MMC Driver
================

The MMC bus is a standardized interface for mass storage and other devices. Its most visible use
is in SD cards, but there are other peripherals based on it, such as SDIO (some Wifi controllers
and other devices), and eMMC (embedded mass storage, found often on cheap netbooks and ARM based
computers).

The MMC stack on Haiku aims to provide generic support for these devices. To achieve this, it is
split in three layers:

- At the lowest level, bus drivers implement the basic operations such as sending and receiving
  commands, handling interrupts, and recovering from bus errors. Currently, there is a single
  driver for hot controllers that follow the SDHCI specification: PCI devices with class 8 and
  subclass 5, or equivalent controllers discovered through ACPI. Other controllers may be added
  to this driver, if they are similar enough, or implemented as separate drivers if their register
  structure is different.
- At the higher level, drivers implement functionality for specific type of cards. At the moment
  the single driver is mmc_disk, handling all SD, SDHC, MMC and eMMC cards. Separate drivers can
  be added for SDIO devices if someone encounters one.
- In between these two, a bus manager handles the card enumeration and initialization. It makes
  sure each card is assigned an RCA, a 16-bit address that can then be used to send commands to
  a specific device on the bus.

The bus standard was initially developed for MMC cards. Some disagreement in the design group
led to the creation of two standards, one for MMC and one for SD cards. The communication bus
remains compatible, but the commands are a bit different, including during the initialization
process. MMC cards had almost disappeared, but made a come back in the form of eMMC, which is
essentially an MMC chip soldered directly to a motherboard, not installed in a removable card.
As a result, both the mmc_disk driver and the bus manager have to handle some of the differences
between MMC and SD devices.

Reference documentation
-----------------------

The initial development for these drivers was done as a Google Summer of Code project in 2018.
The corresponding `weekly reports <https://www.haiku-os.org/blog/krish_iyer>`__ provide furhter
details about this initial work.

Both SD and MMC standards provide reference documentation which was used as a base for the driver.
This was then completed by testing and experimenting on real hardware, as the specifications are
never detailed enough to ensure a fully working driver. The relevant documents are:

- `Part 1: Physical Layer Simplified Specification Version 1.10 <https://www.sdcard.org/downloads/pls/pdf/index.php?p=Part1_Physical_Layer_Simplified_Specification_Ver1.10.jpg&f=Part1_Physical_Layer_Simplified_Specification_Ver1.10.pdf&e=EN_P1110>`__.
- `Part A2: SD Host Controller Simplified Specification Version 1.00 <https://www.sdcard.org/downloads/pls/pdf/index.php?p=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.jpg&f=PartA2_SD_Host_Controller_Simplified_Specification_Ver1.00.pdf&e=EN_A2100>`__
- MMCA Committee: The MultiMedia Card system specification, version 3.31
- JEDEC JESD84-B50: Embedded Multi-Media Card (e.MMC) Electrical Standard, version 5.0
- Samsung Electronics: MultiMediaCardSpecification version 0.9
- Documentation from other manufacturers of controllers and SD/MMC cards when available

Later versions of these documents can be referred as well, they add new features for example to
allow higher capacity cards and faster transfer rates. The driver was written initially with the
earliest version of the specification to ensure the largest compatibility possible with host
controllers, and avoid having to detect and optionally enable all the new features.

Loading and testing the driver
------------------------------

Emulating the hardware
~~~~~~~~~~~~~~~~~~~~~~

QEMU allows to emulate both SDHC and eMMC devices with an SDHCI controller.
This also offers tracing of the received commands to make sure the driver is processing them as
expected. On real hardware, tracing is only available from the Haiku software side of things,
which may make investigating some issues more difficult.

::

   qemu-img create sd-card.img 32M
   qemu-system-x86_64 -drive index=0,file=haiku-nightly-anyboot.iso,format=raw \
       -device sdhci-pci -device sd-card,drive=mydrive \
       -drive if=sd,index=1,file=sd-card.img,format=raw,id=mydrive
       -m 512M -enable-kvm -usbdevice tablet -machine q35

This does the following:

- Create an SD card image of 32MB
- Run qemu with a bootable image in an IDE disk, and an SDHCI bus with an SD card in it
- Have enough memory to boot Haiku, use KVM mode for speed, and a tablet for ease of use
- Use the Q35 chipset so the mouse and SDHCI controllers don’t share an interrupt (not strictly
  required, but it avoids calls to the SDHCI interrupt handler on every mouse move).

Tracing of SD operations can also be added to see how qemu is interpreting our commands:

::

   -trace sdhci* -trace sdbus* -trace sdcard*

Testing and loading the driver
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

The code for SD and SDHC support is merged and part of the default Haiku build.

MMC support is not enabled yet in the mmc_disk driver. Get `the corresponding change <https://review.haiku-os.org/c/haiku/+/10928>`__
in your build if you want to test MMC and eMMC support.

Insight into the code and future tasks
--------------------------------------

MMC Bus drivers (src/add-ons/kernel/busses/mmc)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The bus driver provides the low level aspects: interrupts management,
DMA transfer, accessing the hardware registers. It acts as a platform
abstraction layer so that the bus manager and disk driver can be written
independently of the underlying hardware.

Currently there is a single implementation for SDHCI (MMC bus over PCI).
Later on, other drivers will be added for other ways to access the MMC
bus (for example on ARM devices where it does not live on a PCI bus, and
may have a different register layout).

For this reason, the bus drivers should only do the most low-level
things, trying to keep as much code as possible in the upper layers.

One slightly confusing thing about SDHCI is that it allows a single PCI
device to implement multiple separate MMC busses (each of which could
have multiple devices attached). For this reason there is an SDHCI
“device” that attaches to the PCI device node for the controller, and
then publishes multiple device nodes for each available bus. The nodes
then work independently of each other. This setup seems pretty uncommon
in real hardware, however.

The Bus Manager (src/add-ons/kernel/bus_managers/mmc)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

The bus manager is responsible for enumerating devices on the bus,
assigning them addresses, and keeping track of which card is active at
any given time.

Essentially it has everything that requires collaboration between
multiple MMC devices, as well as things that are not specific to a
device type (common to SDIO, SD and MMC cards, for example)

Disk Driver (src/add-ons/kernel/drivers/disk/mmc)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This is a mass storage driver for MMC, SD and SDHC cards. Currently only
SD and SDHC are tested, MMC and eMMC will have to be added (they are
similar but there are some differences).

Wiring the driver in the device manager (src/system/kernel/device_manager/device_manager.cpp)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

(note: possibly not accurate documentation, I did not check how things
in the device manager are actually implemented, but this is my
understanding of it).

The device manager attempts to implement lazy, on-demand scanning of the
devices. The idea is to speed up booting by not spending a lot of time
scanning everything first, and only scanning small parts of the device
tree as they are needed.

The trigger is accesses to the devfs. For example, when an application
opens /dev/disk, the device manager will start looking for disks so it
can populate it. This means the device manager needs to know which
branches of the device tree to explore. Currently this knowledge is
hardcoded into the device tree sourcecode, and there’s a TODO item about
moving that knowledge to drivers instead. But it’s tricky, since the
whole point is to avoid loading all the drivers.

Anyway, currently, the device manager is hardcoded to look for mass
storage devices under SDHCI busses, both standard ones and some
non-standard ones (for example, Ricoh provides SDHCI implenmentations
that are conform to the spec, except they don’t have the right device
type in the PCI registers).

Insight into the code
~~~~~~~~~~~~~~~~~~~~~

MMC Bus management overview
^^^^^^^^^^^^^^^^^^^^^^^^^^^

The device tree for MMC support looks like this:

-  PCI bus manager

   -  (other PCI devices)
   -  SDHCI controller

      -  SDHCI bus

         -  MMC bus manager

            -  MMC device

               -  mmc_disk device

            -  MMC device

               -  (other SDIO driver)

         -  MMC bus manager (second MMC bus)

            -  MMC device

               -  mmc_disk device

At the first level, the PCI bus manager publishes a device node for each
device found. One of them is our SDHCI controller, identified either by
the PCI device class and subclass, or for not completely SDHCI
compatible device, by the device and vendor IDs.

The SDHCI bus driver attaches to this device and publishes his own node.
It then scans the device and publishes an MMC bus node for each slot
(there may be multiple SD slots attached to a single PCI controller).

The MMC bus manager then attach to each of these slots, and send the
appropriate commands for enumerating the SD cards (there may be multiple
cards in a “slot”), and publishes a device node for each of them.
Finally, the mmc_disk driver can bind itself to one of these device
nodes, and publish the corresponding disk node, which is also be made
available in /dev/disk/mmc.

Currently the mmc bus does not publish anything in the devfs, but this
could be added if sending raw SD/MMC commands to SD cards from userland
is considered desirable.

SDHCI driver
^^^^^^^^^^^^

The SDHCI driver is the lowest level of the MMC stack. It provides
abstraction of the SDHCI device. Later on, different way to access an SD
bus may be added, for example for ARM devices which decided to use a
different register interface.

The entry point is as usual **supports_device()**. This method is called
only for devices which may be SDHCI controllers, thanks to filtering
done in the device manager to probe only the relevant devices. The
probing is done on-demand, currently when the system is enumerating
/dev/disk in the devfs. Later on, when we have SDIO support, probing
will also be triggered in other cases.

The function identifies the device by checking the class and subclass,
as well as a limited set of hardcoded PCI device and vendor IDs for
devices that do not use the assigned subclass.

Once a compatible device is found, **register_child_devices()** is used
to publish device nodes for each slot to be controlled by the mmc bus
manager. The registers for each device are mapped into virtual memory,
using the information from the PCI bar registers. **struct registers**
is defined so that it matches the register layout, and provide a little
abstraction to raw register access.

An SdhciBus object is created to manage each of these busses at the
SDHCI level. It will be responsible for executing SD commands on that
bus, and dealing with the resulting interrupts.

The Bus Manager
^^^^^^^^^^^^^^^

The MMC bus manager manages the MMC bus (duh). Its tasks are:

-  enumerating SD cards on the bus
-  assigning RCAs to the cards for identifying them when sending
   commands
-  setting the bus clock speed according to what the cards can handle
-  remember which SD card is currently active (CMD7)
-  manage cards state
-  publish device nodes for each card

Disk Driver
^^^^^^^^^^^

The disk driver is attached to devices implementing SDSC or SDHC/SDXC
commands. There will be other drivers for non-storage (SDIO) cards.

To help with this, the MMC bus manager provides the device with the
information it gathered while initializing the device. According to the
commands recognized by the card during the initialization sequence, it’s
possible to know if it’s SDSC, SDHC/SDXC, or something else (SDIO,
legacy MMC, etc).

The disk driver publishes devfs entries in /dev/disk/mmc and implements
the usual interface for disk devices. From this point on, the device can
be used just like any other mass storage device.

Tasks to be completed
~~~~~~~~~~~~~~~~~~~~~

The SDHCI driver is able to send and receive commands. However it does
not handle card insertion and removal interrupts yet, so the card must
be already inserted when the driver is loaded.

The mmc_disk driver is complete and working, but was not tested for MMC
and eMMC devices. Some changes may be needed.

There is also work to be done for better performance: making sure we
switch to the high-speed clock when an SD card supports it, and use the
8-bit data transfer mode for eMMC when possible.

Drivers for SDIO devices should also be added. The mmc_bus and SDHCI
drivers have been tested only with one card on the bus at a time (for
lack of hardware allowing more complex setups).

If you find it difficult to understand the driver development and it’s
functioning and role, please refer *docs/develop/kernel/device_manager_introduction.html*
