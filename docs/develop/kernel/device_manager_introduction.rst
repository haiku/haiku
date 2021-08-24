Device Driver Architecture
==================================================

This document tries to give you a short introduction into the new device
manager, and how to write drivers for it. Haiku still supports the
legacy device driver architecture introduced with BeOS.

The new device driver architecture of Haiku is still a moving target,
although most of its details are already specificed.

1. The Basics
-------------

The device manager functionality builds upon *device_node* objects.
Every driver in the system publishes one or more of such nodes, building
a tree of device nodes. This tree is in theory a dynamic representation
of the current hardware devices in the system, but in practice will also
contain implementation specific details; since every node comes with an
API specific to that node, you'll find device nodes that only come with
a number of support functions for a certain class of drivers.

Structurally, a *device_node* is a set of a module, attributes, and
resources, as well as a parent and children. At a minimum, a node must
have a module, all other components are optional.

TODO: picture of the device node tree

When the system starts, there is only a root node registered. Only
primary hardware busses register with the root node, such as PCI, and
ISA on x86. Since the PCI bus is an intelligent bus, it knows what
hardware is installed, and registers a child node for each device on the
bus.

Every driver can also publish a device in */dev* for communication with
userland applications. All drivers and devices are kernel modules.

2. Exploring the Device Tree
----------------------------

So how does it all work? When building the initial device tree, the
system only explores a minimum of device drivers only, resulting in a
tree that basically only shows the hardware found in the computer.

Now, if the system requires disk access, it will scan the device file
system for a driver that provides such functionality, in this case, it
will look for drivers under "/dev/disk/". The device manager has a set
of built-in rules for how to translate a device path into a device node,
and vice versa: every node representing a device of an intelligent bus
(such as PCI) will also contain device type information following the
PCI definitions. In this case, the "disk" sub-path will translate into
the *PCI_mass_storage* type, and hence, the device manager will then
completely explore all device nodes of that type.

It will also use that path information to only ask drivers that actually
are in a matching module directory. In the above example of a disk
driver, this would be either in "busses/scsi", "busses/ide",
"drivers/disk", ...

For untyped or generic busses, it will use the context information
gained from the devfs query directly, and will search for drivers in
that sub directory only. The only exception to this rule are the devfs
directories "disk", "ports", and "bus", which will also allow to search
matching drivers in "busses". While this is relatively limited, it is a
good way to cut down the number of drivers to be loaded.

3. Writing a Driver
-------------------

The device manager assumes the following API from a driver module:

-  **supports_device()**
   Determines wether or not the driver supports a given parent device
   node, that is the hardware device it represents (if any), and the API
   the node exports.
-  **register_device()**
   The driver should register its device node here. The parent driver is
   always initialized at this point. When registering the node, the
   driver can also attach certain I/O resources (like I/O ports, or
   memory ranges) to the node -- the device manager will make sure that
   only one node can claim these resources.
-  **init_driver()**
   Any initialization necessary to get the driver going. For most
   drivers, this will be reduced to the creation of a private data
   structure that is going to be used for all of the following
   functions.
-  **uninit_driver()**
   Uninitializes resources acquired by **init_driver()**.
-  **register_child_devices()**
   If the driver wants to register any child device nodes or to publish
   any devices, it should do so here. This function is called only
   during the initial registration process of the device node.
-  **rescan_child_devices()**
   Is called whenever a manual rescan is triggered.
-  **device_removed()** Is called when the device node is about to be
   unregistered when its device is gone, for example when a USB device
   is unplugged.
-  **suspend()**
   Enters different sleep modes.
-  **resume()**
   Resumes a device from a previous sleep mode.

To ensure that a module exports this API, it **must** end its module
name with "driver_v1" to denote the version of the API it supports. Note
that **suspend()** and **resume()** are currently never called, as Haiku
has no power management implemented yet.

If your driver can give the device it is attached to a nice name that
can be presented to the user, it should add the **B_DEVICE_PRETTY_NAME**
attribute to the device node.

The **B_DEVICE_UNIQUE_ID** should be used in case the device has a
unique ID that can be used to identify it, and also differentiate it
from other devices of the same model and vendor. This information will
be added to the file system attributes of all devices published by your
driver, so that user applications can identify, say, a USB printer no
matter what USB slot it is attached to, and assign it additional data,
like paper configuration, or recognize it as the default printer.

If your driver implements an API that is used by a support or bus
module, you will usually use the **B_DEVICE_FIXED_CHILD** attribute to
specify exactly which child device node you will be talking to. If you
support several child nodes, you may want to have a closer look at the
section explaining `how to write a bus driver <#bus_driver>`__.

In addition to the child nodes a driver registers itself, a driver can
either have dynamic children or fixed children, never both. Also, fixed
children are registered before **register_child_devices()** is called,
while dynamic children are registered afterwards.

4. Publishing a Device
----------------------

To publish a device entry in the device file system under */dev*, all
your driver has to do is to call the

::

       publish_device(device_node *node, const char *path,
           const char *deviceModuleName);

function the device manager module exports. The *path* is the path
component that follows "/dev", for example "net/ipro1000/0". The
*deviceModuleName* is the module exporting the device functionality. It
should end with "device_v1" to show the device manager which protocol it
supports. If the device node your device belongs to is removed, your
device is removed automatically with it. On the other hand, you are
allowed to unpublish the device at any point using the
**unpublish_device()** function the device manager delivers for this.

A device module must export the following API:

-  **init_device()**
   This is called when the open() is called on this device for the first
   time. You may want to create a private data structure that is passed
   on to all subsequent calls of the **open()** function that your
   device exports.
