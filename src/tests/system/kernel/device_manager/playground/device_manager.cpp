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


#define TRACE(a) dprintf a

#define DEVICE_MANAGER_ROOT_NAME "system/devices_root/driver_v1"

extern struct device_module_info gDeviceModuleInfo;
extern struct driver_module_info gDriverModuleInfo;
extern struct driver_module_info gBusModuleInfo;
extern struct driver_module_info gBusDriverModuleInfo;

extern "C" status_t _add_builtin_module(module_info *info);
extern "C" status_t _get_builtin_dependencies(void);
extern bool gDebugOutputEnabled;
	// from libkernelland_emu.so

status_t dm_get_attr_uint8(const device_node* node, const char* name,
	uint8* _value, bool recursive);
status_t dm_get_attr_uint16(const device_node* node, const char* name,
	uint16* _value, bool recursive);
status_t dm_get_attr_uint32(const device_node* node, const char* name,
	uint32* _value, bool recursive);
status_t dm_get_attr_string(const device_node* node, const char* name,
	const char** _value, bool recursive);


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


// a structure to put nodes into lists
struct node_entry {
	struct list_link	link;
	device_node*		node;
};

typedef DoublyLinkedList<device_node> NodeList;

struct device_node : DoublyLinkedListLinkImpl<device_node> {
							device_node(const char *moduleName,
								const device_attr *attrs,
								const io_resource *resources);
							~device_node();

			status_t		InitCheck();

			const char*		ModuleName() const { return fModuleName; }
			device_node*	Parent() const { return fParent; }
			AttributeList&	Attributes() { return fAttributes; }
			const AttributeList& Attributes() const { return fAttributes; }

			status_t		InitDriver();
			bool			UninitDriver();

			// The following two are only valid, if the node's driver is
			// initialized
			driver_module_info* DriverModule() const { return fDriver; }
			void*			DriverData() const { return fDriverData; }

			void			AddChild(device_node *node);
			void			RemoveChild(device_node *node);
			const NodeList&	Children() const { return fChildren; }
			void			UninitUnusedChildren();

			status_t		Register();
			status_t		Probe(const char* devicePath);
			bool			IsRegistered() const { return fRegistered; }
			bool			IsInitialized() const { return fInitialized > 0; }

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
								float& bestSupport);
			status_t		_RegisterPath(const char* path);
			status_t		_RegisterDynamic();
			status_t		_RemoveChildren();
			bool			_UninitUnusedChildren();

	device_node*			fParent;
	NodeList				fChildren;
	int32					fRefCount;
	int32					fInitialized;
	bool					fRegistered;
	uint32					fFlags;

	const char*				fModuleName;
	driver_module_info*		fDriver;
	void*					fDriverData;

	AttributeList			fAttributes;
};

enum node_flags {
	NODE_FLAG_REMOVE_ON_UNINIT	= 0x01
};

device_manager_info *gDeviceManager;

static device_node *sRootNode;
static recursive_lock sLock;


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


//	#pragma mark -


