The SPARC port
##############

The SPARC port targets various machines from Sun product lineup. The initial effort is on the
Ultra 60 and Ultra 5, with plans to latter add the Sun T5120 and its newer CPU. This may change
depending on hardware donations and developer interest.

Support for 32-bit versions of SPARC is currently not planned.

SPARC ABI
=========

The SPARC architecture has 32 integer registers, divided as follows:

- global registers (g0-g7)
- input (i0-i7)
- local (l0-l7)
- output (o0-o7)

Parameter passing and return is done using the output registers, which are
generally considered scratch registers and can be corrupted by the callee. The
caller must take care of preserving them.

The input and local registers are callee-saved, but we have hardware assistance
in the form of a register window. There is an instruction to shift the registers
so that:

- o registers become i registers
- local and output registers are replaced with fresh sets, for use by the
  current function
- global registers are not affected

Note that as a side-effect, o7 is moved to i7, this is convenient because these
are usually the stack and frame pointers, respectively. So basically this sets
the frame pointer for free.

Simple enough functions may end up using just the o registers, in that case
nothing special is necessary, of course.

When shifting the register window, the extra registers come from the register
stack in the CPU. This is not infinite, however, most implementations of SPARC
will only have 8 windows available. When the internal stack is full, an overflow
trap is raised, and the handler must free up old windows by storing them on the
stack, likewise, when the internal stack is empty, an underflow trap must fill
it back from the stack-saved data.

Misaligned memory access
========================

The SPARC CPU is not designed to gracefully handle misaligned accesses.
You can access a single byte at any address, but 16-bit access only at even
addresses, 32bit access at multiple of 4 addresses, etc.

For example, on x86, such accesses are not a problem, it is allowed and handled
directly by the instructions doing the access. So there is no performance cost.

On SPARC, however, such accesses will cause a SIGBUS. This means a trap handler
has to catch the misaligned access and do it in software, byte by byte, then
give back control to the application. This is, of course, very slow, so we
should avoid it when possible.

Fortunately, gcc knows about this, and will normally do the right thing:

- For usual variables and structures, it will make sure to lay them out so that
  they are aligned. It relies on stack alignment, as well as malloc returning
  sufficiently aligned memory (as required by the C standard).
- On packed structure, gcc knows the data is misaligned, and will automatically
  use the appropriate way to access it (most likely, byte-by-byte).

This leaves us with two undesirable cases:

- Pointer arithmetics and casting. When computing addresses manually, it's
  possible to generate a misaligned address and cast it to a type with a wider
  alignment requirement. In this case, gcc may access the pointer using a
  multi byte instruction and cause a SIGBUS. Solution: make sure the struct
  is aligned, or declare it as packed so unaligned access are used instead.
- Access to hardware: it is a common pattern to declare a struct as packed,
  and map it to hardware registers. If the alignment isn't known, gcc will use
  byte by byte access. It seems volatile would cause gcc to use the proper way
  to access the struct, assuming that a volatile value is necessarily
  aligned as it should.

In the end, we just need to be careful about pointer math resulting in unalined
access. -Wcast-align helps with that, but it also raises a lot of false positives
(where the alignment is preserved even when casting to other types). So we
enable it only as a warning for now. We will need to ceck the sigbus handler to
identify places where we do a lot of misaligned accesses that trigger it, and
rework the code as needed. But in general, except for these cases, we're fine.

The Ultrasparc MMUs
============================

First, a word of warning: the MMU was different in SPARCv8 (32bit)
implementations, and it was changed again on newer CPUs.

The Ultrasparc-II we are supporting for now is documented in the Ultrasparc
user manual. There were some minor changes in the Ultrasparc-III to accomodate
larger physical addresses. This was then standardized as JPS1, and Fujitsu
also implemented it.

Later on, the design was changed again, for example Ultrasparc T2 (UA2005
architecture) uses a different data structure format to enlarge, again, the
physical and virtual address tags.

For now te implementation is focused on Ultrasparc-II because that's what I
have at hand, later on we will need support for the more recent systems.

Ultrasparc-II MMU
-----------------

There are actually two separate units for the instruction and data address
spaces, known as I-MMU and D-MMU. They each implement a TLB (translation
lookaside buffer) for the recently accessed pages.

This is pretty much all there is to the MMU hardware. No hardware page table
walk is provided. However, there is some support for implementing a TSB
(Translation Storage Buffer) in the form of providing a way to compute an
address into that buffer where the data for a missing page could be.

It is up to software to manage the TSB (globally or per-process) and in general
keep track of the mappings. This means we are relatively free to manage things
however we want, as long as eventually we can feed the iTLB and dTLB with the
relevant data from the MMU trap handler.

To make sure we can handle the fault without recursing, we need to pin a few
items in place:

In the TLB:

- TLB miss handler code
- TSB and any linked data that the TLB miss handler may need
- asynchronous trap handlers and data

In the TSB:

- TSB-miss handling code
- Interrupt handlers code and data

So, from a given virtual address (assuming we are using only 8K pages and a
512 entry TSB to keep things simple):

VA63-44 are unused and must be a sign extension of bit 43
VA43-22 are the 'tag' used to match a TSB entry with a virtual address
VA21-13 are the offset in the TSB at which to find a candidate entry
VA12-0 are the offset in the 8K page, and used to form PA12-0 for the access

