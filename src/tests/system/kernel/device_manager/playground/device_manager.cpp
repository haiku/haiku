/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device_manager.h"

#include <new>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <Locker.h>
#include <module.h>
#include <PCI.h>

#include <fs/KPath.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/Stack.h>

#include "bus.h"

#define TRACE(a) dprintf a

#define DEVICE_MANAGER_ROOT_NAME "system/devices_root/driver_v1"

extern struct device_module_info gDeviceModuleInfo;
extern struct driver_module_info gDriverModuleInfo;
extern struct device_module_info gGenericVideoDeviceModuleInfo;
extern struct driver_module_info gGenericVideoDriverModuleInfo;
extern struct device_module_info gSpecificVideoDeviceModuleInfo;
extern struct driver_module_info gSpecificVideoDriverModuleInfo;
extern struct driver_module_info gBusModuleInfo;
extern struct driver_module_info gBusDriverModuleInfo;

extern "C" status_t _add_builtin_module(module_info *info);
extern "C" status_t _get_builtin_dependencies(void);
extern bool gDebugOutputEnabled;
	// from libkernelland_emu.so

struct device_attr_private : device_attr,
		DoublyLinkedListLinkImpl<device_attr_private> {
						device_attr_private();
						device_attr_private(const device_attr& attr);
						~device_attr_private();

			status_t	InitCheck();
			status_t	CopyFrom(const device_attr& attr);

	static	int			Compare(const device_attr* attrA,
							const device_attr *attrB);

private:
			void		_Unset();
};

typedef DoublyLinkedList<device_attr_private> AttributeList;

// I/O resource
typedef struct io_resource_info {
	struct io_resource_info *prev, *next;
	device_node*		owner;			// associated node; NULL for temporary allocation
	io_resource			resource;		// info about actual resource
} io_resource_info;

class Device : public DoublyLinkedListLinkImpl<Device> {
public:
							Device(device_node* node, const char* path,
								const char* moduleName);
							~Device();

			status_t		InitCheck() const;

			device_node*	Node() const { return fNode; }
			const char*		Path() const { return fPath; }
			const char*		ModuleName() const { return fModuleName; }

			status_t		InitDevice();
			void			UninitDevice();

			device_module_info* DeviceModule() const { return fDeviceModule; }
			void*			DeviceData() const { return fDeviceData; }

private:
	device_node*			fNode;
	const char*				fPath;
	const char*				fModuleName;

	int32					fInitialized;
	device_module_info*		fDeviceModule;
	void*					fDeviceData;
};

typedef DoublyLinkedList<Device> DeviceList;
typedef DoublyLinkedList<device_node> NodeList;

struct device_node : DoublyLinkedListLinkImpl<device_node> {
							device_node(const char* moduleName,
								const device_attr* attrs,
								const io_resource* resources);
							~device_node();

			status_t		InitCheck() const;

			const char*		ModuleName() const { return fModuleName; }
			device_node*	Parent() const { return fParent; }
			AttributeList&	Attributes() { return fAttributes; }
			const AttributeList& Attributes() const { return fAttributes; }

			status_t		InitDriver();
			bool			UninitDriver();
			void			UninitUnusedDriver();

			// The following two are only valid, if the node's driver is
			// initialized
			driver_module_info* DriverModule() const { return fDriver; }
			void*			DriverData() const { return fDriverData; }

			void			AddChild(device_node *node);
			void			RemoveChild(device_node *node);
			const NodeList&	Children() const { return fChildren; }
			void			DeviceRemoved();

			status_t		Register(device_node* parent);
			status_t		Probe(const char* devicePath, uint32 updateCycle);
			bool			IsRegistered() const { return fRegistered; }
			bool			IsInitialized() const { return fInitialized > 0; }
			uint32			Flags() const { return fFlags; }

			void			Acquire();
			bool			Release();

			const DeviceList& Devices() const { return fDevices; }
			void			AddDevice(Device* device);
			void			RemoveDevice(Device* device);

			int				CompareTo(const device_attr* attributes) const;
			device_node*	FindChild(const device_attr* attributes) const;
			device_node*	FindChild(const char* moduleName) const;

			void			Dump(int32 level = 0);

private:
			status_t		_RegisterFixed(uint32& registered);
			bool			_AlwaysRegisterDynamic();
			status_t		_AddPath(Stack<KPath*>& stack, const char* path,
								const char* subPath = NULL);
			status_t		_GetNextDriverPath(void*& cookie, KPath& _path);
			status_t		_GetNextDriver(void* list,
								driver_module_info*& driver);
			status_t		_FindBestDriver(const char* path,
								driver_module_info*& bestDriver,
								float& bestSupport,
								device_node* previous = NULL);
			status_t		_RegisterPath(const char* path);
			status_t		_RegisterDynamic(device_node* previous = NULL);
			status_t		_RemoveChildren();
			device_node*	_FindCurrentChild();
			void			_ReleaseWaiting();

	device_node*			fParent;
	NodeList				fChildren;
	int32					fRefCount;
	int32					fInitialized;
	bool					fRegistered;
	uint32					fFlags;
	float					fSupportsParent;
	uint32					fLastUpdateCycle;

	const char*				fModuleName;

	driver_module_info*		fDriver;
	void*					fDriverData;

	DeviceList				fDevices;
	AttributeList			fAttributes;
};

