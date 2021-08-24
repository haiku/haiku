Haiku boot process specification
================================

Creation Date: November 23, 2002
Version: 2.0 (Jan 22, 2021)
Status: documenting the current state of things
Author(s): Axel Dörfler, Adrien Destugues
                                               

Overview
--------

Unlike other systems, Haiku comes with its own user-friendly bootloader.
The main task of the bootloader is to load and start the kernel. We
don't have a concept of an initramfs as Linux does, instead our
bootloader is able to find the kernel and modules in a BFS partition,
and even extract them from packages as needed. It also provides an early
boot menu that can be used to change settings, boot older versions of
Haiku that were snapshotted by the package system, and write boot logs
to USB mass storage.

Booting from BIOS
-----------------

Haiku BIOS boot loader process is split into 3 different stages. Since
the second stage is bound tightly to both other stages (which are
independent from each other), it is referred to as stage 1.5, whereas
the other stages are referred to as stage 1 and 2. This architecture is
used because the BIOS booting process only loads a very small piece of
code from disk for booting, insufficient for the needs outlined above.

The following will explain all stages in detail.

Stage 1
~~~~~~~

The first stage is responsible for loading the real boot loader from a
BFS disk. It will be loaded by the Master Boot Record (MBR) and executed
in the x86 real mode. It is only used if the system will be booted
directly from a BFS partition, it won't be used at all if it is booted
from a floppy disk or CD-ROM (in this case, stage 1.5 is in charge
immediately).

| It resides in the first 1024 bytes of a BFS disk which usually refers
  to the first two sectors of the partition in question. Since the BFS
  superblock is located at byte offset 512, and about 170 bytes large,
  this section is already reserved, and thus cannot be used by the
  loader itself.
| The MBR only loads the first sector of a partition into memory, so it
  has to load the superblock (and the rest of its implementation) by
  itself.

| The loader must be able to load the real boot loader from a certain
  path, and execute it. In BeOS this boot loader would be in
  "/boot/beos/system/zbeos", in Haiku this is haiku_loader.bios_ia32
  found in the haiku_loader package.
| Theoretically, it is enough to load the first few blocks from the
  loader, and let the next stage then load the whole thing (which it has
  to do anyway if it has been written on a floppy). This would be one
  possible optimization if the 850 bytes of space are filled too early,
  but would require that "zbeos" is written in one sequential block
  (which should be always the case anyway).

haiku_loader.bios_ia32
~~~~~~~~~~~~~~~~~~~~~~

Contains both the stage 1.5 boot loader, and the compressed stage 2
loader. It's not an ELF executable file; i.e. it can be directly written
to a floppy disk which would cause the BIOS to load the first 512 bytes
of that file and execute it.

Therefore, it will start with the stage 1.5 boot loader which will be
loaded either by the BIOS when it directly resides on the disk (for
example when loaded from a floppy disk), or the stage 1 boot loader,
although this one could have a different entry point than the BIOS.

Stage 1.5
~~~~~~~~~

Will have to load the rest of haiku_loader into memory (if not already
done by the stage 1 loader in case it has been loaded from a BFS disk),
set up the global descriptor table, switch to x86 protected mode,
uncompress stage 2, and execute it.

This part is very similar to the stage 1 boot loader from NewOS.

Stage 2
~~~~~~~

This is the most complex part of the boot loader. In short, it has to
load any modules and devices the kernel needs to access the boot device,
set up the system, load the kernel, and execute it.

The kernel, and the modules and drivers needed are loaded from the boot
disk - therefore the loader has to be able to access BFS disks. It also
has to be able to load and parse the settings of these drivers (and the
kernel) from the boot disk, some of them are already important for the
boot loader itself (like "don't call the BIOS"). Since this stage is
already executed in protected mode, it has to use the virtual-86 mode to
call the BIOS and access any disk.

Before loading those files from the boot disk, it should look for
additional files located on a specific disk location after the "zbeos"
file (on floppy disk or CD-ROM). This way, it could access disks that
cannot be accessed by the BIOS itself.

Setting up the system for the kernel also means initalizing PCI devices
needed during the boot process before the kernel is up. It must be able
to do so since the BIOS might not have set up those devices correctly or
at all.

It also must calculate a check sum for the boot device which the kernel
can then use to identify the boot volume and partition with - there is
no other reliable way to map BIOS disk IDs to the /dev/disk/... tree the
system itself is using.

After having loaded and relocated the kernel, it executes it by passing
a special structure which tells the kernel things like the boot device
check sum, which modules are already loaded and where they are.

The stage 2 boot loader also includes user interaction. If the user
presses a special key during the boot process (like the space key, or
some others as well), a menu will be presented where the user can select
the boot device (if several, the loader has to scan for options), safe
mode options, VESA mode, etc.

This menu may also come up if an error occured during the execution of
the stage 2 loader.

Open Firmware
-------------

On Open Firmware based systems, there is no need for a stage 1.5 because
the firmware does not give us as many constraints. Instead, the stage 2
is loaded directly by the firmware. This requires converting the
haiku_loader executable to the appropriate executable format (a.out on
sparc, pef on powerpc). The conversion is done using custom tools
because binutils does not support these formats anymore.

There is no notion of real and protected mode on non-x86 architectures,
and the bootloader is able to easily call Open Firmware methods to
perform most tasks (disk access, network booting, setting up the
framebuffer) in a largely hardware-independent way.

U-Boot
------

U-Boot is able to load the stage2 loader directly from an ELF file.
However, it does not provide any other features. It is not possible for
the bootloader to call into U-Boot APIs for disk access, displaying
messages on screen etc (while possible in theory, these features are
often disabled in U-Boot). This means haiku_loader would need to parse
the FDT (describing the available hardware) and bundle its own drivers
for using the hardware. This approach is not easy to set up, and it is
recommended to instead use the UEFI support in U-Boot where possible.

EFI
---

On EFI systems, there is no need for a stage1 loader as there is for
BIOS. Instead, our stage2 loader (haiku_loader) can be executed directly
from the EFI firmware.

The EFI firmware only knows how to run executables in the PE format (as
used by Windows) because Microsoft was involved in specifying it. On
x86_64, we can use binutils to output a PE file directly. But on other
platforms, this is not supported by binutils. So, what we do is generate
a "fake" PE header and wrap our elf file inside it. The bootloader then
parses the embedded ELF header and relocates itself, so the other parts
of the code can be run.

After this initial loading phase, the process is very similar to the
Open Firmware one. EFI provides us with all the tools we need to do disk
access and both text mode and framebuffer output.
