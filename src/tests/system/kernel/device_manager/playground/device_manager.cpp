/*
 * Copyright 2008, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "device_manager.h"

#include <util/AutoLock.h>
#include <util/DoublyLinkedList.h>

#include <KernelExport.h>
#include <module.h>
#include <Locker.h>

#include <new>
#include <set>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


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

status_t dm_get_attr_uint32(device_node* node, const char* name, uint32* _value,
	bool recursive);

device_manager_info *gDeviceManager;

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

			status_t		InitDriver();
			void			UninitDriver();

			// The following two are only valid, if the node's driver is
			// initialized
			driver_module_info* DriverModule() const { return fDriver; }
			void*			DriverData() const { return fDriverData; }

			void			AddChild(device_node *node);

			status_t		Register();
			bool			IsRegistered() const { return fRegistered; }

private:
			status_t		_RegisterFixed(uint32& registered);
			status_t		_RegisterDynamic();

	device_node*			fParent;
	NodeList				fChildren;
	int32					fRefCount;
	int32					fInitialized;
	bool					fRegistered;

	const char*				fModuleName;
	driver_module_info*		fDriver;
	void*					fDriverData;

	AttributeList			fAttributes;
};


static device_node *sRootNode;


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
dm_find_attr(device_node* node, const char* name, bool recursive,
	type_code type)
{
	do {
		AttributeList::Iterator iterator = node->Attributes().GetIterator();

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


//	#pragma mark -


/*!	Allocate device node info structure;
	initially, ref_count is one to make sure node won't get destroyed by mistake
*/
device_node::device_node(const char *moduleName, const device_attr *attrs,
	const io_resource *resources)
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
	AttributeList::Iterator iterator = fAttributes.GetIterator();
	while (iterator.HasNext()) {
		device_attr_private* attr = iterator.Next();
		iterator.Remove();
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


void
device_node::UninitDriver()
{
	if (fInitialized-- > 1)
		return;

	if (fDriver->uninit_driver != NULL)
		fDriver->uninit_driver(this);
	fDriverData = NULL;

	put_module(ModuleName());
}


void
device_node::AddChild(device_node *node)
{
	// we must not be destroyed	as long as we have children
	fRefCount++;
	node->fParent = this;
	fChildren.Add(node);
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

	if (DriverModule()->register_child_devices != NULL)
		DriverModule()->register_child_devices(this);

	// Register all possible child device nodes

	uint32 findFlags;
	if (dm_get_attr_uint32(this, B_DRIVER_FIND_CHILD_FLAGS, &findFlags, false) != B_OK)
		return B_OK;

#if 0
	if ((findFlags & B_FIND_CHILD_ON_DEMAND) != 0)
		return B_OK;
#endif

	return _RegisterDynamic();
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
		if (strcmp(attr->name, B_DRIVER_FIXED_CHILD))
			continue;

		driver_module_info* driver;
		status_t status = get_module(attr->value.string,
			(module_info**)&driver);
		if (status != B_OK)
			return status;

		if (driver->supports_device != NULL
			&& driver->register_device != NULL) {
			float support = driver->supports_device(this);
			if (support <= 0.0)
				status = B_ERROR;

			if (status == B_OK)
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
device_node::_RegisterDynamic()
{
	uint32 findFlags;
	if (dm_get_attr_uint32(this, B_DRIVER_FIND_CHILD_FLAGS, &findFlags, false)
			!= B_OK)
		findFlags = 0;

	driver_module_info* bestDriver = NULL;
	float best = 0.0;

	void* list = open_module_list_etc("bus", "driver_v1");
	while (true) {
		char name[B_FILE_NAME_LENGTH];
		size_t nameLength = sizeof(name);

		if (read_next_module_name(list, name, &nameLength) != B_OK)
			break;

		if (!strcmp(fModuleName, name))
			continue;

		driver_module_info* driver;
		if (get_module(name, (module_info**)&driver) != B_OK)
			continue;

		if (driver->supports_device != NULL
			&& driver->register_device != NULL) {
			float support = driver->supports_device(this);

			if ((findFlags & B_FIND_MULTIPLE_CHILDREN) == 0) {
				if (support > best) {
					if (bestDriver != NULL)
						put_module(bestDriver->info.name);

					bestDriver = driver;
					best = support;
					continue;
						// keep reference to best module around
				}
			} else if (support > 0.0) {
printf("  register module \"%s\", support %f\n", name, support);
				driver->register_device(this);
			}
		}

		put_module(name);
	}
	close_module_list(list);

	if (bestDriver != NULL) {
printf("  register best module \"%s\", support %f\n", bestDriver->info.name, best);
		bestDriver->register_device(this);
		put_module(bestDriver->info.name);
	}
	
	return B_OK;
}


//	#pragma mark -


//	#pragma mark - Device Manager module API


static status_t
rescan_device(device_node *node)
{
	return B_ERROR;
}


static status_t
register_device(device_node *parent, const char *moduleName,
	const device_attr *attrs, const io_resource *ioResources,
	device_node **_node)
{
	if ((parent == NULL && sRootNode != NULL) || moduleName == NULL)
		return B_BAD_VALUE;

	TRACE(("register device \"%s\", parent %p\n", moduleName, parent));
	// TODO: handle I/O resources!

	device_node *newNode = new(std::nothrow) device_node(moduleName, attrs,
		ioResources);
	if (newNode == NULL)
		return B_NO_MEMORY;

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
	if (status < B_OK)
		return status;

	if (_node)
		*_node = newNode;

	return B_OK;

err1:
	delete newNode;
	return status;
}


static status_t
unregister_device(device_node *node)
{
	return B_ERROR;
}


static driver_module_info*
driver_module(device_node *node)
{
	return node->DriverModule();
}


static void*
driver_data(device_node *node)
{
	return node->DriverData();
}


static device_node *
device_root(void)
{
	return sRootNode;
}


static status_t
get_next_child_device(device_node *parent, device_node *_node,
	const device_attr *attrs)
{
	return B_ERROR;
}


static device_node *
get_parent(device_node *node)
{
	return NULL;
}


static void
put_device_node(device_node *node)
{
}


status_t
dm_get_attr_uint8(device_node* node, const char* name, uint8* _value,
	bool recursive)
{
	if (name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT8_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui8;
	return B_OK;
}


status_t
dm_get_attr_uint16(device_node* node, const char* name, uint16* _value,
	bool recursive)
{
	if (name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT16_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui16;
	return B_OK;
}


status_t
dm_get_attr_uint32(device_node* node, const char* name, uint32* _value,
	bool recursive)
{
	if (name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT32_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui32;
	return B_OK;
}


status_t
dm_get_attr_uint64(device_node* node, const char* name,
	uint64* _value, bool recursive)
{
	if (name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_UINT64_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.ui64;
	return B_OK;
}


status_t
dm_get_attr_string(device_node* node, const char* name, const char** _value,
	bool recursive)
{
	if (name == NULL || _value == NULL)
		return B_BAD_VALUE;

	device_attr_private* attr = dm_find_attr(node, name, recursive,
		B_STRING_TYPE);
	if (attr == NULL)
		return B_NAME_NOT_FOUND;

	*_value = attr->value.string;
	return B_OK;
}


status_t
dm_get_attr_raw(device_node* node, const char* name, const void** _data,
	size_t* _length, bool recursive)
{
	if (name == NULL || (_data == NULL && _length == NULL))
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
	driver_module,
	driver_data,
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
		{B_DRIVER_PRETTY_NAME, B_STRING_TYPE, {string: "Devices Root"}},
		{B_DRIVER_BUS, B_STRING_TYPE, {string: "root"}},
		{B_DRIVER_FIND_CHILD_FLAGS, B_UINT32_TYPE,
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
	_add_builtin_module((module_info *)&sDeviceManagerModule);
	_add_builtin_module((module_info *)&sDeviceRootModule);
	_add_builtin_module((module_info *)&gDeviceModuleInfo);
	_add_builtin_module((module_info *)&gDriverModuleInfo);
	_add_builtin_module((module_info *)&gBusModuleInfo);
	_add_builtin_module((module_info *)&gBusDriverModuleInfo);

	gDeviceManager = &sDeviceManagerModule;

	status_t status = _get_builtin_dependencies();
	if (status < B_OK) {
		fprintf(stderr, "device_manager: Could not initialize modules: %s\n",
			strerror(status));
		return 1;
	}

	dm_init_root_node();

	return 0;
}
