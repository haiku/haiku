/*
 * Copyright 2008-2009, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <kdevice_manager.h>

#include <new>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <KernelExport.h>
#include <Locker.h>
#include <module.h>
#include <PCI.h>

#include <boot_device.h>
#include <device_manager_defs.h>
#include <fs/devfs.h>
#include <fs/KPath.h>
#include <kernel.h>
#include <generic_syscall.h>
#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>
#include <util/Stack.h>

#include "AbstractModuleDevice.h"
#include "devfs_private.h"
#include "id_generator.h"
#include "IORequest.h"
#include "io_resources.h"
#include "IOSchedulerRoster.h"


//#define TRACE_DEVICE_MANAGER
#ifdef TRACE_DEVICE_MANAGER
#	define TRACE(a) dprintf a
#else
#	define TRACE(a) ;
#endif


#define DEVICE_MANAGER_ROOT_NAME "system/devices_root/driver_v1"
#define DEVICE_MANAGER_GENERIC_NAME "system/devices_generic/driver_v1"


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


namespace {


class Device : public AbstractModuleDevice,
	public DoublyLinkedListLinkImpl<Device> {
public:
							Device(device_node* node, const char* moduleName);
	virtual					~Device();

			status_t		InitCheck() const;

			const char*		ModuleName() const { return fModuleName; }

	virtual	status_t		InitDevice();
	virtual	void			UninitDevice();

	virtual void			Removed();

			void			SetRemovedFromParent(bool removed)
								{ fRemovedFromParent = removed; }

private:
	const char*				fModuleName;
	bool					fRemovedFromParent;
};


} // unnamed namespace


typedef DoublyLinkedList<Device> DeviceList;
typedef DoublyLinkedList<device_node> NodeList;

struct device_node : DoublyLinkedListLinkImpl<device_node> {
							device_node(const char* moduleName,
								const device_attr* attrs);
							~device_node();

			status_t		InitCheck() const;

			status_t		AcquireResources(const io_resource* resources);

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
			status_t		Reprobe();
			status_t		Rescan();

			bool			IsRegistered() const { return fRegistered; }
			bool			IsInitialized() const { return fInitialized > 0; }
			bool			IsProbed() const { return fLastUpdateCycle != 0; }
			uint32			Flags() const { return fFlags; }

			void			Acquire();
			bool			Release();

			const DeviceList& Devices() const { return fDevices; }
			void			AddDevice(Device* device);
			void			RemoveDevice(Device* device);

			int				CompareTo(const device_attr* attributes) const;
			device_node*	FindChild(const device_attr* attributes) const;
			device_node*	FindChild(const char* moduleName) const;

			int32			Priority();

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
			status_t		_Probe();
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
	ResourceList			fResources;
};

// flags in addition to those specified by B_DEVICE_FLAGS
enum node_flags {
	NODE_FLAG_REGISTER_INITIALIZED	= 0x00010000,
	NODE_FLAG_DEVICE_REMOVED		= 0x00020000,
	NODE_FLAG_OBSOLETE_DRIVER		= 0x00040000,
	NODE_FLAG_WAITING_FOR_DRIVER	= 0x00080000,

	NODE_FLAG_PUBLIC_MASK			= 0x0000ffff
};


static device_node *sRootNode;
static recursive_lock sLock;
static const char* sGenericContextPath;


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
		kprintf("   ");
}


static void
dump_attribute(device_attr* attr, int32 level)
{
	if (attr == NULL)
		return;

	put_level(level + 2);
	kprintf("\"%s\" : ", attr->name);
	switch (attr->type) {
		case B_STRING_TYPE:
			kprintf("string : \"%s\"", attr->value.string);
			break;
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			kprintf("uint8 : %" B_PRIu8 " (%#" B_PRIx8 ")", attr->value.ui8,
				attr->value.ui8);
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			kprintf("uint16 : %" B_PRIu16 " (%#" B_PRIx16 ")", attr->value.ui16,
				attr->value.ui16);
			break;
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			kprintf("uint32 : %" B_PRIu32 " (%#" B_PRIx32 ")", attr->value.ui32,
				attr->value.ui32);
			break;
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
			kprintf("uint64 : %" B_PRIu64 " (%#" B_PRIx64 ")", attr->value.ui64,
				attr->value.ui64);
			break;
		default:
			kprintf("raw data");
	}
	kprintf("\n");
}


static int
dump_io_scheduler(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	IOScheduler* scheduler = (IOScheduler*)parse_expression(argv[1]);
	scheduler->Dump();
	return 0;
}


static int
dump_io_request_owner(int argc, char** argv)
{
	if (argc != 2) {
		print_debugger_command_usage(argv[0]);
		return 0;
	}

	IORequestOwner* owner = (IORequestOwner*)parse_expression(argv[1]);
	owner->Dump();
	return 0;
}


static int
dump_io_request(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-request>\n", argv[0]);
		return 0;
	}

	IORequest* request = (IORequest*)parse_expression(argv[1]);
	request->Dump();
	return 0;
}


static int
dump_io_operation(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-operation>\n", argv[0]);
		return 0;
	}

	IOOperation* operation = (IOOperation*)parse_expression(argv[1]);
	operation->Dump();
	return 0;
}


static int
dump_io_buffer(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-io-buffer>\n", argv[0]);
		return 0;
	}

	IOBuffer* buffer = (IOBuffer*)parse_expression(argv[1]);
	buffer->Dump();
	return 0;
}


static int
dump_dma_buffer(int argc, char** argv)
{
	if (argc != 2 || !strcmp(argv[1], "--help")) {
		kprintf("usage: %s <ptr-to-dma-buffer>\n", argv[0]);
		return 0;
	}

	DMABuffer* buffer = (DMABuffer*)parse_expression(argv[1]);
	buffer->Dump();
	return 0;
}


static int
dump_device_nodes(int argc, char** argv)
{
	sRootNode->Dump();
	return 0;
}


static void
publish_directories(const char* subPath)
{
	if (gBootDevice < 0) {
		if (subPath[0]) {
			// we only support the top-level directory for modules
			return;
		}

		// we can only iterate over the known modules to find all directories
		KPath path("drivers");
		if (path.Append(subPath) != B_OK)
			return;

		size_t length = strlen(path.Path()) + 1;
			// account for the separating '/'

		void* list = open_module_list_etc(path.Path(), "driver_v1");
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);
		while (read_next_module_name(list, name, &nameLength) == B_OK) {
			if (nameLength == length)
				continue;

			char* leaf = name + length;
			char* end = strchr(leaf, '/');
			if (end != NULL)
				end[0] = '\0';

			path.SetTo(subPath);
			path.Append(leaf);

			devfs_publish_directory(path.Path());
		}
		close_module_list(list);
	} else {
		// TODO: implement module directory traversal!
	}
}


static status_t
control_device_manager(const char* subsystem, uint32 function, void* buffer,
	size_t bufferSize)
{
	// TODO: this function passes pointers to userland, and uses pointers
	// to device nodes that came from userland - this is completely unsafe
	// and should be changed.
	switch (function) {
		case DM_GET_ROOT:
		{
			device_node_cookie cookie;
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize != sizeof(device_node_cookie))
				return B_BAD_VALUE;
			cookie = (device_node_cookie)sRootNode;

			// copy back to user space
			return user_memcpy(buffer, &cookie, sizeof(device_node_cookie));
		}

		case DM_GET_CHILD:
		{
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize != sizeof(device_node_cookie))
				return B_BAD_VALUE;

			device_node_cookie cookie;
			if (user_memcpy(&cookie, buffer, sizeof(device_node_cookie)) < B_OK)
				return B_BAD_ADDRESS;

			device_node* node = (device_node*)cookie;
			NodeList::ConstIterator iterator = node->Children().GetIterator();

			if (!iterator.HasNext()) {
				return B_ENTRY_NOT_FOUND;
			}
			node = iterator.Next();
			cookie = (device_node_cookie)node;

			// copy back to user space
			return user_memcpy(buffer, &cookie, sizeof(device_node_cookie));
		}

		case DM_GET_NEXT_CHILD:
		{
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize != sizeof(device_node_cookie))
				return B_BAD_VALUE;

			device_node_cookie cookie;
			if (user_memcpy(&cookie, buffer, sizeof(device_node_cookie)) < B_OK)
				return B_BAD_ADDRESS;

			device_node* last = (device_node*)cookie;
			if (!last->Parent())
				return B_ENTRY_NOT_FOUND;

			NodeList::ConstIterator iterator
				= last->Parent()->Children().GetIterator();

			// skip those we already traversed
			while (iterator.HasNext()) {
				device_node* node = iterator.Next();

				if (node == last)
					break;
			}

			if (!iterator.HasNext())
				return B_ENTRY_NOT_FOUND;
			device_node* node = iterator.Next();
			cookie = (device_node_cookie)node;

			// copy back to user space
			return user_memcpy(buffer, &cookie, sizeof(device_node_cookie));
		}

		case DM_GET_NEXT_ATTRIBUTE:
		{
			struct device_attr_info attrInfo;
			if (!IS_USER_ADDRESS(buffer))
				return B_BAD_ADDRESS;
			if (bufferSize != sizeof(device_attr_info))
				return B_BAD_VALUE;
			if (user_memcpy(&attrInfo, buffer, sizeof(device_attr_info)) < B_OK)
				return B_BAD_ADDRESS;

			device_node* node = (device_node*)attrInfo.node_cookie;
			device_attr* last = (device_attr*)attrInfo.cookie;
			AttributeList::Iterator iterator = node->Attributes().GetIterator();
			// skip those we already traversed
			while (iterator.HasNext() && last != NULL) {
				device_attr* attr = iterator.Next();

				if (attr == last)
					break;
			}

			if (!iterator.HasNext()) {
				attrInfo.cookie = 0;
				return B_ENTRY_NOT_FOUND;
			}

			device_attr* attr = iterator.Next();
			attrInfo.cookie = (device_node_cookie)attr;
			strlcpy(attrInfo.name, attr->name, 254);
			attrInfo.type = attr->type;
			switch (attrInfo.type) {
				case B_UINT8_TYPE:
					attrInfo.value.ui8 = attr->value.ui8;
					break;
				case B_UINT16_TYPE:
					attrInfo.value.ui16 = attr->value.ui16;
					break;
				case B_UINT32_TYPE:
					attrInfo.value.ui32 = attr->value.ui32;
					break;
				case B_UINT64_TYPE:
					attrInfo.value.ui64 = attr->value.ui64;
					break;
				case B_STRING_TYPE:
					strlcpy(attrInfo.value.string, attr->value.string, 254);
					break;
				/*case B_RAW_TYPE:
					if (attr.value.raw.length > attr_info->attr.value.raw.length)
						attr.value.raw.length = attr_info->attr.value.raw.length;
					user_memcpy(attr.value.raw.data, attr_info->attr.value.raw.data,
						attr.value.raw.length);
					break;*/
			}

			// copy back to user space
			return user_memcpy(buffer, &attrInfo, sizeof(device_attr_info));
		}
	}

	return B_BAD_HANDLER;
}