Inside the TLBs, VA63-13 is stored, so there can be multiple entries matching
the same tag active at the same time, even when there is only one in the TSB.
The entries are rotated using a simple LRU scheme, unless they are locked of
course. Be careful to not fill a TLB with only locked entries! Also one must
take care of not inserting a new mapping for a given VA without first removing
any possible previous one (no need to worry about this when handling a TLB
miss however, as in that case we obviously know that there was no previous
entry).

Entries also have a "context". This could for example be mapped to the process
ID, allowing to easily clear all entries related to a specific context.

TSB entries format
------------------

Each entry is composed of two 64bit values: "Tag" and "Data". The data uses the
same format as the TLB entries, however the tag is different.

They are as follow:

Tag
***

Bit 63: 'G' indicating a global entry, the context should be ignored.
Bits 60-48: context ID (13 bits)
Bits 41-0: VA63-22 as the 'tag' to identify this entry

Data
****

Bit 63: 'V' indicating a valid entry, if it's 0 the entry is unused.
Bits 62-61: size: 8K, 64K, 512K, 4MB
Bit 60: NFO, indicating No Fault Only
Bit 59: Invert Endianness of accesses to this page
Bits 58-50: reserved for use by software
Bits 49-41: reserved for diagnostics
Bits 40-13: Physical Address<40-13>
Bits 12-7: reserved for use by software
Bit 6: Lock in TLB
Bit 5: Cachable physical
Bit 4: Cachable virtual
Bit 3: Access has side effects (HW is mapped here, or DMA shared RAM)
Bit 2: Privileged
Bit 1: Writable
Bit 0: Global

TLB internal tag
****************

Bits 63-13: VA<63-13>
Bits 12-0: context ID

Conveniently, a 512 entries TSB fits exactly in a 8K page, so it can be locked
in the TLB with a single entry there. However, it may be a wise idea to instead
map 64K (or more) of RAM locked as a single entry for all the things that needs
to be accessed by the TLB miss trap handler, so we minimize the use of TLB
entries.

Likewise, it may be useful to use 64K pages instead of 8K whenever possible.
The hardware provides some support for mixing the two sizes but it makes things
a bit more complex. Let's start out with simpler things.

Software floating-point support
===============================

The SPARC instruction set specifies instruction for handling long double
values, however, no hardware implementation actually provides them. They
generate a trap, which is expected to be handled by the softfloat library.

Since traps are slow, and gcc knows better, it will never generate those
instructions. Instead it directly calls into the C library, to functions
specified in the ABI and used to do long double math using softfloats.

The support code for this is, in our case, compiled into both the kernel and
libroot. It lives in src/system/libroot/os/arch/sparc/softfloat.c (and other
support files). This code was extracted from FreeBSD, rather than the glibc,
because that made it much easier to get it building in the kernel.

Openboot bootloader
===================

Openboot is Sun's implementation of Open Firmware. So we should be able to share
a lot of code with the PowerPC port. There are some differences however.

Executable format
-----------------

PowerPC uses COFF. Sparc uses a.out, which is a lot simpler. According to the
spec, some fields should be zeroed out, but they say implementation may chose
to allow other values, so a standard a.out file works as well.

It used to be possible to generate one with objcopy, but support was removed,
so we now use elf2aout (imported from FreeBSD).

The file is first loaded at 4000, then relocated to its load address (we use
202000 and executed there)

Openfirmware prompt
-------------------

To get the prompt on display, use STOP+A at boot until you get the "ok" prompt.
On some machines, if no keyboard is detected, the ROM will assume it is set up
in headless mode, and will expect a BREAK+A on the serial port.

STOP+N resets all variables to default values (in case you messed up input or
output, for example).

Useful commands
---------------

Disable autoboot to get to the openboot prompt and stop there

.. code-block:: text

   setenv auto-boot? false

Configuring for keyboard/framebuffer io

.. code-block:: text

   setenv screen-#columns 160
   setenv screen-#rows 49
   setenv output-device screen:r1920x1080x60
   setenv input-device keyboard

Configuring openboot for serial port

.. code-block:: text

   setenv ttya-mode 38400,8,n,1,-
   setenv output-device ttya
   setenv input-device ttya
   reset

Boot from network
-----------------

static ip
*********

This currently works best, because rarp does not let the called binary know the
IP address. We need the IP address if we want to mount the root filesystem using
remote_disk server.

.. code-block:: text

    boot net:192.168.1.2,somefile,192.168.1.89

The first IP is the server from which to download (using TFTP), the second is
the client IP to use. Once the bootloader starts, it will detect that it is
booted from network and look for a the remote_disk_server on the same machine.

rarp
****

This needs a reverse ARP server (easy to setup on any Linux system). You need
to list the MAC address of the SPARC machine in /etc/ethers on the server. The
machine will get its IP, and will use TFTP to the server which replied, to get
the boot file from there.

.. code-block:: text

    boot net:,somefile

(net is an alias to the network card and also sets the load address: /pci@1f,4000/network@1,1)

dhcp
****

This needs a DHCP/BOOTP server configured to send the info about where to find
the file to load and boot.

.. code-block:: text

    boot net:dhcp



Debugging
---------

.. code-block:: text

   202000 dis (disassemble starting at 202000 until next return instruction)
   4000 1000 dump (dump 1000 bytes from address 4000)
   .registers (show global registers)
   .locals (show local/windowed registers)
   %pc dis (disassemble code being exectuted)
   ctrace (backtrace)