// flags in addition to those specified by B_DEVICE_FLAGS
enum node_flags {
	NODE_FLAG_REGISTER_INITIALIZED	= 0x00010000,
	NODE_FLAG_DEVICE_REMOVED		= 0x00020000,
	NODE_FLAG_OBSOLETE_DRIVER		= 0x00040000,
	NODE_FLAG_WAITING_FOR_DRIVER	= 0x00080000,

	NODE_FLAG_PUBLIC_MASK			= 0x0000ffff
};

device_manager_info *gDeviceManager;

static device_node *sRootNode;
static recursive_lock sLock;

static uint32 sDriverUpdateCycle = 1;
	// this is a *very* basic devfs emulation


//	#pragma mark -


static device_attr_private*
find_attr(const device_node* node, const char* name, bool recursive,
	type_code type)
{
	do {
		AttributeList::ConstIterator iterator
			= node->Attributes().GetIterator();

		while (iterator.HasNext()) {
			device_attr_private* attr = iterator.Next();

			if (type != B_ANY_TYPE && attr->type != type)
				continue;

			if (!strcmp(attr->name, name))
				return attr;
		}

		node = node->Parent();
	} while (node != NULL && recursive);

	return NULL;
}


static void
put_level(int32 level)
{
	while (level-- > 0)
		dprintf("   ");
}


static void
dump_attribute(device_attr* attr, int32 level)
{
	if (attr == NULL)
		return;

	put_level(level + 2);
	dprintf("\"%s\" : ", attr->name);
	switch (attr->type) {
		case B_STRING_TYPE:
			dprintf("string : \"%s\"", attr->value.string);
			break;
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			dprintf("uint8 : %u (%#x)", attr->value.ui8, attr->value.ui8);
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			dprintf("uint16 : %u (%#x)", attr->value.ui16, attr->value.ui16);
			break;
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			dprintf("uint32 : %lu (%#lx)", attr->value.ui32, attr->value.ui32);
			break;
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
			dprintf("uint64 : %Lu (%#Lx)", attr->value.ui64, attr->value.ui64);
			break;
		default:
			dprintf("raw data");
	}
	dprintf("\n");
}


static void
uninit_unused()
{
	puts("uninit unused");
	RecursiveLocker _(sLock);
	sRootNode->UninitUnusedDriver();
}


static status_t
probe_path(const char* path)
{
	printf("probe path \"%s\"\n", path);
	RecursiveLocker _(sLock);
	return sRootNode->Probe(path, sDriverUpdateCycle);
}


static void
close_path(void* cookie)
{
	Device* device = (Device*)cookie;
	if (device == NULL)
		return;

	printf("close path \"%s\" (node %p)\n", device->Path(), device->Node());
	device->UninitDevice();
}


static Device*
get_device(device_node* node, const char* path)
{
	DeviceList::ConstIterator iterator = node->Devices().GetIterator();
	while (iterator.HasNext()) {
		Device* device = iterator.Next();
		if (!strcmp(device->Path(), path)) {
			status_t status = device->InitDevice();
			if (status != B_OK) {
				printf("opening path \"%s\" failed: %s\n", path,
					strerror(status));
				return NULL;
			}

			printf("open path \"%s\" (node %p)\n", device->Path(),
				device->Node());
			return device;
		}
	}

	// search in children

	NodeList::ConstIterator nodeIterator = node->Children().GetIterator();
	while (nodeIterator.HasNext()) {
		device_node* child = nodeIterator.Next();

		Device* device = get_device(child, path);
		if (device != NULL)
			return device;
	}

	return NULL;
}


static void*
open_path(const char* path)
{
	return get_device(sRootNode, path);
}


//	#pragma mark - Device Manager module API


static status_t
rescan_node(device_node* node)
{
	return B_ERROR;
}


static status_t
register_node(device_node* parent, const char* moduleName,
	const device_attr* attrs, const io_resource* ioResources,
	device_node** _node)
{
	if ((parent == NULL && sRootNode != NULL) || moduleName == NULL)
		return B_BAD_VALUE;

	if (parent != NULL && parent->FindChild(attrs) != NULL) {
		// A node like this one already exists for this parent
		return B_NAME_IN_USE;
	}

	// TODO: handle I/O resources!

	device_node *newNode = new(std::nothrow) device_node(moduleName, attrs,
		ioResources);
	if (newNode == NULL)
		return B_NO_MEMORY;

	TRACE(("%p: register node \"%s\", parent %p\n", newNode, moduleName,
		parent));

	RecursiveLocker _(sLock);

	status_t status = newNode->InitCheck();
	if (status != B_OK)
		goto err1;

#if 0
	// The following is done to reduce the stack usage of deeply nested
	// child device nodes.
	// There is no other need to delay the complete registration process
	// the way done here. This approach is also slightly different as
	// the registration might fail later than it used in case of errors.

	if (!parent->IsRegistered()) {
		// The parent has not been registered completely yet - child
		// registration is deferred to the parent registration
		return B_OK;
	}
#endif

	status = newNode->Register(parent);
	if (status < B_OK) {
		parent->RemoveChild(newNode);
		goto err1;
	}

	if (_node)
		*_node = newNode;

	return B_OK;

err1:
	newNode->Release();
	return status;
}


/*!	Unregisters the device \a node.

	If the node is currently in use, this function will return B_BUSY to
	indicate that the node hasn't been removed yet - it will still remove
	the node as soon as possible.
*/
static status_t
unregister_node(device_node* node)
{
	TRACE(("unregister_node(node %p)\n", node));
	RecursiveLocker _(sLock);

	bool initialized = node->IsInitialized();

	node->DeviceRemoved();

	return initialized ? B_BUSY : B_OK;
}