-  **uninit_device()**
   Is called when the last file descriptor to the device had been
   closed.
-  **device_removed()**
   When the device node your device belongs to is going to be removed,
   you're notified about this in this function.
-  **open()**
   Called whenever your device is opened.
-  **close()**
-  **free()**
   Free the private data structure you allocated in **open()**.
-  **read()**
-  **write()**
-  **io()**
   This is a replacement for the **read()**, and **write()** calls, and
   allows, among other things, for asynchronous I/O. This functionality
   has not yet been implemented, though (see below).
-  **control()**
-  **select()**
-  **deselect()**

5. Writing a Bus Driver
-----------------------

A bus driver is a driver that represents a bus where one or more
arbitrary devices can be attached to.

There are two basic types of busses: intelligent busses like PCI or USB
that know a lot about the devices attached to it, like a generic device
type, as well as device and vendor ID information, and simple
untyped/generic busses that either have not all the information (like
device type) or don't even know what and if any devices are attached.
The device manager has been written in such a way that device
exploration makes use of additional information the bus can provide in
order to find a responsible device driver faster, and with less
overhead.

5.1. Writing an Intelligent Bus Driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

If your bus knows what type of device is attached to, and also has
vendor and device ID information about that device, it is considered to
be an intelligent bus. The bus driver is supposed to have one parent
node representing the bus, and to create a child node for each device
attached to the bus.

The additional information you have about the devices are attached to
the device node in the following attributes:

-  **B_DEVICE_VENDOR_ID**
   The vendor ID - this ID has only to be valid in the namespace of your
   bus.
-  **B_DEVICE_ID**
   The device ID.
-  **B_DEVICE_TYPE**
   The device type as defined by the PCI class base information.
-  **B_DEVICE_SUB_TYPE**
   The device sub type as defined by the PCI sub class information.
-  **B_DEVICE_INTERFACE**
   The device interface type as defined by the PCI class API
   information.

You can use the **B_DEVICE_FLAGS** attribute to define how the device
manager finds the children of the devices you exported. For this kind of
bus drivers, you will usually only want to specify
**B_FIND_CHILD_ON_DEMAND** here, which causes the driver only to be
searched when the system asks for it.

5.2. Writing a Simple Bus Driver
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

A bus can be simple in a number of ways:

#. It may not know how many or if any devices are attached to it
#. It cannot retrieve any type information about the devices it has, but
   knows all devices that are attached to it

An example of the latter would be the Zorro bus of the Amiga - it only
has information about the vendor and device ID, but no type information.
It should be implemented like an intelligent bus, though, with the type
information simply omitted.

Therefore, this section is about the former case, that is, a simple bus
like the ISA bus. Since it doesn't know anything about its children, it
does not publish any child nodes, instead, it will just specify the
B_FIND_MULTIPLE_CHILDREN and B_FIND_CHILD_ON_DEMAND flags for its device
node. Since there is no additional information about this bus, the
device manager will assume a simple bus, and will try to find drivers on
demand only.

The generic bus
---------------

Some devices are not tied to a specific bus. This is the case for all
drivers that do not relate to a physical device: /dev/null, /dev/zero,
/dev/random, etc. A "generic" bus has been added, and these drivers can
attach to it.

6. Open Issues
--------------

While most of the new device manager is fledged out, there are some
areas that could use improvements or are problematic under certain
requirements. Also, some parts just haven't been written yet.

6.1. generic/simple busses
^^^^^^^^^^^^^^^^^^^^^^^^^^

6.2. Unpublishing
^^^^^^^^^^^^^^^^^

6.4. Versioning
^^^^^^^^^^^^^^^

The way the device manager works, it makes versioning of modules (which
are supposed to be one of the strong points of the module system) much
harder or even impossible. While the device manager could introduce a
new API and could translate between a "driver_v1", and a "driver_v2" API
on the fly, it's not yet possible for a PCI sub module to do the same
thing.

**Proposed Solution:** Add attribute **B_DEVICE_ALTERNATE_VERSION** that
specifies alternate versions of the module API this device node
supports. We would then need a **request_version()** or
**set_version()** function (to be called from **supports_device()**)
that allows to specify the version of the parent node this device node
wants to talk to.

6.5. Unregistering Nodes
^^^^^^^^^^^^^^^^^^^^^^^^

6.6. Support for generic drivers is missing
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

This should probably be done by simply adding a simple bus driver named
"generic" that generic drivers need to ask for.

6.7. Mappings, And Other Optimizations
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Due to the way the device tree is built, the device manager could
remember which driver served a given device node. That way, it wouldn't
need to search for a driver anymore, but could just pick it up.
Practically, the device manager should cache the type (and/or
vendor/device) information of a node, and assign one or more drivers
(via module name) to this information. It should also remember negative
outcome, that is if there is no driver supporting the hardware.

This way, only the first boot would require an actual search for
drivers, as subsequent boots would reuse the type-driver assignments. If
a new driver is installed, the cached assignments would need to be
updated immediately. If a driver has been installed outside of the
running system, the device manager might want to create a hash per
module directory to see if anything changed to flush the cache.
Alternatively or additionally, the boot loader could have a menu causing
the cache to be ignored.

It would be nice to find a way for generic and simple busses to reduce
the amount of searching necessary for them. One way would be to remember
which driver supports which bus - but this information is currently only
accessible derived from what the driver does, and is therefore not
reliable or complete. A separately exported information would be
necessary for this.

Also, when looking for a generic or simple bus driver, actual
directories could be omitted; currently, driver search is always
recursive, as that's how the module mechanism is working. Eventually, we
might want to extend the open_module_list_etc() call a bit more to
accomplish that.