device_attr_private*
dm_find_attr(const device_node* node, const char* name, bool recursive,
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


void
dm_dump_node(device_node* node, int32 level)
{
	if (node == NULL)
		return;

	put_level(level);
	dprintf("(%ld) @%p \"%s\"\n", level, node, node->ModuleName());
	
	AttributeList::Iterator attribute = node->Attributes().GetIterator();
	while (attribute.HasNext()) {
		dump_attribute(attribute.Next(), level);
	}

	NodeList::ConstIterator iterator = node->Children().GetIterator();
	while (iterator.HasNext()) {
		dm_dump_node(iterator.Next(), level + 1);
	}
}


static void
uninit_unused()
{
	RecursiveLocker _(sLock);
	sRootNode->UninitUnusedChildren();
}


static status_t
probe_path(const char* path)
{
	RecursiveLocker _(sLock);
	return sRootNode->Probe(path);
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
}


device_node::~device_node()
{
	// Delete children
	NodeList::Iterator nodeIterator = fChildren.GetIterator();
	while (nodeIterator.HasNext()) {
		device_node* child = nodeIterator.Next();
		nodeIterator.Remove();
		delete child;
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
device_node::InitCheck()
{
	return fModuleName != NULL ? B_OK : B_NO_MEMORY;
}


status_t
device_node::InitDriver()
{
	if (fInitialized++ > 0)
		return B_OK;

	status_t status = get_module(ModuleName(), (module_info**)&fDriver);
	if (status < B_OK) {
		fInitialized--;
		return status;
	}

	if (fDriver->init_driver != NULL)
		status = fDriver->init_driver(this, &fDriverData);
	if (status < B_OK) {
		fInitialized--;
		put_module(ModuleName());
		return status;
	}

	return B_OK;
}


bool
device_node::UninitDriver()
{
	if (fInitialized-- > 1)
		return false;

	if (fDriver->uninit_driver != NULL)
		fDriver->uninit_driver(this);

	fDriver = NULL;
	fDriverData = NULL;

	put_module(ModuleName());

	if ((fFlags & NODE_FLAG_REMOVE_ON_UNINIT) != 0)
		delete this;

	return true;
}


void
device_node::AddChild(device_node* node)
{
	// we must not be destroyed	as long as we have children
	fRefCount++;
	node->fParent = this;
	fChildren.Add(node);
}


void
device_node::RemoveChild(device_node* node)
{
	fRefCount--;
		// TODO: we may need to destruct ourselves here!
	node->fParent = NULL;
	fChildren.Remove(node);
}


status_t
device_node::Register()
{
	uint32 registered;
	status_t status = _RegisterFixed(registered);
	if (status != B_OK)
		return status;
	if (registered > 0) {
		fRegistered = true;
		return B_OK;
	}

	// Register the children the driver wants

	if (DriverModule()->register_child_devices != NULL) {
		status = DriverModule()->register_child_devices(this);
		if (status != B_OK)
			return status;

		if (!fChildren.IsEmpty()) {
			fRegistered = true;
			return B_OK;
		}
	}

	// Register all possible child device nodes

	status = _RegisterDynamic();
	if (status == B_OK)
		fRegistered = true;

	return status;
}


/*!	Registers any children that are identified via the B_DRIVER_FIXED_CHILD
	attribute.
	If any of these children cannot be registered, this call will fail (we
	don't remove already registered children in this case).
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
		dm_get_attr_uint16(this, B_DEVICE_TYPE, &type, false);
		dm_get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false);
		dm_get_attr_uint16(this, B_DEVICE_INTERFACE, &interface, false);

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
	float& bestSupport)
{
	if (bestDriver == NULL)
		bestSupport = 0.0f;

	void* list = open_module_list_etc(path, "driver_v1");
	driver_module_info* driver;
	while (_GetNextDriver(list, driver) == B_OK) {
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
printf("  register module \"%s\", support %f\n", driver->info.name, support);
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
	dm_get_attr_uint16(this, B_DEVICE_TYPE, &type, false);
	dm_get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false);

	return type == PCI_serial_bus || type == PCI_bridge;
		// TODO: we may want to be a bit more specific in the future
}


status_t
device_node::_RegisterDynamic()
{
	uint32 findFlags = 0;
	dm_get_attr_uint32(this, B_DEVICE_FIND_CHILD_FLAGS, &findFlags, false);

	// If this is our initial registration, we honour the B_FIND_CHILD_ON_DEMAND
	// requirements
	if (!fRegistered && (findFlags & B_FIND_CHILD_ON_DEMAND) != 0
		&& !_AlwaysRegisterDynamic())
		return B_OK;

	KPath path;

	if ((findFlags & B_FIND_MULTIPLE_CHILDREN) == 0) {
		// find the one driver
		driver_module_info* bestDriver = NULL;
		float bestSupport = 0.0;
		void* cookie = NULL;

		while (_GetNextDriverPath(cookie, path) == B_OK) {
			_FindBestDriver(path.Path(), bestDriver, bestSupport);
		}

		if (bestDriver != NULL) {
printf("  register best module \"%s\", support %f\n", bestDriver->info.name, bestSupport);
			bestDriver->register_device(this);
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


status_t
device_node::_RemoveChildren()
{
	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		if (!child->IsInitialized()) {
			// this child is not used currently, and can be removed safely
			iterator.Remove();
			fRefCount--;
			child->fParent = NULL;
			delete child;
		} else
			child->fFlags |= NODE_FLAG_REMOVE_ON_UNINIT;
	}

	return fChildren.IsEmpty() ? B_OK : B_BUSY;
}


status_t
device_node::Probe(const char* devicePath)
{
	uint16 type = 0;
	uint16 subType = 0;
	if (dm_get_attr_uint16(this, B_DEVICE_TYPE, &type, false) == B_OK
		&& dm_get_attr_uint16(this, B_DEVICE_SUB_TYPE, &subType, false)
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
			if (!fChildren.IsEmpty()) {
				// We already have a driver that claims this node.
				// Try to remove uninitialized children, so that this node
				// can be re-evaluated
				// TODO: try first if there is a better child!
				// TODO: publish both devices, make new one busy as long
				// as the old one is in use!
				if (_RemoveChildren() != B_OK)
					return B_OK;
			}
			return _RegisterDynamic();
		}

		return B_OK;
	}

	NodeList::Iterator iterator = fChildren.GetIterator();
	while (iterator.HasNext()) {
		device_node* child = iterator.Next();
		
		status_t status = child->Probe(devicePath);
		if (status != B_OK)
			return status;
	}

	return B_OK;
}


bool
device_node::_UninitUnusedChildren()
{
	// First, we need to go to the leaf, and go back from there

	bool uninit = true;

	NodeList::Iterator iterator = fChildren.GetIterator();

	while (iterator.HasNext()) {
		device_node* child = iterator.Next();

		if (!child->_UninitUnusedChildren())
			uninit = false;
	}

	// Not all of our children could be uninitialized
	if (!uninit)
		return false;

	if (!IsInitialized())
		return true;

	if ((DriverModule()->info.flags & B_KEEP_LOADED) != 0) {
		// We must not get unloaded
		return false;
	}

	return UninitDriver();
}


void
device_node::UninitUnusedChildren()
{
	_UninitUnusedChildren();
}


//	#pragma mark - Device Manager module API


static status_t
rescan_device(device_node* node)
{
	return B_ERROR;
}


static status_t
register_device(device_node* parent, const char* moduleName,
	const device_attr* attrs, const io_resource* ioResources,
	device_node** _node)
{
	if ((parent == NULL && sRootNode != NULL) || moduleName == NULL)
		return B_BAD_VALUE;

	// TODO: handle I/O resources!

	device_node *newNode = new(std::nothrow) device_node(moduleName, attrs,
		ioResources);
	if (newNode == NULL)
		return B_NO_MEMORY;

	TRACE(("%p: register device \"%s\", parent %p\n", newNode, moduleName,
		parent));

	RecursiveLocker _(sLock);

	status_t status = newNode->InitCheck();
	if (status != B_OK)
		goto err1;

	status = newNode->InitDriver();
	if (status != B_OK)
		goto err1;

	// make it public
	if (parent != NULL)
		parent->AddChild(newNode);
	else
		sRootNode = newNode;

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

	status = newNode->Register();
	if (status < B_OK) {
		parent->RemoveChild(newNode);
		goto err1;
	}

	if (_node)
		*_node = newNode;

	return B_OK;

err1:
	delete newNode;
	return status;
}


static status_t
unregister_device(device_node* node)
{
	return B_ERROR;
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
device_root(void)
{
	return sRootNode;
}


static status_t
get_next_child_device(device_node* parent, device_node* _node,
	const device_attr* attrs)
{
	return B_ERROR;
}


static device_node*
get_parent(device_node* node)
{
	return NULL;
}


static void
put_device_node(device_node* node)
{
}


status_t
dm_get_attr_uint8(const device_node* node, const char* name, uint8* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT8_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui8;
	return B_OK;
}


status_t
dm_get_attr_uint16(const device_node* node, const char* name, uint16* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT16_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui16;
	return B_OK;
}


status_t
dm_get_attr_uint32(const device_node* node, const char* name, uint32* _value,
	bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT32_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui32;
	return B_OK;
}


status_t
dm_get_attr_uint64(const device_node* node, const char* name,
	uint64* _value, bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT64_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui64;
	return B_OK;
}


status_t
dm_get_attr_string(const device_node* node, const char* name,
	const char** _value, bool recursive)
{
	if (node == NULL || name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_STRING_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.string;
	return B_OK;
}


status_t
dm_get_attr_raw(const device_node* node, const char* name, const void** _data,
	size_t* _length, bool recursive)
{
	if (node == NULL || name == NULL || (_data == NULL && _length == NULL))
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive, B_RAW_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	if (_data != NULL)
		*_data = attr->value.raw.data;
	if (_length != NULL)
		*_length = attr->value.raw.length;
	return B_OK;
}


status_t
dm_get_next_attr(device_node* node, device_attr** _attr)
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
	rescan_device,
	register_device,
	unregister_device,
	get_driver,
	device_root,
	get_next_child_device,
	get_parent,
	put_device_node,

	// attributes
	dm_get_attr_uint8,
	dm_get_attr_uint16,
	dm_get_attr_uint32,
	dm_get_attr_uint64,
	dm_get_attr_string,
	dm_get_attr_raw,
	dm_get_next_attr,
};


//	#pragma mark - root node


void
dm_init_root_node(void)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "Devices Root"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {string: "root"}},
		{B_DEVICE_FIND_CHILD_FLAGS, B_UINT32_TYPE,
			{ui32: B_FIND_MULTIPLE_CHILDREN}},
		{NULL}
	};

	if (register_device(NULL, DEVICE_MANAGER_ROOT_NAME, attrs, NULL, NULL)
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
	_add_builtin_module((module_info*)&gDeviceModuleInfo);
	_add_builtin_module((module_info*)&gDriverModuleInfo);
	_add_builtin_module((module_info*)&gBusModuleInfo);
	_add_builtin_module((module_info*)&gBusDriverModuleInfo);

	gDeviceManager = &sDeviceManagerModule;

	status_t status = _get_builtin_dependencies();
	if (status < B_OK) {
		fprintf(stderr, "device_manager: Could not initialize modules: %s\n",
			strerror(status));
		return 1;
	}

	recursive_lock_init(&sLock, "device manager");

	dm_init_root_node();
	dm_dump_node(sRootNode, 0);

	probe_path("net");
	uninit_unused();

	recursive_lock_destroy(&sLock);
	return 0;
}
