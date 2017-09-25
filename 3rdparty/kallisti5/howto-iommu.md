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
fully utilize IOMMU and reserve the PCI cards for IOMMU use.

Now, all we need to do is to reserve the PCI device. We want to make sure
no host drivers attempt to attach to the PCI device in question.

First we need to find the PCIID for the device in question. We can find
this through lcpci. Running lspci shows a bunch of devices. I've identified
this device as my target:

```
$ lspci -nn | egrep "VGA|Audio"
28:00.0 VGA compatible controller [0300]: Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 470/480/570/580] [1002:67df] (rev c7)
28:00.1 Audio device [0403]: Advanced Micro Devices, Inc. [AMD/ATI] Ellesmere [Radeon RX 580] [1002:aaf0]

29:00.0 VGA compatible controller [0300]: Advanced Micro Devices, Inc. [AMD/ATI] Redwood XT GL [FirePro V4800] [1002:68c8]
29:00.1 Audio device [0403]: Advanced Micro Devices, Inc. [AMD/ATI] Redwood HDMI Audio [Radeon HD 5000 Series] [1002:aa60]

2b:00.3 Audio device [0403]: Advanced Micro Devices, Inc. [AMD] Device [1022:1457]
```

Now that we have our target PCI IDs (in this case, `1002:68c8,1002:aa60`), we can bind this device to
the vfio-pci driver.

**vfio-pci module**

If your distro ships vfio-pci as a module, you will need to add the vfio drivers to the initial ramdisk
to leverage them as early as possible in the boot process.

Below is an example on Fedora:

```
echo 'add_drivers+="vfio vfio_iommu_type1 vfio_pci vfio_virqfd"' > /etc/dracut.conf.d/vfio.conf
dracut -f --kver `uname -r`
```

**vfio reservation**

Now that the requirements are met, we can attach the target GPU to the vfio driver using the information
we have collected so far.

Below, i've leveraged the PCI ID's collected above, and provided them to the vfio-pci driver via the kernel
parameters at boot.

> Be sure to replace <CPU> with amd or intel depending on your system.

```
rd.driver.pre=vfio-pci vfio-pci.ids=1002:68c8,1002:aa60 <CPU>_iommu=on
```

> YMMV: These steps differ a lot between distros

On my EFI Fedora 26 system, I appended the line above to my /etc/sysconfig/grub config under GRUB_CMDLINE_LINUX.
Then, regenerated my grub.cfg via ```grub2-mkconfig -o /boot/efi/EFI/fedora/grub.cfg```


Attach GPU to VM
-----------------

Now we reboot and cross our fingers.

> If displays attached to the card are now black, then things are working as designed. If you see your
> desktop on the target GPU, the vfio driver didn't properly bind to your card and something was done
> incorrectly.

On my AMD Linux system, we can see that IOMMU is active and functional:

```
kallisti5@eris ~ $ dmesg | egrep "IOMMU|AMD-Vi"
[    0.650138] AMD-Vi: IOMMU performance counters supported
[    0.652201] AMD-Vi: Found IOMMU at 0000:00:00.2 cap 0x40
[    0.652201] AMD-Vi: Extended features (0xf77ef22294ada):
[    0.652204] AMD-Vi: Interrupt remapping enabled
[    0.652204] AMD-Vi: virtual APIC enabled
[    0.652312] AMD-Vi: Lazy IO/TLB flushing enabled
[    0.653841] perf/amd_iommu: Detected AMD IOMMU #0 (2 banks, 4 counters/bank).
[    4.114847] AMD IOMMUv2 driver by Joerg Roedel <jroedel@suse.de>

```

And checking for vfio we can see it successfully took over my graphics card:
```
kallisti5@eris ~ $ dmesg | grep vfio
[    3.928695] vfio-pci 0000:29:00.0: vgaarb: changed VGA decodes: olddecodes=io+mem,decodes=io+mem:owns=none
[    3.940222] vfio_pci: add [1002:68c8[ffff:ffff]] class 0x000000/00000000
[    3.952302] vfio_pci: add [1002:aa60[ffff:ffff]] class 0x000000/00000000
[   35.629861] vfio-pci 0000:29:00.0: enabling device (0000 -> 0003)
```

On every boot, the device will be available for attachment to VM's
Now, we simply attach the device to a VM:

```
sudo qemu-system-x86_64 --enable-kvm -hda haiku-nightly-anyboot.image -m 2048 -device pci-assign,host=29:00.0
```

If you're doing this for a graphics card generally the qemu window will
lock up at the bootsplash and the video will appear on the second window.
Click the qemu window to control the Haiku machine.