static status_t
get_driver(device_node* node, driver_module_info** _module, void** _data)
{
	if (node->DriverModule() == NULL)
		return B_NO_INIT;

	if (_module != NULL)
		*_module = node->DriverModule();
	if (_data != NULL)
		*_data = node->DriverData();

	return B_OK;
}


static device_node*
get_root_node(void)
{
	if (sRootNode != NULL)
		sRootNode->Acquire();

	return sRootNode;
}


static status_t
get_next_child_node(device_node* parent, const device_attr* attributes,
	device_node** _node)
{
	RecursiveLocker _(sLock);

	NodeList::ConstIterator iterator = parent->Children().GetIterator();
	device_node* last = *_node;

	// skip those we already traversed
	while (iterator.HasNext() && last != NULL) {
		device_node* node = iterator.Next();

		if (node != last)
			continue;
	}

	// find the next one that fits
	while (iterator.HasNext()) {
		device_node* node = iterator.Next();

		if (!node->IsRegistered())
			continue;

		if (!node->CompareTo(attributes)) {
			if (last != NULL)
				last->Release();

			node->Acquire();
			*_node = node;
			return B_OK;
		}
	}

	if (last != NULL)
		last->Release();

	return B_ENTRY_NOT_FOUND;
}


static device_node*
get_parent_node(device_node* node)
{
	if (node == NULL)
		return NULL;

	RecursiveLocker _(sLock);

	device_node* parent = node->Parent();
	parent->Acquire();

	return parent;
}


static void
put_node(device_node* node)
{
	RecursiveLocker _(sLock);
	node->Release();
}


static status_t
publish_device(device_node *node, const char *path, const char *moduleName)
{
	if (path == NULL || !path[0] || moduleName == NULL || !moduleName[0])
		return B_BAD_VALUE;

	RecursiveLocker _(sLock);
	dprintf("publish device: node %p, path %s, module %s\n", node, path,
		moduleName);

	Device* device = new(std::nothrow) Device(node, path, moduleName);
	if (device == NULL)
		return B_NO_MEMORY;

	if (device->InitCheck() != B_OK) {
		delete device;
		return B_NO_MEMORY;
	}

	node->AddDevice(device);
	return B_OK;
}


static status_t
unpublish_device(device_node *node, const char *path)
{
	if (path == NULL)
		return B_BAD_VALUE;

	RecursiveLocker _(sLock);

	DeviceList::ConstIterator iterator = node->Devices().GetIterator();
	while (iterator.HasNext()) {
		Device* device = iterator.Next();
		if (!strcmp(device->Path(), path)) {
			node->RemoveDevice(device);
			delete device;
			return B_OK;
		}
	}

	return B_ENTRY_NOT_FOUND;
}


static status_t
get_attr_uint8(const device_node* node, const char* name, uint8* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_UINT8_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui8;
	return B_OK;
}


static status_t
get_attr_uint16(const device_node* node, const char* name, uint16* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_UINT16_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui16;
	return B_OK;
}


static status_t
get_attr_uint32(const device_node* node, const char* name, uint32* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_UINT32_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui32;
	return B_OK;
}


static status_t
get_attr_uint64(const device_node* node, const char* name,
	uint64* _value, bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_UINT64_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui64;
	return B_OK;
}


static status_t
get_attr_string(const device_node* node, const char* name,
	const char** _value, bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_STRING_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.string;
	return B_OK;
}


