Haiku PCI Driver Development Under QEMU on Linux
====================================================

Developing Haiku drivers for PCI and PCI Express cards is now a lot easier
given advancements in IOMMU under Linux. You can effectively detach PCI cards
from their host operating system and attach them to guest VM's resulting in
true hardware emulation. In this guide we will be configuring a secondary
graphics card to be attached to a Haiku virtual machine.

**Warning**: Any device attached to a VM will be unavailable to the host operating
system. This means you **cannot** use your primary graphics card, network device,
etc within a VM and the host operating system at the same time. In this
example, we have two graphics cards installed in the Linux system.

IOMMU Setup
-----------------------
You will need to have IOMMU hardware support on your motherboard for this
to function. Most modern AMD A3 socket chips and Intel i3/i5/i7 devices
have IOMMU built in. If your board does indeed have IOMMU, you will likely
need to enable IOMMU within the bios of your motherboard before proceeding.

Linux Setup
-----------------------
Now that you have an IOMMU enabled system, you will need to tell Linux to
fully utilize IOMMU. To do this, you will need to add a few kernel boot
parameters. Depending on how your system is configured, there may be a few
places to do this. Here are some example config files:

 * `/etc/default/grub` (`GRUB_CMDLINE_LINUX_DEFAULT`)
 * `/boot/grub/grub.cfg`
 * `/boot/grub/menu.lst`
 * `/boot/refind_linux.conf`

Enabling OS support IOMMU for IOMMU involves adding one of the following
kernel boot parameters:

**AMD:**
```
iommu=pt iommu=1
```
**Intel:**
```
intel_iommu=on
```

Now, all we need to do is to reserve the PCI device. We want to make sure
no host drivers attempt to attach to the PCI device in question.

First we need to find the PCIID for the device in question. We can find
this through lcpci. Running lspci shows a bunch of devices. I've identified
this device as my target:

```
07:00.0 VGA compatible controller: Advanced Micro Devices, Inc. [AMD/ATI] Cedar [Radeon HD 5000]
```

Now, to get the PCI ID, I run lspci again with the -n flag (lspci -n). We find
the matching BUS ID and we get our PCI ID:

```
07:00.0 0300: 1002:68f9
```

Now that we have our target PCI ID (`1002:68f9`), we can bind this device to
a special pci-stub driver.

We will create two files for this graphics card:

**`/lib/modprobe.d/pci-stub.conf`:**
```
options pci-stub ids=1002:68f9
```

**`/lib/modprobe.d/drm.conf`:**
```
softdep drm pre: pci-stub
```

The first line tells the pci-stub driver to bind to the device in question.
The second line tells DRM (graphics driver stack) that it should make sure
pci-stub loads before DRM (ensuring the device is stubbed and not loaded by
DRM).

Now we reboot and cross our fingers.

On my AMD Linux system, we can see that IOMMU is active and functional:

```
kallisti5@eris ~ $ dmesg | grep AMD-Vi
[    0.119400] [Firmware Bug]: AMD-Vi: IOAPIC[9] not in IVRS table
[    0.119406] [Firmware Bug]: AMD-Vi: IOAPIC[10] not in IVRS table
[    0.119409] [Firmware Bug]: AMD-Vi: No southbridge IOAPIC found
[    0.119412] AMD-Vi: Disabling interrupt remapping
[    1.823122] AMD-Vi: Found IOMMU at 0000:00:00.2 cap 0x40
[    1.823253] AMD-Vi: Initialized for Passthrough Mode
```

And checking for pci-stub we can see it successfully took over my graphics card:
```
kallisti5@eris ~ $ dmesg | grep pci-stub
[    3.685970] pci-stub: add 1002:68F9 sub=FFFFFFFF:FFFFFFFF cls=00000000/00000000
[    3.686002] pci-stub 0000:07:00.0: claimed by stub
```

On every boot, the device will be available for attachment to VM's
Now, we simply attach the device to a VM:

```
sudo qemu-system-x86_64 --enable-kvm -hda haiku-nightly-anyboot.image -m 2048 -device pci-assign,host=07:00.0
```

If you experience any problems, try looking at kvm messages:
```
kallisti5@eris ~ $ dmesg | grep kvm
```

If you're doing this for a graphics card generally the qemu window will
lock up at the bootsplash and the video will appear on the second window.
Click the qemu window to control the Haiku machine.

If things go well you will see:
```
[ 1966.132176] kvm: Nested Virtualization enabled
[ 1966.132185] kvm: Nested Paging enabled
[ 1972.212231] pci-stub 0000:07:00.0: kvm assign device
[ 1974.186382] kvm: zapping shadow pages for mmio generation wraparound
```
