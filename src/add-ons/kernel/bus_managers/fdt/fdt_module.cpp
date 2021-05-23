/*
 * Copyright 2014, Ithamar R. Adema <ithamar@upgrade-android.com>
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Copyright 2015-2021, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <drivers/bus/FDT.h>
#include <KernelExport.h>
#include <util/kernel_cpp.h>
#include <device_manager.h>

#include <AutoDeleter.h>
#include <AutoDeleterDrivers.h>
#include <HashMap.h>
#include <debug.h>

extern "C" {
#include <fdt.h>
#include <libfdt.h>
#include <libfdt_env.h>
};


//#define TRACE_FDT
#ifdef TRACE_FDT
#define TRACE(x...) dprintf(x)
#else
#define TRACE(x...)
#endif


extern void* gFDT;

device_manager_info* gDeviceManager;

extern fdt_bus_module_info gBusModule;
extern fdt_device_module_info gDeviceModule;


//#pragma mark -


struct fdt_bus {
	device_node* node;
	HashMap<HashKey32<int32>, device_node*> phandles;
};


struct fdt_device {
	device_node* node;
	device_node* bus;
};


static status_t
fdt_register_node(fdt_bus* bus, int node, device_node* parentDev,
	device_node*& curDev)
{
	TRACE("%s('%s', %p)\n", __func__, fdt_get_name(gFDT, node, NULL),
		parentDev);

	const void* prop; int propLen;
	device_attr attrs[8];
	device_attr* attr = attrs;

	const char *name = fdt_get_name(gFDT, node, NULL);

	*attr++ = (device_attr) { B_DEVICE_BUS, B_STRING_TYPE, {string: "fdt"}};
	*attr++ = (device_attr) { B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
		{ string: (strcmp(name, "") != 0) ? name : "Root" } };
	*attr++ = (device_attr) { "fdt/node", B_UINT32_TYPE, {ui32: (uint32)node}};
	*attr++ = (device_attr) { "fdt/name", B_STRING_TYPE, {string: name}};

	prop = fdt_getprop(gFDT, node, "device_type", &propLen);
	if (prop != NULL) {
		*attr++ = (device_attr) { "fdt/device_type", B_STRING_TYPE,
			{ string: (const char*)prop } };

	prop = fdt_getprop(gFDT, node, "compatible", &propLen);

	if (prop != NULL) {
		*attr++ = (device_attr){ "fdt/compatible", B_STRING_TYPE,
			{ string: (const char*)prop } };
	}

	*attr = {0};

	status_t res = gDeviceManager->register_node(parentDev,
		"bus_managers/fdt/driver_v1", attrs, NULL, &curDev);

	if (res < B_OK)
		return res;

	prop = fdt_getprop(gFDT, node, "phandle", &propLen);

	if (prop != NULL)
		bus->phandles.Put(fdt32_to_cpu(*(uint32_t*)prop), curDev);

	return B_OK;
}


static void
fdt_traverse(fdt_bus* bus, int &node, int &depth, device_node* parentDev)
{
	int curDepth = depth;
#if 0
	for (int i = 0; i < depth; i++) dprintf("  ");
	dprintf("node('%s')\n", fdt_get_name(gFDT, node, NULL));
#endif
	device_node* curDev;
	fdt_register_node(bus, node, parentDev, curDev);

	node = fdt_next_node(gFDT, node, &depth);
	while (node >= 0 && depth == curDepth + 1) {
		fdt_traverse(bus, node, depth, curDev);
	}
}


//#pragma mark bus

static int32
fdt_bus_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			TRACE("fdt root init\n");
			return B_OK;

		case B_MODULE_UNINIT:
			TRACE("fdt root uninit\n");
			return B_OK;
	}

	return B_BAD_VALUE;
}


static float
fdt_bus_supports_device(device_node* parent)
{
	TRACE("fdt_bus_supports_device\n");

	// make sure parent is really device root
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false))
		return B_ERROR;

	if (strcmp(bus, "root"))
		return 0.0;

	return 1.0;
}


static status_t
fdt_bus_register_device(device_node* parent)
{
	TRACE("+fdt_bus_register_device\n");
	struct ScopeExit {
		ScopeExit() {TRACE("-fdt_bus_register_device\n");}
	} scopeExit;

	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {string: "FDT"}},
		{B_DEVICE_FLAGS, B_UINT32_TYPE, {ui32: B_KEEP_DRIVER_LOADED}},
		{}
	};

	return gDeviceManager->register_node(
		parent, "bus_managers/fdt/root/driver_v1", attrs, NULL, NULL);
}


static status_t
fdt_bus_init(device_node* node, void** cookie)
{
	TRACE("fdt_bus_init\n");

	ObjectDeleter<fdt_bus> bus(new(std::nothrow) fdt_bus());
	if (!bus.IsSet())
		return B_NO_MEMORY;

	bus->node = node;
	*cookie = bus.Detach();
	return B_OK;
}


static void
fdt_bus_uninit(void* cookie)
{
	TRACE("fdt_bus_uninit\n");

	ObjectDeleter<fdt_bus> bus((fdt_bus*)cookie);
}


static status_t
fdt_bus_register_child_devices(void* cookie)
{
	TRACE("fdt_bus_register_child_devices\n");

	fdt_bus* bus = (fdt_bus*)cookie;

	int node = -1, depth = -1;
	node = fdt_next_node(gFDT, node, &depth);
	fdt_traverse(bus, node, depth, bus->node);

	return B_OK;
}


device_node*
fdt_bus_node_by_phandle(fdt_bus* bus, int phandle)
{
	ASSERT(bus != NULL);

	device_node** devNode;
	if (!bus->phandles.Get(phandle, devNode))
		return NULL;

	return *devNode;
}


//#pragma mark device


static status_t
fdt_device_std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;
	}

	return B_BAD_VALUE;
}


static status_t
fdt_device_init_driver(device_node* node, void** cookie)
{
	TRACE("fdt_device_init_driver()\n");

	ObjectDeleter<fdt_device> dev(new(std::nothrow) fdt_device());
	if (!dev.IsSet())
		return B_NO_MEMORY;

	dev->node = node;

	// get bus from parent node
	DeviceNodePutter<&gDeviceManager> parent(
		gDeviceManager->get_parent_node(node));
	driver_module_info* parentModule;
	void* parentDev;
	ASSERT(gDeviceManager->get_driver(
		parent.Get(), &parentModule, &parentDev) >= B_OK);
	if (parentModule == (driver_module_info*)&gDeviceModule)
		dev->bus = ((fdt_device*)parentDev)->bus;
	else if (parentModule == (driver_module_info*)&gBusModule)
		dev->bus = parent.Get();
	else
		panic("bad parent node");

	*cookie = dev.Detach();
	return B_OK;
}


static void
fdt_device_uninit_driver(void* cookie)
{
	TRACE("fdt_device_uninit_driver()\n");
	ObjectDeleter<fdt_device> dev((fdt_device*)cookie);
}


static status_t
fdt_device_register_child_devices(void* cookie)
{
	TRACE("fdt_device_register_child_devices()\n");
	return B_OK;
}


static device_node*
fdt_device_get_bus(fdt_device* dev)
{
	ASSERT(dev != NULL);
	return dev->bus;
}


static const char*
fdt_device_get_name(fdt_device* dev)
{
	ASSERT(dev != NULL);

	uint32 fdtNode;
	ASSERT(gDeviceManager->get_attr_uint32(
		dev->node, "fdt/node", &fdtNode, false) >= B_OK);

	return fdt_get_name(gFDT, (int)fdtNode, NULL);
}


static const void*
fdt_device_get_prop(fdt_device* dev, const char* name, int* len)
{
	ASSERT(dev != NULL);

	uint32 fdtNode;
	ASSERT(gDeviceManager->get_attr_uint32(
		dev->node, "fdt/node", &fdtNode, false) >= B_OK);

	return fdt_getprop(gFDT, (int)fdtNode, name, len);
}


static bool
fdt_device_get_reg(fdt_device* dev, uint32 ord, uint64* regs, uint64* len)
{
	ASSERT(dev != NULL);

	uint32 fdtNode;
	ASSERT(gDeviceManager->get_attr_uint32(
		dev->node, "fdt/node", &fdtNode, false) >= B_OK);

	int propLen;
	const void* prop = fdt_getprop(gFDT, (int)fdtNode, "reg", &propLen);
	if (prop == NULL)
		return false;

	// TODO: use '#address-cells', '#size-cells' in parent node to identify
	// field sizes

	if ((ord + 1)*16 > (uint32)propLen)
		return false;

	if (regs != NULL)
		*regs = fdt64_to_cpu(*(((uint64*)prop) + 2*ord));

	if (len != NULL)
		*len = fdt64_to_cpu(*(((uint64*)prop) + 2*ord + 1));

	return true;
}


static bool
fdt_device_get_interrupt(fdt_device* dev, uint32 ord,
	device_node** interruptController, uint64* interrupt)
{
	ASSERT(dev != NULL);

	uint32 fdtNode;
	ASSERT(gDeviceManager->get_attr_uint32(
		dev->node, "fdt/node", &fdtNode, false) >= B_OK);

	// TODO: handle other interrupt encodings
	int propLen;
	const void* prop = fdt_getprop(gFDT, (int)fdtNode, "interrupts-extended",
		&propLen);
	if (prop == NULL) {
		prop = fdt_getprop(gFDT, (int)fdtNode, "interrupts",
			&propLen);
		if (prop == NULL)
			return false;

		if ((ord + 1)*4 > (uint32)propLen)
			return false;

		if (interrupt != NULL)
			*interrupt = fdt32_to_cpu(*(((uint32*)prop) + ord));

		if (interruptController != NULL) {
			prop = fdt_getprop(gFDT, (int)fdtNode, "interrupt-parent",
				&propLen);
			if (prop != NULL && propLen == 4) {
				uint32 phandle = fdt32_to_cpu(*(uint32*)prop);

				fdt_bus* bus;
				ASSERT(gDeviceManager->get_driver(
					dev->bus, NULL, (void**)&bus) >= B_OK);

				*interruptController = fdt_bus_node_by_phandle(bus, phandle);
			}
		}
		return true;
	}

	// TODO: use '#interrupt-cells' to identify field sizes

	if ((ord + 1)*8 > (uint32)propLen)
		return false;

	if (interruptController != NULL) {
		uint32 phandle = fdt32_to_cpu(*(((uint32*)prop) + 2*ord));

		fdt_bus* bus;
		ASSERT(gDeviceManager->get_driver(
			dev->bus, NULL, (void**)&bus) >= B_OK);

		*interruptController = fdt_bus_node_by_phandle(bus, phandle);
	}

	if (interrupt != NULL)
		*interrupt = fdt32_to_cpu(*(((uint32*)prop) + 2*ord + 1));

	return true;
}


//#pragma mark -

fdt_bus_module_info gBusModule = {
	{
		{
			"bus_managers/fdt/root/driver_v1",
			0,
			fdt_bus_std_ops
		},
		fdt_bus_supports_device,
		fdt_bus_register_device,
		fdt_bus_init,
		fdt_bus_uninit,
		fdt_bus_register_child_devices,
		NULL,	// rescan devices
		NULL,	// device removed
	},
	fdt_bus_node_by_phandle,
};


fdt_device_module_info gDeviceModule = {
	{
		{
			"bus_managers/fdt/driver_v1",
			0,
			fdt_device_std_ops
		},

		NULL,		// supports device
		NULL,		// register device (our parent registered us)
		fdt_device_init_driver,
		fdt_device_uninit_driver,
		fdt_device_register_child_devices,
		NULL,		// rescan devices
		NULL,		// device removed
	},
	fdt_device_get_bus,
	fdt_device_get_name,
	fdt_device_get_prop,
	fdt_device_get_reg,
	fdt_device_get_interrupt,
};


module_info* modules[] = {
	(module_info*)&gBusModule,
	(module_info*)&gDeviceModule,
	NULL
};

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ NULL }
};