static status_t
get_attr_raw(const device_node* node, const char* name, const void** _data,
	size_t* _length, bool recursive)
{
	if (node == NULL || name == NULL || (_data == NULL && _length == NULL))
		return B_BAD_VALUE;

	device_attr_private* attr = find_attr(node, name, recursive, B_RAW_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	if (_data != NULL)
		*_data = attr->value.raw.data;
	if (_length != NULL)
		*_length = attr->value.raw.length;
	return B_OK;
}


static status_t
get_next_attr(device_node* node, device_attr** _attr)
{
	if (node == NULL)
		return B_BAD_VALUE;

	device_attr_private* next;
	device_attr_private* attr = *(device_attr_private**)_attr;

	if (attr != NULL) {
		// next attribute
		next = attr->GetDoublyLinkedListLink()->next;
	} else {
		// first attribute
		next = node->Attributes().First();
	}

	*_attr = next;

	return next ? B_OK : B_ENTRY_NOT_FOUND;
}


static struct device_manager_info sDeviceManagerModule = {
	{
		B_DEVICE_MANAGER_MODULE_NAME,
		0,
		NULL
	},

	// device nodes
	rescan_node,
	register_node,
	unregister_node,
	get_driver,
	get_root_node,
	get_next_child_node,
	get_parent_node,
	put_node,

	// devices
	publish_device,
	unpublish_device,

	// attributes
	get_attr_uint8,
	get_attr_uint16,
	get_attr_uint32,
	get_attr_uint64,
	get_attr_string,
	get_attr_raw,
	get_next_attr,
};


//	#pragma mark - device_attr


device_attr_private::device_attr_private()
{
	name = NULL;
	type = 0;
	value.raw.data = NULL;
	value.raw.length = 0;
}


device_attr_private::device_attr_private(const device_attr& attr)
{
	CopyFrom(attr);
}


device_attr_private::~device_attr_private()
{
	_Unset();
}


status_t
device_attr_private::InitCheck()
{
	return name != NULL ? B_OK : B_NO_INIT;
}


status_t
device_attr_private::CopyFrom(const device_attr& attr)
{
	name = strdup(attr.name);
	if (name == NULL)
		return B_NO_MEMORY;

	type = attr.type;

	switch (type) {
		case B_UINT8_TYPE:
		case B_UINT16_TYPE:
		case B_UINT32_TYPE:
		case B_UINT64_TYPE:
			value.ui64 = attr.value.ui64;
			break;

		case B_STRING_TYPE:
			if (attr.value.string != NULL) {
				value.string = strdup(attr.value.string);
				if (value.string == NULL) {
					_Unset();
					return B_NO_MEMORY;
				}
			} else
				value.string = NULL;
			break;

		case B_RAW_TYPE:
			value.raw.data = malloc(attr.value.raw.length);
			if (value.raw.data == NULL) {
				_Unset();
				return B_NO_MEMORY;
			}

			value.raw.length = attr.value.raw.length;
			memcpy((void*)value.raw.data, attr.value.raw.data,
				attr.value.raw.length);
			break;

		default:
			return B_BAD_VALUE;
	}

	return B_OK;
}


void
device_attr_private::_Unset()
{
	if (type == B_STRING_TYPE)
		free((char*)value.string);
	else if (type == B_RAW_TYPE)
		free((void*)value.raw.data);

	free((char*)name);

	name = NULL;
	value.raw.data = NULL;
	value.raw.length = 0;
}


/*static*/ int
device_attr_private::Compare(const device_attr* attrA, const device_attr *attrB)
{
	if (attrA->type != attrB->type)
		return -1;

	switch (attrA->type) {
		case B_UINT8_TYPE:
			return (int)attrA->value.ui8 - (int)attrB->value.ui8;

		case B_UINT16_TYPE:
			return (int)attrA->value.ui16 - (int)attrB->value.ui16;

		case B_UINT32_TYPE:
			if (attrA->value.ui32 > attrB->value.ui32)
				return 1;
			if (attrA->value.ui32 < attrB->value.ui32)
				return -1;
			return 0;

		case B_UINT64_TYPE:
			if (attrA->value.ui64 > attrB->value.ui64)
				return 1;
			if (attrA->value.ui64 < attrB->value.ui64)
				return -1;
			return 0;

		case B_STRING_TYPE:
			return strcmp(attrA->value.string, attrB->value.string);

		case B_RAW_TYPE:
			if (attrA->value.raw.length != attrB->value.raw.length)
				return -1;

			return memcmp(attrA->value.raw.data, attrB->value.raw.data,
				attrA->value.raw.length);
	}

	return -1;
}


//	#pragma mark - Device


Device::Device(device_node* node, const char* path, const char* moduleName)
	:
	fNode(node),
	fInitialized(0),
	fDeviceModule(NULL),
	fDeviceData(NULL)
{
	fPath = strdup(path);
	fModuleName = strdup(moduleName);
}


Device::~Device()
{
	free((char*)fPath);
	free((char*)fModuleName);
}


status_t
Device::InitCheck() const
{
	return fPath != NULL && fModuleName != NULL ? B_OK : B_NO_MEMORY;
}


status_t
Device::InitDevice()
{
	if ((fNode->Flags() & NODE_FLAG_DEVICE_REMOVED) != 0) {
		// TODO: maybe the device should be unlinked in devfs, too
		return ENODEV;
	}
	if ((fNode->Flags() & NODE_FLAG_WAITING_FOR_DRIVER) != 0)
		return B_BUSY;

	if (fInitialized++ > 0) {
		fNode->InitDriver();
			// acquire another reference to our parent as well
		return B_OK;
	}

	status_t status = get_module(ModuleName(), (module_info**)&fDeviceModule);
	if (status == B_OK) {
		// our parent always has to be initialized
		status = fNode->InitDriver();
	}
	if (status < B_OK) {
		fInitialized--;
		return status;
	}

	if (fDeviceModule->init_device != NULL)
		status = fDeviceModule->init_device(fNode->DriverData(), &fDeviceData);

	if (status < B_OK) {
		fNode->UninitDriver();
		fInitialized--;

		put_module(ModuleName());
		fDeviceModule = NULL;
		fDeviceData = NULL;
	}

	return status;
}


void
Device::UninitDevice()
{
	if (fInitialized-- > 1) {
		fNode->UninitDriver();
		return;
	}

	TRACE(("uninit driver for node %p\n", this));

	if (fDeviceModule->uninit_device != NULL)
		fDeviceModule->uninit_device(fDeviceData);

	fDeviceModule = NULL;
	fDeviceData = NULL;

	put_module(ModuleName());

	fNode->UninitDriver();
}


//	#pragma mark - device_node


/*!	Allocate device node info structure;
	initially, ref_count is one to make sure node won't get destroyed by mistake
*/
device_node::device_node(const char* moduleName, const device_attr* attrs,
	const io_resource* resources)
{
	fModuleName = strdup(moduleName);
	if (fModuleName == NULL)
		return;

	fParent = NULL;
	fRefCount = 1;
	fInitialized = 0;
	fRegistered = false;
	fFlags = 0;
	fSupportsParent = 0.0;
	fLastUpdateCycle = 0;
	fDriver = NULL;
	fDriverData = NULL;

	// copy attributes

	while (attrs != NULL && attrs->name != NULL) {
		device_attr_private* attr
			= new(std::nothrow) device_attr_private(*attrs);
		if (attr == NULL)
			break;

		fAttributes.Add(attr);
		attrs++;
	}

	get_attr_uint32(this, B_DEVICE_FLAGS, &fFlags, false);
	fFlags &= NODE_FLAG_PUBLIC_MASK;
}


device_node::~device_node()
{
	TRACE(("delete node %p\n", this));
	ASSERT(DriverModule() == NULL);

	if (Parent() != NULL) {
		if ((fFlags & NODE_FLAG_OBSOLETE_DRIVER) != 0) {
			// This driver has been obsoleted; another driver has been waiting
			// for us - make it available
			Parent()->_ReleaseWaiting();
		}
		Parent()->RemoveChild(this);
	}

	// Delete children
	NodeList::Iterator nodeIterator = fChildren.GetIterator();
	while (nodeIterator.HasNext()) {
		device_node* child = nodeIterator.Next();
		nodeIterator.Remove();
		delete child;
	}

	// Delete devices
	DeviceList::Iterator deviceIterator = fDevices.GetIterator();
	while (deviceIterator.HasNext()) {
		Device* device = deviceIterator.Next();
		deviceIterator.Remove();
		// TODO: unpublish!
		delete device;
	}

	// Delete attributes
	AttributeList::Iterator attrIterator = fAttributes.GetIterator();
	while (attrIterator.HasNext()) {
		device_attr_private* attr = attrIterator.Next();
		attrIterator.Remove();
		delete attr;
	}

	free((char*)fModuleName);
}


status_t
device_node::InitCheck() const
{
	return fModuleName != NULL ? B_OK : B_NO_MEMORY;
}


status_t
device_node::InitDriver()
{
	if (fInitialized++ > 0) {
		if (Parent() != NULL) {
			Parent()->InitDriver();
				// acquire another reference to our parent as well
		}
		Acquire();
		return B_OK;
	}

	status_t status = get_module(ModuleName(), (module_info**)&fDriver);
	if (status == B_OK && Parent() != NULL) {
		// our parent always has to be initialized
		status = Parent()->InitDriver();
	}
	if (status < B_OK) {
		fInitialized--;
		return status;
	}

	if (fDriver->init_driver != NULL)
		status = fDriver->init_driver(this, &fDriverData);

	if (status < B_OK) {
		if (Parent() != NULL)
			Parent()->UninitDriver();
		fInitialized--;

		put_module(ModuleName());
		fDriver = NULL;
		fDriverData = NULL;
		return status;
	}

	Acquire();
	return B_OK;
}


bool
device_node::UninitDriver()
{
	if (fInitialized-- > 1) {
		if (Parent() != NULL)
			Parent()->UninitDriver();
		Release();
		return false;
	}

	TRACE(("uninit driver for node %p\n", this));

	if (fDriver->uninit_driver != NULL)
		fDriver->uninit_driver(fDriverData);

	fDriver = NULL;
	fDriverData = NULL;

	put_module(ModuleName());

	if (Parent() != NULL)
		Parent()->UninitDriver();
	Release();

	return true;
}


void
device_node::AddChild(device_node* node)
{
	// we must not be destroyed	as long as we have children
	Acquire();
	node->fParent = this;
	fChildren.Add(node);
}


void
device_node::RemoveChild(device_node* node)
{
	node->fParent = NULL;
	fChildren.Remove(node);
	Release();
}


/*!	Registers this node, and all of its children that have to be registered.
	Also initializes the driver and keeps it that way on return in case
	it returns successfully.
*/
status_t
device_node::Register(device_node* parent)
{
	// make it public
	if (parent != NULL)
		parent->AddChild(this);
	else
		sRootNode = this;

	status_t status = InitDriver();
	if (status != B_OK)
		return status;

	if ((fFlags & B_KEEP_DRIVER_LOADED) != 0) {
		// We keep this driver loaded by having it always initialized
		InitDriver();
	}

	fFlags |= NODE_FLAG_REGISTER_INITIALIZED;
		// We don't uninitialize the driver - this is done by the caller
		// in order to save reinitializing during driver loading.

	uint32 registered;
	status = _RegisterFixed(registered);
	if (status != B_OK) {
		UninitUnusedDriver();
		return status;
	}
	if (registered > 0) {
		fRegistered = true;
		return B_OK;
	}

	// Register the children the driver wants

	if (DriverModule()->register_child_devices != NULL) {
		status = DriverModule()->register_child_devices(this);
		if (status != B_OK) {
			UninitUnusedDriver();
			return status;
		}

		if (!fChildren.IsEmpty()) {
			fRegistered = true;
			return B_OK;
		}
	}

	// Register all possible child device nodes

	status = _RegisterDynamic();
	if (status == B_OK)
		fRegistered = true;
	else
		UninitUnusedDriver();

	return status;
}


/*!	Registers any children that are identified via the B_DRIVER_FIXED_CHILD
	attribute.
	If any of these children cannot be registered, this call will fail (we
	don't remove children we already registered up to this point in this case).
*/
status_t
device_node::_RegisterFixed(uint32& registered)
{
	AttributeList::Iterator iterator = fAttributes.GetIterator();
	registered = 0;

	while (iterator.HasNext()) {
		device_attr_private* attr = iterator.Next();
		if (strcmp(attr->name, B_DEVICE_FIXED_CHILD))
			continue;

		driver_module_info* driver;
		status_t status = get_module(attr->value.string,
			(module_info**)&driver);
		if (status != B_OK)
			return status;

		if (driver->register_device != NULL) {
			status = driver->register_device(this);
			if (status == B_OK)
				registered++;
		}

		put_module(attr->value.string);

		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
device_node::_AddPath(Stack<KPath*>& stack, const char* basePath,
	const char* subPath)
{
	KPath* path = new(std::nothrow) KPath;
	if (path == NULL)
		return B_NO_MEMORY;

	status_t status = path->SetTo(basePath);
	if (status == B_OK && subPath != NULL && subPath[0])
		status = path->Append(subPath);
	if (status == B_OK)
		status = stack.Push(path);

	if (status != B_OK)
		delete path;

	return status;
}


status_t
device_node::_GetNextDriverPath(void*& cookie, KPath& _path)
{
	Stack<KPath*>* stack = NULL;

	if (cookie == NULL) {
		// find all paths and add them
		stack = new(std::nothrow) Stack<KPath*>();
		if (stack == NULL)
			return B_NO_MEMORY;

		StackDeleter<KPath*> stackDeleter(stack);
		uint16 type = 0;
		uint16 subType = 0;
		uint16 interface = 0;
		get_attr_uint16(this, B_DEVICE_TYPE, &type, false);
		get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false);
		get_attr_uint16(this, B_DEVICE_INTERFACE, &interface, false);

		// TODO: maybe make this extendible via settings file?
		switch (type) {
			case PCI_mass_storage:
				switch (subType) {
					case PCI_scsi:
						_AddPath(*stack, "busses", "scsi");
						break;
					case PCI_ide:
						_AddPath(*stack, "busses", "ide");
						break;
					case PCI_sata:
						_AddPath(*stack, "busses", "sata");
						break;
					default:
						_AddPath(*stack, "busses", "disk");
						break;
				}
				break;
			case PCI_serial_bus:
				switch (subType) {
					case PCI_firewire:
						_AddPath(*stack, "busses", "firewire");
						break;
					case PCI_usb:
						_AddPath(*stack, "busses", "usb");
						break;
					default:
						_AddPath(*stack, "busses");
						break;
				}
				break;
			case PCI_network:
				_AddPath(*stack, "drivers", "net");
				break;
			case PCI_display:
				_AddPath(*stack, "drivers", "graphics");
				break;
			case PCI_multimedia:
				switch (subType) {
					case PCI_audio:
					case PCI_hd_audio:
						_AddPath(*stack, "drivers", "audio");
						break;
					case PCI_video:
						_AddPath(*stack, "drivers", "video");
						break;
					default:
						_AddPath(*stack, "drivers");
						break;
				}
				break;
			default:
				if (sRootNode == this) {
					_AddPath(*stack, "busses/pci");
					_AddPath(*stack, "bus_managers");
				} else
					_AddPath(*stack, "drivers");
				break;
		}

		stackDeleter.Detach();

		cookie = (void*)stack;
	} else
		stack = static_cast<Stack<KPath*>*>(cookie);

	KPath* path;
	if (stack->Pop(&path)) {
		_path.Adopt(*path);
		delete path;
		return B_OK;
	}

	delete stack;
	return B_ENTRY_NOT_FOUND;
}


status_t
device_node::_GetNextDriver(void* list, driver_module_info*& driver)
{
	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);

		status_t status = read_next_module_name(list, name, &nameLength);
		if (status != B_OK)
			return status;

		if (!strcmp(fModuleName, name))
			continue;

		if (get_module(name, (module_info**)&driver) != B_OK)
			continue;

		if (driver->supports_device == NULL
			|| driver->register_device == NULL) {
			put_module(name);
			continue;
		}

		return B_OK;
	}
}


