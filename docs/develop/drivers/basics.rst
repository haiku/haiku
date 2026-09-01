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
  Returns the function pointers that will allow to operate on a device file. See the next section for details.

api_version
  Populate with B_CUR_DRIVER_VERSION. This lets the kernel know this version of the interface is being used.

Device hooks
============

Drivers expose themselves to userspace through device files. In Haiku, these are part of a special
filesystem called devfs, which is mounted in /dev. The device files are organized in subdirectories,
depending on the type of device. For example, mass storage devices are in /dev/disk/, serial ports
are in /dev/ports/, and so on. This makes it easy for applications to locate devices of a given type.

Whenever possible, your driver should publish devices in one of the existing directories and provide
the same interface as the other devices published there by other drivers. This makes sure applications
can interact with the different devices without having to handle special cases, and the drivers handle
all the required hardware abstraction. If you are writing the first driver for a new type of device,
you will need to decide by yourself what interface to use. In that case, try to consider not just the
device you are working with, but what similar devices may end up in the same directory later on.
Design the interface so that it can be used in a more general case, by similar but slightly different
devices using other drivers.

The hooks are defined in os/drivers/Drivers.h:

.. code-block:: cpp
   status_t open(const char* name. uint32 flags, void **cookie);
   status_t close(void* cookie);
   status_t free(void* cookie);
   status_t control(void* cookie, uint32 op, void* data, size_t len);
   status_t read(void* cookie, off_t position, void* data, size_t* numBytes);
   status_t write(void* cookie, off_t position, void* data, size_t* numBytes);
   status_t select(void* cookie, uint8 event, uint32 ref, selectsync* sync);
   status_t deselect(void* cookie, uint8 event, selectsync* sync);
   status_t readv(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes);
   status_t writev(void* cookie, off_t position, const iovec* vec, size_t count, size_t* numBytes);

If you are familiar with the use of files and file descriptors in UNIX systems, the names of some of
these functions will look familiar. This is because the driver is implementing the other side of
these operations in the case of device files. For example, when an application calls read() on the
device file, this results in calling the driver's read hook.

All the functions return a status_t error code and are allowed to fail with any error code. If in
doubt, it's always safe to return B_NOT_SUPPORTED for things you don't want to handle. The error
code will be forwarded to the calling application.

Now let's look into each of these functions in more detail.

open
   This is the first hook that will be called, when an application opens the device file.
   It receives the device name (corresponding to one that was listed by publish_devices) and flags
   (such as O_EXCL, O_RDWR, etc). The cookie argument is an output. You can allocate some memory
   and store a pointer to it there. The cookie will then be passed to all the other functions. In
   simple cases, where your driver manages a single device and doesn't need to track any state,
   you may get away with not using the cookie at all. The cookie is associated with a file
   descriptor in the application.

close
   This is called when the file description is closed. Note that UNIX has separate notions of a
   file descriptor and the underlying file description. For example, duplicating a file descriptor
   with dup() does not create a new file description. The kernel keeps track of duplicated file
   descriptors, and calls the close hook only when all file descriptors are closed.

free
   This is called when the file descriptor can be freed. It is a good time to release the memory
   allocation for the cookie structure. This is separate from close because a device can be closed
   while it is still in use by the select/deselect hooks.

control
   This hook is called when an application uses ioctl or posix_devctl on the device file. It is
   used for all operations on a device that don't map well to the basic "read" and "write" logic.
   Some other operations (such as getting and setting file descriptor flags using fcntl) also use
   the control hook.
   A control operation contains an operation code, and a sized buffer which may be used for input,
   output, or both. There is a list of generic and specific ioctls defined in os/drivers/Drivers.h,
   but drivers can define extra operations as needed.

read, write
   These implement read and write operations. Note that the direction is as seen from the application
   calling the function. Therefore, in a read operation, your driver should fill the passed buffer
   with data from the device, and in a write operation, it should transfer the data from the buffer
   to the device.
   read and write operations always include the position (offset). similarly to userspace functions
   pread and pwrite. The kernel internally handles seeking and keeping track of the current position
   for you. In some cases, the position can be ignored completely (for example, a driver returning
   values from a temperature sensor should just return the current value at every read).

select, deselect
   These are used to implement waiting for events on the device, implementing userspace functions
   like select(), poll() and kqueue().

readv, writev
   These function implement "scatter-gather" input and output. They are similar to read and write,
   but instead of storing the data in a single linear buffer, they have an array of smaller buffers.
   This gives application more flexibility in the way they manage their memory.

Nonblocking and asynchronous IO
-------------------------------

By default, the read and write operations are blocking. This means the function should not return
until the operation is fully complete, and the data buffer is only guaranteed to exist as long as
the function is running. When implementing non-blocking IO, the driver will have to do its own
caching and copy the data in its own buffers. This is usually inefficient. If your driver is
expected to require a high throughput or handle a lot of data in asynchronous mode, using the "new"
driver model (see the "device manager" section of the documentation) allows an additional io hook
designed for this, typically used in combination with the IO scheduler.

There are also some special cases of asynchronous IO. For example, network devices implement it through
a specific ioctl dealing with net_buffer structures.

Unsupported filesystem functions
--------------------------------

In Haiku, device file cannot be memory-mapped with mmap. Instead, there is typically an ioctl
allowing to obtain an area id for accessing memory mapped data (for example: framebuffer drivers
are implemented in this way).

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

