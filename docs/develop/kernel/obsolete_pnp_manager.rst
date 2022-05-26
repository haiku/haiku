Plug and Play Manager
=====================

This file contains the documentation written by Thomas Kurschel that was originally
found in the headers of his pnp_manager.
It's outdated but could be used as a basis for the real documentation.

PNP Manager
-----------

PnP manager; Takes care of registration and loading of PnP drivers

Read pnp_driver.h first to understand the basic idea behind PnP drivers.

To register a driver node, use register_driver. If the device got lost, 
use unregister_driver (note: if the parent node is removed, your node 
get removed automatically as your driver has obviously nothing to work 
with anymore). To get access to a (parent) device, use load_driver/
unload_driver.

To let the manager find a consumer (see pnp_driver.h), you can either
specify its name directly during registration, using a 
PNP_DRIVER_FIXED_CONSUMER attribute, or let the manager search the 
appropriate consumer(s) via a PNP_DRIVER_DYNAMIC_CONSUMER attribute. 

Searching of dynamic consumers is done as follows:

- First, the manager searches for a Specific driver in the base 
  directory (see below)
- If no Specific driver is found, all Generic drivers stored under
  "generic" sub-directory are informed in turn until one returns success
- Finally, _all_ Universal drivers, stored in the "universal" sub-
  directory, are informed

Specification of the base directory and of the names of Specific 
drivers is done via a file name pattern given by a 
PNP_DRIVER_DYNAMIC_CONSUMER attribute. 

First, all substrings of the form "%attribute_name%" are replaced by the 
content of the attribute "attribute_name" as follows:

- if the attribute contains an integer value, its content is converted to hex 
  (lowercase) with a fixed length according to the attribute's value range