status_t
device_node::_FindBestDriver(const char* path, driver_module_info*& bestDriver,
	float& bestSupport, device_node* previous)
{
	if (bestDriver == NULL)
		bestSupport = previous != NULL ? previous->fSupportsParent : 0.0f;

	void* list = open_module_list_etc(path, "driver_v1");
	driver_module_info* driver;
	while (_GetNextDriver(list, driver) == B_OK) {
		if (previous != NULL && driver == previous->DriverModule()) {
			put_module(driver->info.name);
			continue;
		}

		float support = driver->supports_device(this);
		if (support > bestSupport) {
			if (bestDriver != NULL)
				put_module(bestDriver->info.name);

			bestDriver = driver;
			bestSupport = support;
			continue;
				// keep reference to best module around
		}

		put_module(driver->info.name);
	}
	close_module_list(list);

	return bestDriver != NULL ? B_OK : B_ENTRY_NOT_FOUND;
}


status_t
device_node::_RegisterPath(const char* path)
{
	void* list = open_module_list_etc(path, "driver_v1");
	driver_module_info* driver;
	uint32 count = 0;

	while (_GetNextDriver(list, driver) == B_OK) {
		float support = driver->supports_device(this);
		if (support > 0.0) {
			TRACE(("  register module \"%s\", support %f\n", driver->info.name,
				support));
			if (driver->register_device(this) == B_OK)
				count++;
		}

		put_module(driver->info.name);
	}
	close_module_list(list);

	return count > 0 ? B_OK : B_ENTRY_NOT_FOUND;
}