//	#pragma mark - Device Manager module API


static status_t
rescan_node(device_node* node)
{
	RecursiveLocker _(sLock);
	return node->Rescan();
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

	RecursiveLocker _(sLock);

	device_node* newNode = new(std::nothrow) device_node(moduleName, attrs);
	if (newNode == NULL)
		return B_NO_MEMORY;

	TRACE(("%p: register node \"%s\", parent %p\n", newNode, moduleName,
		parent));

	status_t status = newNode->InitCheck();
	if (status == B_OK)
		status = newNode->AcquireResources(ioResources);
	if (status == B_OK)
		status = newNode->Register(parent);

	if (status != B_OK) {
		newNode->Release();
		return status;
	}

	if (_node)
		*_node = newNode;

	return B_OK;
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

	Device* device = new(std::nothrow) Device(node, moduleName);
	if (device == NULL)
		return B_NO_MEMORY;

	status_t status = device->InitCheck();
	if (status == B_OK)
		status = devfs_publish_device(path, device);
	if (status != B_OK) {
		delete device;
		return status;
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

#if 0
	DeviceList::ConstIterator iterator = node->Devices().GetIterator();
	while (iterator.HasNext()) {
		Device* device = iterator.Next();
		if (!strcmp(device->Path(), path)) {
			node->RemoveDevice(device);
			delete device;
			return B_OK;
		}
	}
#endif

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


struct device_manager_info gDeviceManagerModule = {
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

	// I/O resources

	// ID generator
	dm_create_id,
	dm_free_id,

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


Device::Device(device_node* node, const char* moduleName)
	:
	fModuleName(strdup(moduleName)),
	fRemovedFromParent(false)
{
	fNode = node;
}


Device::~Device()
{
	free((char*)fModuleName);
}


status_t
Device::InitCheck() const
{
	return fModuleName != NULL ? B_OK : B_NO_MEMORY;
}


status_t
Device::InitDevice()
{
	RecursiveLocker _(sLock);

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

	if (Module()->init_device != NULL)
		status = Module()->init_device(fNode->DriverData(), &fDeviceData);

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
	RecursiveLocker _(sLock);

	if (fInitialized-- > 1) {
		fNode->UninitDriver();
		return;
	}

	TRACE(("uninit driver for node %p\n", this));

	if (Module()->uninit_device != NULL)
		Module()->uninit_device(fDeviceData);

	fDeviceModule = NULL;
	fDeviceData = NULL;

	put_module(ModuleName());

	fNode->UninitDriver();
}


void
Device::Removed()
{
	RecursiveLocker _(sLock);

	if (!fRemovedFromParent)
		fNode->RemoveDevice(this);

	delete this;
}


//	#pragma mark - device_node


device_node::device_node(const char* moduleName, const device_attr* attrs)
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
	while (device_node* child = fChildren.RemoveHead()) {
		delete child;
	}

	// Delete devices
	while (Device* device = fDevices.RemoveHead()) {
		device->SetRemovedFromParent(true);
		devfs_unpublish_device(device, true);
	}

	// Delete attributes
	while (device_attr_private* attr = fAttributes.RemoveHead()) {
		delete attr;
	}

	// Delete resources
	while (io_resource_private* resource = fResources.RemoveHead()) {
		delete resource;
	}

	free((char*)fModuleName);
}


status_t
device_node::InitCheck() const
{
	return fModuleName != NULL ? B_OK : B_NO_MEMORY;
}


status_t
device_node::AcquireResources(const io_resource* resources)
{
	if (resources == NULL)
		return B_OK;

	for (uint32 i = 0; resources[i].type != 0; i++) {
		io_resource_private* resource = new(std::nothrow) io_resource_private;
		if (resource == NULL)
			return B_NO_MEMORY;

		status_t status = resource->Acquire(resources[i]);
		if (status != B_OK) {
			delete resource;
			return status;
		}

		fResources.Add(resource);
	}

	return B_OK;
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

	if (fDriver->init_driver != NULL) {
		status = fDriver->init_driver(this, &fDriverData);
		if (status != B_OK) {
			dprintf("driver %s init failed: %s\n", ModuleName(),
				strerror(status));
		}
	}

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

	int32 priority = node->Priority();

	// Enforce an order in which the children are traversed - from most
	// specific to least specific child.
	NodeList::Iterator iterator = fChildren.GetIterator();
	device_node* before = NULL;
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();
		if (child->Priority() <= priority) {
			before = child;
			break;
		}
	}

	fChildren.Insert(before, node);
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

	uint32 registeredFixedCount;
	status = _RegisterFixed(registeredFixedCount);
	if (status != B_OK) {
		UninitUnusedDriver();
		return status;
	}

	// Register the children the driver wants

	if (DriverModule()->register_child_devices != NULL) {
		status = DriverModule()->register_child_devices(DriverData());
		if (status != B_OK) {
			UninitUnusedDriver();
			return status;
		}

		if (!fChildren.IsEmpty()) {
			fRegistered = true;
			return B_OK;
		}
	}

	if (registeredFixedCount > 0) {
		// Nodes with fixed children cannot have any dynamic children, so bail
		// out here
		fRegistered = true;
		return B_OK;
	}

	// Register all possible child device nodes

	status = _RegisterDynamic();
	if (status == B_OK)
		fRegistered = true;
	else
		UninitUnusedDriver();

	return status;
}


/*!	Registers any children that are identified via the B_DEVICE_FIXED_CHILD
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
		if (status != B_OK) {
			TRACE(("register fixed child %s failed: %s\n", attr->value.string,
				strerror(status)));
			return status;
		}

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

	TRACE(("  add path: \"%s\", %" B_PRId32 "\n", path->Path(), status));

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

		bool generic = false;
		uint16 type = 0;
		uint16 subType = 0;
		uint16 interface = 0;
		if (get_attr_uint16(this, B_DEVICE_TYPE, &type, false) != B_OK
			|| get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false)
					!= B_OK)
			generic = true;

		get_attr_uint16(this, B_DEVICE_INTERFACE, &interface, false);

		// TODO: maybe make this extendible via settings file?
		switch (type) {
			case PCI_mass_storage:
				switch (subType) {
					case PCI_scsi:
						_AddPath(*stack, "busses", "scsi");
						break;
					case PCI_ide:
						_AddPath(*stack, "busses", "ata");
						_AddPath(*stack, "busses", "ide");
						break;
					case PCI_sata:
						// TODO: check for ahci interface
						_AddPath(*stack, "busses", "scsi");
						_AddPath(*stack, "busses", "ata");
						_AddPath(*stack, "busses", "ide");
						break;
					default:
						_AddPath(*stack, "busses");
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
				} else if (!generic) {
					_AddPath(*stack, "drivers");
				} else {
					// For generic drivers, we only allow busses when the
					// request is more specified
					if (sGenericContextPath != NULL
						&& (!strcmp(sGenericContextPath, "disk")
							|| !strcmp(sGenericContextPath, "ports")
							|| !strcmp(sGenericContextPath, "bus"))) {
						_AddPath(*stack, "busses");
					}
					_AddPath(*stack, "drivers", sGenericContextPath);
				}
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
	// If this is not a bus, we don't have to scan it
	if (find_attr(this, B_DEVICE_BUS, false, B_STRING_TYPE) == NULL)
		return B_OK;

	// If we're not being probed, we honour the B_FIND_CHILD_ON_DEMAND
	// requirements
	if (!IsProbed() && (fFlags & B_FIND_CHILD_ON_DEMAND) != 0
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
device_node::_Probe()
{
	device_node* previous = NULL;

	if (IsProbed() && !fChildren.IsEmpty()
		&& (fFlags & (B_FIND_CHILD_ON_DEMAND | B_FIND_MULTIPLE_CHILDREN))
				== B_FIND_CHILD_ON_DEMAND) {
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

	if ((fFlags & B_FIND_CHILD_ON_DEMAND) != 0) {
		bool matches = false;
		uint16 type = 0;
		uint16 subType = 0;
		if (get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false) == B_OK
			&& get_attr_uint16(this, B_DEVICE_TYPE, &type, false) == B_OK) {
			// Check if this node matches the device path
			// TODO: maybe make this extendible via settings file?
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
		} else {
			// This driver does not support types, but still wants to its
			// children explored on demand only.
			matches = true;
			sGenericContextPath = devicePath;
		}

		if (matches) {
			fLastUpdateCycle = updateCycle;
				// This node will be probed in this update cycle

			status = _Probe();

			sGenericContextPath = NULL;
			return status;
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


status_t
device_node::Reprobe()
{
	status_t status = InitDriver();
	if (status < B_OK)
		return status;

	MethodDeleter<device_node, bool> uninit(this,
		&device_node::UninitDriver);

	// If this child has been probed already, probe it again
	status = _Probe();
	if (status != B_OK)
		return status;

	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		status = child->Reprobe();
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


status_t
device_node::Rescan()
{
	status_t status = InitDriver();
	if (status < B_OK)
		return status;

	MethodDeleter<device_node, bool> uninit(this,
		&device_node::UninitDriver);

	if (DriverModule()->rescan_child_devices != NULL) {
		status = DriverModule()->rescan_child_devices(DriverData());
		if (status != B_OK)
			return status;
	}

	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		status = child->Rescan();
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

		if (device->Module() != NULL
			&& device->Module()->device_removed != NULL)
			device->Module()->device_removed(device->Data());
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
		bool found = false;

		while (iterator.HasNext()) {
			attr = iterator.Next();

			if (!strcmp(attr->name, attributes->name)) {
				found = true;
				break;
			}
		}
		if (!found)
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


/*!	This returns the priority or importance of this node. Nodes with higher
	priority are registered/probed first.
	Currently, only the B_FIND_MULTIPLE_CHILDREN flag alters the priority;
	it might make sense to be able to directly set the priority via an
	attribute.
*/
int32
device_node::Priority()
{
	return (fFlags & B_FIND_MULTIPLE_CHILDREN) != 0 ? 0 : 100;
}


void
device_node::Dump(int32 level)
{
	put_level(level);
	kprintf("(%" B_PRId32 ") @%p \"%s\" (ref %" B_PRId32 ", init %" B_PRId32
		", module %p, data %p)\n", level, this, ModuleName(), fRefCount,
		fInitialized, DriverModule(), DriverData());

	AttributeList::Iterator attribute = Attributes().GetIterator();
	while (attribute.HasNext()) {
		dump_attribute(attribute.Next(), level);
	}

	DeviceList::Iterator deviceIterator = fDevices.GetIterator();
	while (deviceIterator.HasNext()) {
		Device* device = deviceIterator.Next();
		put_level(level);
		kprintf("device: %s, %p\n", device->ModuleName(), device->Data());
	}

	NodeList::ConstIterator iterator = Children().GetIterator();
	while (iterator.HasNext()) {
		iterator.Next()->Dump(level + 1);
	}
}


//	#pragma mark - root node


static void
init_node_tree(void)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Devices Root"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "root"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE,
			{ui32: B_FIND_MULTIPLE_CHILDREN | B_KEEP_DRIVER_LOADED }},
		{NULL}
	};

	device_node* node;
	if (register_node(NULL, DEVICE_MANAGER_ROOT_NAME, attrs, NULL, &node)
			!= B_OK) {
		dprintf("Cannot register Devices Root Node\n");
	}

	device_attr genericAttrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Generic"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "generic"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE, {ui32: B_FIND_MULTIPLE_CHILDREN
			| B_KEEP_DRIVER_LOADED | B_FIND_CHILD_ON_DEMAND}},
		{NULL}
	};

	if (register_node(node, DEVICE_MANAGER_GENERIC_NAME, genericAttrs, NULL,
			NULL) != B_OK) {
		dprintf("Cannot register Generic Devices Node\n");
	}
}


