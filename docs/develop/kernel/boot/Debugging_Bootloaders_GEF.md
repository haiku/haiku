# Bootloader debugging with GEF

When Haiku's early boot process is experiencing unknown crashes or faults, it can be extremely
difficult to troubleshoot (especially when serial, video, or other i/o devices are non-functional)

It **is** possible to step through the boot of any architecture of Haiku in a debugger if the system
boots and the issue can be reproduced in qemu.

> This works for any architecture and is _extremely_ helpful to trouble early platforms. Linux or Mac OS
> are requirements. You need a full POSIX environment.

## Building Haiku

On most non-x86 platforms, you will need a "kernel" (haiku_loader) and an "initrd" (haiku_floppyboot).

For arm/arm64: ```jam -q @minimum-mmc```

## Launching Haiku in QEMU

In the example below, we will prepare Haiku arm in QEMU for debugging.

```
qemu-system-arm -M raspi2 -kernel haiku_loader.u-boot -initrd haiku-floppyboot.tgz.u-boot -serial stdio -m 2G -dtb rpi2.dtb -s -S
```

**Key Flags:**

  * **-s**
    * Shorthand for -gdb tcp::1234, i.e. open a gdbserver on TCP port 1234.
  * **-S**
    * Do not start CPU at startup (you must type 'c' in the monitor).

These simple flags will make qemu listen for a debugger connection on localhost:1234 and have the VM not start until you tell it to.

> In the example above, we are Emulating a Raspberry Pi 2, and using our Raspberry Pi 2 dtb. If you don't have a dtb for the machine
> you're emulating, you can dump qemu's internal dtb by adding ```-M dumpdtb=myboard.dtb``` to the end of your qemu command.

## Attaching GEF

[GEF](https://github.com/hugsy/gef) is an enhanced debugger which works extremely well for debugging code running in virtual machines.
It piggy-backs on gdb and offers a lot of valueable insight at a glance without requiring to know every gdb command.

Once GEF is installed, we can step through the process to attach gdb to qemu.

### Open gdb with our symbols.

First we run gdb pointed at our boot loader.  We use the native ELF binary as that seems to give gdb/gef the most accurate knowledge
of our symbols. (the haiku_loader.u-boot is wrapped by u-boot's mkimage, your milage may vary based on platform)

```gdb objects/haiku/arm/release/system/boot/u-boot/boot_loader_u-boot```

### Set the architecture

This may not be required, but re-enforces to gef/gdb that we're working on arm.

```set architecture arm```

### Connect to QEMU

Now we tell gdb/gef about out running (but paused) QEMU instance.

```gef-remote -q localhost:1234```

A successful connection should occur.

### Step into debugging

Before you begin execution, it's handy to set a *breakpoint*. A *breakpoint* tells gdb/gef where it should pause execution to begin
the debugging process. All of our bootloaders start in a ```start_gen``` function, so this is a good place to start.

```breakpoint start_gen```

Now that a breakpoint is defined, lets run the virtual machine.

In gef, type ```continue```.

If everything is working as expected, you should now be "paused" at the ```start_gen``` function (hopefully showing the C/C++ code).

Now, you have a few commands to leverage:

  * **step**
    * Take a single step forward and execute the code listed.
    * Does **not** step "into" functions, just over them getting the return from the code.
    * Alias: s
  * **stepi**
    * step forward "into" the next code.
    * If you're on a function it will enter the function and show the code executed.
  * **break**
    * add additional "breakpoints" where you can step through the code execution.
  * **continue**
    * Resume execution.
    * If you have no additional breakpoints the code will "go do what it's supposed to"
    * Alias: c