bool
device_node::_AlwaysRegisterDynamic()
{
	uint16 type = 0;
	uint16 subType = 0;
	get_attr_uint16(this, B_DEVICE_TYPE, &type, false);
	get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false);

	return type == PCI_serial_bus || type == PCI_bridge;
		// TODO: we may want to be a bit more specific in the future
}


status_t
device_node::_RegisterDynamic(device_node* previous)
{
	// If this is our initial registration, we honour the B_FIND_CHILD_ON_DEMAND
	// requirements
	if (!fRegistered && (fFlags & B_FIND_CHILD_ON_DEMAND) != 0
		&& !_AlwaysRegisterDynamic())
		return B_OK;

	KPath path;

	if ((fFlags & B_FIND_MULTIPLE_CHILDREN) == 0) {
		// find the one driver
		driver_module_info* bestDriver = NULL;
		float bestSupport = 0.0;
		void* cookie = NULL;

		while (_GetNextDriverPath(cookie, path) == B_OK) {
			_FindBestDriver(path.Path(), bestDriver, bestSupport, previous);
		}

		if (bestDriver != NULL) {
			TRACE(("  register best module \"%s\", support %f\n",
				bestDriver->info.name, bestSupport));
			if (bestDriver->register_device(this) == B_OK) {
				// There can only be one node of this driver
				// (usually only one at all, but there might be a new driver
				// "waiting" for its turn)
				device_node* child = FindChild(bestDriver->info.name);
				if (child != NULL) {
					child->fSupportsParent = bestSupport;
					if (previous != NULL) {
						previous->fFlags |= NODE_FLAG_OBSOLETE_DRIVER;
						previous->Release();
						child->fFlags |= NODE_FLAG_WAITING_FOR_DRIVER;
					}
				}
				// TODO: if this fails, we could try the second best driver,
				// and so on...
			}
			put_module(bestDriver->info.name);
		}
	} else {
		// register all drivers that match
		void* cookie = NULL;
		while (_GetNextDriverPath(cookie, path) == B_OK) {
			_RegisterPath(path.Path());
		}
	}

	return B_OK;
}