driver_module_info gDeviceRootModule = {
	{
		DEVICE_MANAGER_ROOT_NAME,
		0,
		NULL,
	},
};


driver_module_info gDeviceGenericModule = {
	{
		DEVICE_MANAGER_GENERIC_NAME,
		0,
		NULL,
	},
	NULL
};


//	#pragma mark - private kernel API


status_t
device_manager_probe(const char* path, uint32 updateCycle)
{
	TRACE(("device_manager_probe(\"%s\")\n", path));
	RecursiveLocker _(sLock);

	// first, publish directories in the driver directory
	publish_directories(path);

	return sRootNode->Probe(path, updateCycle);
}


status_t
device_manager_init(struct kernel_args* args)
{
	TRACE(("device manager init\n"));

	IOSchedulerRoster::Init();

	dm_init_id_generator();
	dm_init_io_resources();

	recursive_lock_init(&sLock, "device manager");
	init_node_tree();

	register_generic_syscall(DEVICE_MANAGER_SYSCALLS, control_device_manager,
		1, 0);

	add_debugger_command("dm_tree", &dump_device_nodes,
		"dump device node tree");
	add_debugger_command_etc("io_scheduler", &dump_io_scheduler,
		"Dump an I/O scheduler",
		"<scheduler>\n"
		"Dumps I/O scheduler at address <scheduler>.\n", 0);
	add_debugger_command_etc("io_request_owner", &dump_io_request_owner,
		"Dump an I/O request owner",
		"<owner>\n"
		"Dumps I/O request owner at address <owner>.\n", 0);
	add_debugger_command("io_request", &dump_io_request, "dump an I/O request");
	add_debugger_command("io_operation", &dump_io_operation,
		"dump an I/O operation");
	add_debugger_command("io_buffer", &dump_io_buffer, "dump an I/O buffer");
	add_debugger_command("dma_buffer", &dump_dma_buffer, "dump a DMA buffer");
	return B_OK;
}


status_t
device_manager_init_post_modules(struct kernel_args* args)
{
	RecursiveLocker _(sLock);
	return sRootNode->Reprobe();
}

