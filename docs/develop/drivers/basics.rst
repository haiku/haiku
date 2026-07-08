Device driver basics
####################

There is a lot of gatekeeping about drivers and kernel development. It isn't as complicated as
some people would like you to think. The API is quite simple, and directly matches with how the
driver is used from the userspace caller side.

Difficulties lie in obtaining appropriate documentation for the hardware (sadly, some manufacturers
don't publish those), the relative lack of memory protection in the kernel, and the use of the
somewhat limited kernel debugger rather than a full-blown userspace one. Expect to spend a lot of
time testing things, rebooting your machine, and scratching your head as you try to figure it out.

Seems like a lot of fun, right? Now that I have convinced you that you should write drivers, let's
look at it into more details.

Entry points
============

Unlike userspace applications, the entry point for drivers (and kernel modules in general) is not
a main() function.

BeOS style drivers have 5 entry points that the kernel will call, as well as one global variable
that needs to be exported. There is a separate interface for "new style" drivers which works differently.

.. code-block:: cpp

  status_t init_hardware(void);
  status_t init_driver(void);
  status_t uninit_driver(void);
  const char** publish_devices(void);
  device_hooks* find_device(const char* name);
  int api_version;

init_hardware
  Called when the system boots. Must return B_OK if the driver detected some hardware it can handle.

init_driver
  Called to initialize the driver. Must return B_OK to confirm the driver is ready for use.

uninit_driver
  Called when the driver is about to be unloaded from memory.

publish_devices
  Returns an array of device names that the driver will export. The names are relative to the /dev directory.

find_device
  Returns the function pointers that will allow to operate on a device file. These correspond to the following operations on the
  device file: open, close, free, control, read, write, select, deselect, readv and writev.

api_version
  Populate with B_CUR_DRIVER_VERSION. This lets the kernel know this version of the interface is being used.

Building and testing your driver
================================

You can write your driver as standalone code, outside of the Haiku source tree. In that case, the
easiest option is to use the makefile engine. You will need to set the NAME ad SRCS variables in
your Makefile as usual. You also need to set the TYPE variable to DRIVER. The Makefile engine should
then take care of building things for you.

If you write your driver with the goal of integrating it in the main Haiku source tree, you should
instead set it up with Jam. Drivers are built using the KernelAddon rule.

Once built, a BeOS style driver should be installed in ~/config/add-ons/kernel/drivers/bin. A link
to it should be created under ~/config/add-ons/kernel/drivers/ (possibly in subdirectories) matching
where the driver is going to publish its devices in the devfs. For example, for a driver managing
devices in /dev/bus/usb/usbrawpci:

.. code-block:: sh

  ln -s ~/config/non-packaged/add-ons/kernel/drivers/bin/usbrawpci ~/config/non-packaged/add-ons/kernel/drivers/dev/bus/usb/usbrawpci

Debugging and testing
=====================

The best option, if you can set it up, is to have a serial port in your machine that the kernel can
use to output debug logs to another computer. Unfortunately, serial ports are mostly gone from modern
hardware, and the replacement solutions are quite a bit more complex to set up. If your setup allows
it, consider:

- Running Haiku in a virtual machine, accessing the hardware you're writing a driver for using USB
  or PCI passthrough. In addition to an emulated serial port, this allows to attach a debugger to
  the virtual machine and step through the kernel code if needed.
- Adding a serial port to your machine. This has to be done through a PCI or PCI-Express serial port
  card (USB to serial adapters are currently NOT supported for debugging use). For laptops, an
  ExpressCard to serial adapter is also an option.
- Some server grade motherboards include a "remote management" ethernet port, and an internal
  management console that allows redirecting the serial port data to ethernet and capturing it from
  there.

The next best option is to use a log file. For this, use the dprintf() function in your driver
to trace as many things as possible, then look at the output in /boot/system/var/log/syslog.
The dprintf function accepts parameters similar to printf (format string and values).

Other options would made this considerably simpler, but have not been implemented in Haiky yet:

- Support for sending the system logs over Ethernet instead of serial. A simple protocol not
  involving the entire IP stack could be used (such as broadcasting it over UDP with a fixed source
  IP address).
- Support for USB serial devices (or some subset of them) as output for the system log.
- Some machines support a special debug port, usually by repurposing an USB port as a serial one
  with a special adapter. Unfortunately, this is not always documented, and laptop manufacturers
  have not standardized a single type of adapter.

Drivers in Haiku will usually have a TRACE macro that can be enabled to produce extra logging (by
calling dprintf), or not (by ignoring its parameter). This allows to enable traces only for the
specific driver you are working on, while not being drowned in output from the other ones.

Synchronization primitives
==========================

See `the Synchronization primitives<https://www.haiku-os.org/docs/api/synchronization_primitives.html>`_
page in the public API documentation for information about the various way to synchronize code between
multiple threads, and more importantly, interrupts and driver threads.

Conclusion
==========

This is all there is to know about drivers in general!

The device hooks are used in various ways in each type of device. For example, for a network card,
userspace will use read() to receive network packets and write() to send them. The ioctl function
is often used to provide more specific, structured operations.

You can start your work using a `template driver <https://www.haiku-os.org/files/2002-05-18_mphipps_devdriver_template.zip>`_, so you don't have to write everything from scratch.