void
device_node::_ReleaseWaiting()
{
	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		child->fFlags &= ~NODE_FLAG_WAITING_FOR_DRIVER;
	}
}


status_t
device_node::_RemoveChildren()
{
	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();
		child->Release();
	}

	return fChildren.IsEmpty() ? B_OK : B_BUSY;
}


device_node*
device_node::_FindCurrentChild()
{
	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		if ((child->Flags() & NODE_FLAG_WAITING_FOR_DRIVER) == 0)
			return child;
	}

	return NULL;
}


status_t
device_node::Probe(const char* devicePath, uint32 updateCycle)
{
	if ((fFlags & NODE_FLAG_DEVICE_REMOVED) != 0
		|| updateCycle == fLastUpdateCycle)
		return B_OK;

	status_t status = InitDriver();
	if (status < B_OK)
		return status;

	MethodDeleter<device_node, bool> uninit(this,
		&device_node::UninitDriver);

	uint16 type = 0;
	uint16 subType = 0;
	if (get_attr_uint16(this, B_DEVICE_TYPE, &type, false) == B_OK
		&& get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false)
			== B_OK) {
		// Check if this node matches the device path
		// TODO: maybe make this extendible via settings file?
		bool matches = false;
		if (!strcmp(devicePath, "disk")) {
			matches = type == PCI_mass_storage;
		} else if (!strcmp(devicePath, "audio")) {
			matches = type == PCI_multimedia
				&& (subType == PCI_audio || subType == PCI_hd_audio);
		} else if (!strcmp(devicePath, "net")) {
			matches = type == PCI_network;
		} else if (!strcmp(devicePath, "graphics")) {
			matches = type == PCI_display;
		} else if (!strcmp(devicePath, "video")) {
			matches = type == PCI_multimedia && subType == PCI_video;
		}

		if (matches) {
			device_node* previous = NULL;

			fLastUpdateCycle = updateCycle;
				// This node will be probed in this update cycle

			if (!fChildren.IsEmpty()
				&& (fFlags & B_FIND_MULTIPLE_CHILDREN) == 0) {
				// We already have a driver that claims this node; remove all
				// (unused) nodes, and evaluate it again
				_RemoveChildren();

				previous = _FindCurrentChild();
				if (previous != NULL) {
					// This driver is still active - give it back the reference
					// that was stolen by _RemoveChildren() - _RegisterDynamic()
					// will release it, if it really isn't needed anymore
					previous->Acquire();
				}
			}
			return _RegisterDynamic(previous);
		}

		return B_OK;
	}

	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		status = child->Probe(devicePath, updateCycle);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


/*!	Uninitializes all temporary references to the driver. The registration
	process keeps the driver initialized to optimize the startup procedure;
	this function gives this reference away again.
*/
void
device_node::UninitUnusedDriver()
{
	// First, we need to go to the leaf, and go back from there

	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		child->UninitUnusedDriver();
	}

	if (!IsInitialized()
		|| (fFlags & NODE_FLAG_REGISTER_INITIALIZED) == 0)
		return;

	fFlags &= ~NODE_FLAG_REGISTER_INITIALIZED;

	UninitDriver();
}