- the content of string attributes is quoted by " and invalid characters 
  (i.e. /%" and all characters outside 32..126) are replaced by their
  unsigned decimal value, delimited by %
- other attribute types cannot be used

Second, the resulting name is split into chunks according to the presence 
of | characters (you can escape % and | with a ^ character). These
characters are only delimiters and get removed before further processing.
The directory before the first | character is the base directory (see 
above). It contains the "generic" and the "universal" subdirectories. 
The names of the specific drivers are created by first taking the entire
file name, then by removing the last chunk, then by removing the last
two chunks and so on until only the first chunk is left. 

As drivers can contain multiple modules, the module name is constructed
by appending the content of the PNP_DRIVER_TYPE attribute to the driver's file
name, seperated by a slash character (note: this only applies to dynamic
consumers; for fixed consumers, you specify the module name directly via 
PNP_DRIVER_FIXED_CONSUMER).

E.g. given a dynamic consumer pattern of 
"pci/vendor=%vendor_id%|, device=%device_id%" for a device with the 
attributes vendor_id=0x123 and device_id=0xabcd (both being uint16), the
PnP manager tries the specific drivers "pci/vendor=0123, device=abcd" and
(if the first one fails/doesn't exist) "pci/vendor=0123". If they both
refuse to handle the device, all drivers under "pci/generic" are tried
until one accepts the device. Finally, all drivers under "pci/universal" 
are	loaded, whatever happened before.

In practise, you should try to use specific drivers as much as possible.
If detection based on device IDs is impossible (e.g. because the bus 
doesn't support them at all), you can put the driver under "generic".
Generic drivers can also be used to specify wrappers that try to load old-
style drivers if no new driver can be found. Also, they can be used to
report an error or invoke an user program that tries downloading a
proper Specific driver. Universal drivers are mainly used for 
informational purposes, e.g. to publish data about each found device,
or to provide raw access to all devices.

If the device uses physical address space or I/O space or ISA DMA 
channels (called I/O resources), the driver has to acquire these 
resources. During hardware detection (usually via probe()), 
acquire_io_resources() must be called to get exclusive access.
If no hardware could be found, they must be released via 
release_io_resources(). If detection was successful, the list of 
the (acquired) resources must be passed to register_device().
Resources can either belong to one hardware detection or to a device.
If a hardware detection collides with another, it has to wait; 
if it collides with a device whose driver is not loaded, the
driver loading is blocked. When detection fails, i.e. if 
release_io_resources() is called, all blocked drivers can be loaded
again. If the detection fails, i.e. the resources are transferred
via register_device(), all blocked devices are unregistered and
pending load requests aborted. If a hardware detection collides
with a device whose driver is loaded, acquire_io_resources() fails
with B_BUSY. As this makes a hardware rescan impossible if the 
driver is loaded, you should define	PNP_DRIVER_NO_LIVE_RESCAN 
for nodes that use I/O resources (see below).

To search for new drivers for a given device node, use rescan(). This
marks all consumer devices as being verified and calls probe() 
of all consumers drivers (see above) to let them rescan the parent 
for devices. The <depth> parameter determines the nesting level, e.g.
2 means that first the consumers are scanned and then the consumers
of the consumers.

Normally, all devices can be rescanned. If a driver cannot handle
a rescan safely when it is loaded (i.e. used by a consumer), it
must set PNP_DRIVER_NO_LIVE_RESCAN, in which case the device is
ignored during rescan if the driver is loaded and attempts
to load the driver during a rescan are blocked until the rescan
is finished. If rescanning a device is not possible at all, it must
have set PNP_DRIVER_NEVER_RESCAN to always ignore it.

To distinguish between new devices, lost devices and redetected
devices, consumer devices should provide a connection code and a
device identifier. They are specified by PNP_DRIVER_CONNECTION and
PNP_DRIVER_CONNECTION respectively, and are expanded in the same way
as PNP_DRIVER_DYNAMIC_CONSUMER. It is assumed that there can be only
one device per connection and that a device can be uniquely identify
by a device identifier. If a consumer device is registered on the 
same connection as an existing device but with a different device 
identifier, the old device gets unregistered automatically. If both 
connection and device identifier are the same, registration is 
handled as a redetection and ignored (unless a different type or 
driver module is specified - in this case, the device is replaced). 
Devices that were not redetected during a rescan get unregistered
unless they were ignored (see above).

.. code-block:: cpp

    // interface of PnP manager
    typedef struct device_manager_info {
	module_info info;

	// load driver
	// node - node whos driver is to be loaded
	// user_cookie - cookie to be passed to init_device of driver
	// interface - interface of loaded driver
	// cookie - device cookie issued by loaded driver
	status_t (*init_driver)(device_node_handle node, void *userCookie,
					driver_module_info **interface, void **cookie);
	// unload driver
	status_t (*uninit_driver)(device_node_handle node);

	// rescan node for new dynamic drivers
	// node - node whose dynamic drivers are to be scanned
	status_t (*rescan)(device_node_handle node);

	// register device
	// parent - parent node
	// attributes - NULL-terminated array of node attributes
	// io_resources - NULL-terminated array of I/O resources (can be NULL)
	// node - new node handle
	// on return, io_resources are invalid: on success I/O resources belong 
	// to node, on fail they are released;
	// if device is already registered, B_OK is returned but *node is NULL
	status_t (*register_device)(device_node_handle parent,
					const device_attr *attrs,
					const io_resource_handle *io_resources,
					device_node_handle *node);
	// unregister device
	// all nodes having this node as their parent are unregistered too.
	// if the node contains PNP_MANAGER_ID_GENERATOR/PNP_MANAGER_AUTO_ID
	// pairs, the id specified this way is freed too
	status_t (*unregister_device)(device_node_handle node);

	// find device by node content
	// the given attributes must _uniquely_ identify a device node;
	// parent - parent node (-1 for don't-care)
	// attrs - list of attributes (can be NULL)
	// The node you got will be automatically put on the next call
	// to this function.
	status_t (*get_next_child_device)(device_node_handle parent,
		device_node_handle *_node, const device_attr *attrs);

	// get parent device node
	device_node_handle (*get_parent)(device_node_handle node);

	// Must be called after get_next_child_device() (if you don't iterate through)
	// and get_parent() to make sure the node is freed when it's not used anymore
	void (*put_device_node)(device_node_handle node);

	// acquire I/O resources
	// resources - NULL-terminated array of resources to acquire
	// handles - NULL-terminated array of handles (one per resource); 
	//           array must be provided by caller
	// return B_BUSY if a resource is used by a loaded driver
	status_t (*acquire_io_resources)(io_resource *resources,
					io_resource_handle *handles);
	// release I/O resources
	// handles - NULL-terminated array of handles
	status_t (*release_io_resources)(const io_resource_handle *handles);

	// create unique id
	// generator - name of id set
	// if result >= 0 - unique id
	//    result < 0 - error code
	int32 (*create_id)(const char *generator);
	// free unique id
	status_t (*free_id)(const char *generator, uint32 id);

	// helpers to extract attribute by name.
	// if <recursive> is true, parent nodes are scanned if 
	// attribute isn't found in current node; unless you declared
	// the attribute yourself, use recursive search to handle
	// intermittent nodes, e.g. defined by filter drivers, transparently.
	// for raw and string attributes, you get a copy that must 
	// be freed by caller 
	status_t (*get_attr_uint8)(device_node_handle node,
					const char *name, uint8 *value, bool recursive);
	status_t (*get_attr_uint16)(device_node_handle node,
					const char *name, uint16 *value, bool recursive);
	status_t (*get_attr_uint32)(device_node_handle node,
					const char *name, uint32 *value, bool recursive);
	status_t (*get_attr_uint64)(device_node_handle node,
					const char *name, uint64 *value, bool recursive);
	status_t (*get_attr_string)(device_node_handle node,
					const char *name, char **value, bool recursive);
	status_t (*get_attr_raw)(device_node_handle node,
					const char *name, void **data, size_t *_size,
					bool recursive);

	// get next attribute of node;
	// on call, *<attr_handle> must contain handle of an attribute;
	// on return, *<attr_handle> is replaced by the next attribute or
	// NULL if it was the last;
	// to get the first attribute, <attr_handle> must point to NULL;
	// the returned handle must be released by either passing it to
	// another get_next_attr() call or by using release_attr()
	// directly
	status_t (*get_next_attr)(device_node_handle node,
					device_attr_handle *attrHandle);

	// release attribute handle <attr_handle> of <node>;
	// see get_next_attr
	status_t (*release_attr)(device_node_handle node,
					device_attr_handle attr_handle);

	// retrieve attribute data with handle given;
	// <attr> is only valid as long as you don't release <attr_handle> 
	// implicitely or explicitely
	status_t (*retrieve_attr)(device_attr_handle attr_handle,
					const device_attr **attr);

	// change/add attribute <attr> of/to node
	status_t (*write_attr)(device_node_handle node,
					const device_attr *attr);

	// remove attribute of node by name
	// <name> is name of attribute
	status_t (*remove_attr)(device_node_handle node, const char *name);
    } device_manager_info;

PNP Driver
----------

Required interface of PnP drivers

In contrast to standard BeOS drivers, PnP drivers are normal modules
having the interface described below.

Every device is described by its driver via a PnP node with properties
described in PnP Node Attributes. Devices are organized in a hierarchy, 
e.g. a devfs device is a hard disk device that is connected to a 
controller, which is a PCI device, that is connected to a PCI bus.
Every device is connected to its lower-level device	via a parent link 
stored in its Node. The higher-level is called the consumer of the 
lower-level device. If the lower-level device gets removed, all its 
consumers are removed too.

In our example, the hierarchy is

  devfs device -> hard disk -> controller -> PCI device -> PCI bus

If the PCI bus is removed, everything up to including the devfs device
is removed too.

The driver hierarchy is constructed bottom-up, i.e. the lower-level
driver searches for a corresponding consumer, which in turns searches
for its consumer and so on. The lowest driver is usually something like
a PCI bus, the highest driver is normally a devfs entry (see pnp_devfs.h).
Registration of devices and the search for appropriate consumers is 
done via the pnp_manager (see pnp_manager.h).

When a potential consumer is found, it gets informed about the new
lower-level device and can either refuse its handling or accept it.
On accept, it has to create a new node with the	lower-level device 
node as its parent.

Loading of drivers is done on demand, i.e. if the consumer wants to
access its lower-level device, it explicitely loads the corresponding 
driver, and once it doesn't need it anymore, the lower-level driver
must be unloaded. Usually, this process happens recursively, i.e. in
our example, the hard disk driver loads the controller driver, which 
loads the PCI device driver which loads the PCI bus driver. The same
process applies to unloading.

Because of this dynamic loading, drivers must store persistent data
in the node of their devices. Please be aware that you cannot modify
a node once published.

If a device gets removed, you must unregister its node. As said, the
PnP manager will automatically unregister all consumers too. The 
corresponding drivers are notified to stop talking to their	lower-level 
devices and to terminate running requests. Normally, you want to use a 
dedicated variable that	is verified at each call to make sure that the 
parent is still there. The notification is done independantly of the
driver being loaded by its consumer(s) or not. If it isn't loaded,
the notification callback gets NULL as the device cookie; normally, the
driver returns immediately in this case. As soon as both the device
is removed and the driver is unloaded, device_cleanup gets called to
free resources that couldn't be safely removed in device_removed when
the driver was still loaded.

If a device has exactly one consumer, they often interact in some way.
To simplify that, the consumer can pass a user-cookie to its parent
during load. In this case, it's up to the parent driver to get a 
pointer to the interface of the consumer. Effectively, such consumers
have one interface for their consumers (base on pnp_driver_info), and 
a another for their parents (with a completely driver-specific 
structure).

In terms of synchronization, loading/unloading/remove-notifications
are executed synchronously, i.e. if e.g. a device is to be unloaded 
but	the drive currently handles a remove-notification, the unloading 
is delayed until the nofication callback returns. If multiple consumers
load a driver, the driver gets initialized only once; subsequent load
requests increase an internal load count only and return immediately.
In turn, unloading only happens once the load count reaches zero.

.. code-block:: cpp

    struct driver_module_info {
	module_info info;

	float (*supports_device)(device_node_handle parent, bool *_noConnection);
		// check whether this parent is supported

	status_t (*register_device)(device_node_handle parent);
		// Register your device node.

	status_t (*init_driver)(device_node_handle node, void *user_cookie, void **_cookie);
		// driver is loaded.
		// node - node of device
		// user_cookie - cookie passed by loading driver
		// cookie - cookie issued by this driver

	status_t (*uninit_driver)(void *cookie);
		// driver gets unloaded.

	void (*device_removed)(device_node_handle node, void *cookie);
		// a device node, registered by this driver, got removed.
		// if the driver wasn't loaded when this happenes, no (un)init_device 
		// is called and thus <cookie> is NULL;

	void (*device_cleanup)(device_node_handle node);
		// a device node, registered by this driver, got removed and
		// the driver got unloaded

	void (*get_supported_paths)(const char ***_busses, const char ***_devices);
    };

PNP Bus
-------

Required interface of PnP bus drivers

Busses consist of two node layers: the lower layer defines the bus,
the upper layer defines the abstract devices connected to the bus. 
Both layers are handled by a bus manager. Actual device nodes are 
on top of abstract device nodes.

E.g. if we have a PCI bus with an IDE controller on it, we get

IDE controller -> PCI device -> PCI bus

with:

* IDE controller = actual device node
* PCI device = abstract device node
* PCI bus = bus node

The PCI bus manager establishes both the PCI devices and the PCI busses.

Abstract device nodes act as a gateway between actual device nodes
and the corresponding bus node. They are constructed by the bus 
node driver via	its rescan() hook. To identify a bus node, define
PNP_BUS_IS_BUS as an attribute of it. As a result, the PnP manager
will call the rescan() method of the bus driver whenever the
bus is to be rescanned. Afterwards, all possible dynamic consumers
are informed as done for normal nodes.

Normally, potential device drivers are notified immediately when 
rescan() registers a new abstract device node. But sometimes, device
drivers need to know _all_ devices connected to the bus for correct
detection. To ensure this, the bus node must define 
PNP_BUS_NOTIFY_CONSUMERS_AFTER_RESCAN. In this case, scanning for
consumers is postponed until rescan() has finished.

If hot-plugging of devices can be detected automatically (e.g. USB), 
you should define PNP_DRIVER_ALWAYS_LOADED, so the bus driver is 
always loaded and thus capable of handling hot-plug events generated 
by the bus controller hardware.

