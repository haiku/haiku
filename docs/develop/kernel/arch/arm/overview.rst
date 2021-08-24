The ARM port
============

Note: there are in fact two ports to the ARM architecture, one for 32-bit, and one for 64-bit
systems. They don't have a lot of shared code as the two architectures are very different from
one another.

ARM devices are very popular, and especially since the release of the Raspberry Pi, people have
been requesting that Haiku is ported to it. Unfortunately, limitations in the architecture itself
and the wide diversity of hardware have made this task more complicated, and progress has been
slow. For example, ARM has no standard like the PC is for x86, so concepts as basic as a system
timer, a bootloader, or a serial port, are different from one machine to another. The situation
has improved with the later generations, as more things were integrated in the CPU core, and u-boot
is now well established as the main bootloader for ARM devices.

Limitations
-----------

There will be no support for hardware using architectures older than ARMv5. There will probably be
no support for architectures before ARMv7, which require more work on the compiler and OS, for
example due to lack of atomic instructions.

Support for high vectors (interrupt vectors stored at the end of the memory space) is required.

Information about specific hardware targets
-------------------------------------------

Over the years, various possible ARM targets have been considered for the Haiku ARM port.
We have accumulated some notes and documentation on some of them.

.. toctree::

   /kernel/arch/arm/allwinner_a10
   /kernel/arch/arm/beagle
   /kernel/arch/arm/efikamx
   /kernel/arch/arm/ipaq
   /kernel/arch/arm/rpi1
   /kernel/arch/arm/rpi2

TODO list
---------

Fix pre-ARMv7 support
*********************

The ARM instruction set has evolved a lot over time, and we have to make a choice: use the oldest
versions of the instruction set gives us maximal compatibility, but at the cost of a large
performance hit on newer systems, as well as extra code being needed in the OS to compensate for
the missing instructions.

Currently the cross-tools are compiled to default to ARMv7, Cortex-A8, and
hardware floating point. This works around the missing atomic support, see
below. This should be done by setting the -mcpu,-march and -mfloat-abi
switches at build time, however, they aren't passed on to haikuporter
during the bootstrap build, leading to the ports failing to find the
gcc atomic ops again.

It seems this create other problems, mainly because the UEFI environment for ARM is not supposed to
handle floating point registers. So, the softfloat ABI should be used there instead. To be able
to build both "soft float" and "hard float" code, we need multilib support, see below.

Determine how to handle atomic functions on ARM
***********************************************

GCC inlines are not supported, since the instructionset is ill-equiped for
this on older (pre-ARMv7) architectures. We possibly have to do something
similar to the linux kernel helper functions for this....

On ARMv7 and later, this is not an issue. Not sure about ARMv6, we may get
it going there. ARMv5 definitely needs us to write some code, but is it
worth the trouble?

Fix multilib support
********************

ARM-targetting versions of gcc are usually built with multilib support, to
allow targetting architectures with or without FPU, and using either ARM
or Thumb instructions. This bascally means a different libgcc and libstdc++
are built for each combination.

The cross-tools can be built with multilib support. However, we do some
tricks to get a separate libgcc and libstdc++ for the kernel (without C++11
threads support, as that would not build in the kernel). Building this lib
is not done in a multilib-aware way, so you get one only for the default
arch/cpu/abi the compiler is targetting. This is good enough, as long as that
arch is the one we want to use for the kernel...

Later on, the bootstrap build of the native gcc compiler will fail, because
it tries to build its multilib library set by linking against the different
versions of libroot (with and without fpu, etc). We only build one libroot,
so this also fails.

The current version of the x86_64 compiler appears is using multilib (to build for both 32 and 64
bit targets) and is working fine, so it's possible that most of the issues in this area have
already been fixed.

Figure out how to get page flags (modified/accessed) and implement it
*********************************************************************

use unmapped/read-only mappings to trigger soft faults for tracking used/modified flags for ARMv5 and ARMv6

Fix serial port mapping
***********************

Currently kernel uses the haiku_loader identity
mapping for it, but this lives in user virtual address space...
(Need to not use identity mapping in haiku_loader but just
map_physical_memory() there too so it can be handed over without issues).

Seperate ARM architecture/System-On-Chip IP code
************************************************

The early work on the ARM port resulted in lots of board specific code being added to early stages
of the kernel. Ideally, this would not be needed, the kernel would manage to initialize itself
mostly in a platform independant way, and get the needed information from the FDT passed by the
bootloader. The difficulty is that on older ARM versions, even the interrupt controller and timers
can be different on each machine.

KDL disasm module
*****************

Currently it is not possible to disassemble code in the kernel debugger.

The `NetBSD disassembler <http://fxr.watson.org/fxr/source/arch/arm/arm/disassem.c?v=NETBSD>`_ could be ported and used for this.

Add KDL hangman to the boot image
*********************************

for more enjoyment during porting....

Userland
********

Even if KDL hangman is fun, users will want to run real applications someday.

Bootloader TODOs
****************

- Better handling of memory ranges. Currently no checks are done, and
  memory is assumed to be a single contiguous range, and the "input"
  ranges for mmu_init are setup, but never considered.
- Allocate the pagetable range using mmu_allocate() instead of identity
  mapping it. That way, there's a bit more flexibility in where to place
  it both physically and virtually. This will need a minor change on the
  kernel side too (in the early pagetable allocator).

Other resources
---------------

About flatenned device trees
****************************

* http://www.denx.de/wiki/U-Boot/UBootFdtInfo
* http://wiki.freebsd.org/FlattenedDeviceTree#Supporting_library_.28libfdt.29
* http://elinux.org/images/4/4e/Glikely-powerpc-porting-guide.pdf
* http://ols.fedoraproject.org/OLS/Reprints-2008/likely2-reprint.pdf
* http://www.bsdcan.org/2010/schedule/events/171.en.html
* http://www.devicetree.org/ (unofficial bindings)
* http://www.devicetree.org/Device_Tree_Usage
* http://elinux.org/Device_Trees

About openfirmware
******************

http://www.openfirmware.info/Bindings

About floating point numbers handling on ARM
********************************************

https://wiki.debian.org/ArmHardFloatPort/VfpComparison