/*!	Calls device_removed() on this node and all of its children - starting
	with the deepest and last child.
	It will also remove the one reference that every node gets on its creation.
*/
void
device_node::DeviceRemoved()
{
	// notify children
	NodeList::ConstIterator iterator = Children().GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		child->DeviceRemoved();
	}

	// notify devices
	DeviceList::ConstIterator deviceIterator = Devices().GetIterator();
	while (deviceIterator.HasNext()) {
		Device* device = deviceIterator.Next();

		if (device->DeviceModule() != NULL
			&& device->DeviceModule()->device_removed != NULL)
			device->DeviceModule()->device_removed(device->DeviceData());
	}

	fFlags |= NODE_FLAG_DEVICE_REMOVED;

	if (IsInitialized() && DriverModule()->device_removed != NULL)
		DriverModule()->device_removed(this);

	if ((fFlags & B_KEEP_DRIVER_LOADED) != 0) {
		// There is no point in keeping this driver loaded when its device
		// is gone
		UninitDriver();
	}

	UninitUnusedDriver();
	Release();
}


void
device_node::Acquire()
{
	atomic_add(&fRefCount, 1);
}


bool
device_node::Release()
{
	if (atomic_add(&fRefCount, -1) > 1)
		return false;

	delete this;
	return true;
}


void
device_node::AddDevice(Device* device)
{
	fDevices.Add(device);
}


void
device_node::RemoveDevice(Device* device)
{
	fDevices.Remove(device);
}


int
device_node::CompareTo(const device_attr* attributes) const
{
	if (attributes == NULL)
		return -1;

	for (; attributes->name != NULL; attributes++) {
		// find corresponding attribute
		AttributeList::ConstIterator iterator = Attributes().GetIterator();
		device_attr_private* attr = NULL;
		while (iterator.HasNext()) {
			attr = iterator.Next();

			if (!strcmp(attr->name, attributes->name))
				break;
		}
		if (!iterator.HasNext())
			return -1;

		int compare = device_attr_private::Compare(attr, attributes);
		if (compare != 0)
			return compare;
	}

	return 0;
}


device_node*
device_node::FindChild(const device_attr* attributes) const
{
	if (attributes == NULL)
		return NULL;

	NodeList::ConstIterator iterator = Children().GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		// ignore nodes that are pending to be removed
		if ((child->Flags() & NODE_FLAG_DEVICE_REMOVED) == 0
			&& !child->CompareTo(attributes))
			return child;
	}

	return NULL;
}


device_node*
device_node::FindChild(const char* moduleName) const
{
	if (moduleName == NULL)
		return NULL;

	NodeList::ConstIterator iterator = Children().GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		if (!strcmp(child->ModuleName(), moduleName))
			return child;
	}

	return NULL;
}


void
device_node::Dump(int32 level = 0)
{
	put_level(level);
	dprintf("(%ld) @%p \"%s\" (ref %ld, init %ld)\n", level, this, ModuleName(),
		fRefCount, fInitialized);

	AttributeList::Iterator attribute = Attributes().GetIterator();
	while (attribute.HasNext()) {
		dump_attribute(attribute.Next(), level);
	}

	NodeList::ConstIterator iterator = Children().GetIterator();
	while (iterator.HasNext()) {
		iterator.Next()->Dump(level + 1);
	}
}


//	#pragma mark - root node


static void
init_root_node(void)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Devices Root"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "root"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE,
			{ui32: B_FIND_MULTIPLE_CHILDREN | B_KEEP_DRIVER_LOADED }},
		{NULL}
	};

	if (register_node(NULL, DEVICE_MANAGER_ROOT_NAME, attrs, NULL, NULL)
			!= B_OK) {
		dprintf("Cannot register Devices Root Node\n");
	}
}


static driver_module_info sDeviceRootModule = {
	{
		DEVICE_MANAGER_ROOT_NAME,
		0,
		NULL,
	},
	NULL
};


//	#pragma mark -


int
main(int argc, char** argv)
{
	_add_builtin_module((module_info*)&sDeviceManagerModule);
	_add_builtin_module((module_info*)&sDeviceRootModule);

	// bus
	_add_builtin_module((module_info*)&gBusModuleInfo);
	_add_builtin_module((module_info*)&gBusDriverModuleInfo);

	// sample driver
	_add_builtin_module((module_info*)&gDriverModuleInfo);
	_add_builtin_module((module_info*)&gDeviceModuleInfo);

	// generic video driver
	_add_builtin_module((module_info*)&gGenericVideoDriverModuleInfo);
	_add_builtin_module((module_info*)&gGenericVideoDeviceModuleInfo);

	gDeviceManager = &sDeviceManagerModule;

	status_t status = _get_builtin_dependencies();
	if (status < B_OK) {
		fprintf(stderr, "device_manager: Could not initialize modules: %s\n",
			strerror(status));
		return 1;
	}

	recursive_lock_init(&sLock, "device manager");

	init_root_node();
	sRootNode->Dump();

	probe_path("net");
	probe_path("graphics");

	void* netHandle = open_path("net/sample/0");

	uninit_unused();

	puts("remove net driver");
	device_node* busNode = sRootNode->FindChild(BUS_MODULE_NAME);
	bus_trigger_device_removed(busNode);

	close_path(netHandle);
		// the net nodes must be removed with this call

	void* graphicsHandle = open_path("graphics/generic/0");

	// add specific video driver - ie. simulate installing it
	_add_builtin_module((module_info*)&gSpecificVideoDriverModuleInfo);
	_add_builtin_module((module_info*)&gSpecificVideoDeviceModuleInfo);
	sDriverUpdateCycle++;
	probe_path("graphics");

	open_path("graphics/specific/0");
		// this will fail

	close_path(graphicsHandle);
		// the graphics drivers must be switched with this call

	graphicsHandle = open_path("graphics/specific/0");
	close_path(graphicsHandle);

	uninit_unused();

	recursive_lock_destroy(&sLock);
	return 0;
}
