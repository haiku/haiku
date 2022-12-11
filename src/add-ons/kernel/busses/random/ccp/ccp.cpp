/*
 * Copyright 2022, Jérôme Duval, jerome.duval@gmail.com.
 * Distributed under the terms of the MIT License.
 */


#include <new>
#include <stdio.h>
#include <string.h>

#include <condition_variable.h>
#include <dpc.h>

#include "random.h"

#include "ccp.h"


#define CCP_REG_TRNG	0xc


device_manager_info* gDeviceManager;
random_for_controller_interface *gRandom;
dpc_module_info *gDPC;


static void
handleDPC(void *arg)
{
	CALLED();
	ccp_device_info* bus = reinterpret_cast<ccp_device_info*>(arg);

	uint32 lowValue = read32(bus->registers + CCP_REG_TRNG);
	uint32 highValue = read32(bus->registers + CCP_REG_TRNG);
	if (lowValue == 0 || highValue == 0)
		return;
	gRandom->queue_randomness((uint64)lowValue | ((uint64)highValue << 32));
}


static int32
handleTimerHook(struct timer* timer)
{
	ccp_device_info* bus = reinterpret_cast<ccp_device_info*>(timer->user_data);

	gDPC->queue_dpc(bus->dpcHandle, handleDPC, bus);
	return B_HANDLED_INTERRUPT;
}


//	#pragma mark -


static status_t
register_device(device_node* parent)
{
	device_attr attrs[] = {
		{B_DEVICE_PRETTY_NAME, B_STRING_TYPE, {.string = "CCP device"}},
		{B_DEVICE_BUS, B_STRING_TYPE, {.string = "CCP"}},
		{}
	};

	return gDeviceManager->register_node(parent,
		CCP_DEVICE_MODULE_NAME, attrs, NULL, NULL);
}


static status_t
init_bus(device_node* node, void** bus_cookie)
{
	CALLED();

	driver_module_info* driver;
	ccp_device_info* bus;
	device_node* parent = gDeviceManager->get_parent_node(node);
	gDeviceManager->get_driver(parent, &driver, (void**)&bus);
	gDeviceManager->put_node(parent);

	TRACE_ALWAYS("init_bus() addr 0x%" B_PRIxPHYSADDR " size 0x%" B_PRIx64
		" \n", bus->base_addr, bus->map_size);

	bus->registersArea = map_physical_memory("CCP memory mapped registers",
		bus->base_addr, bus->map_size, B_ANY_KERNEL_ADDRESS,
		B_KERNEL_READ_AREA | B_KERNEL_WRITE_AREA,
		(void **)&bus->registers);
	if (bus->registersArea < 0)
		return bus->registersArea;

	status_t status = gDPC->new_dpc_queue(&bus->dpcHandle, "ccp timer",
		B_LOW_PRIORITY);
	if (status != B_OK) {
		ERROR("dpc setup failed (%s)\n", strerror(status));
		return status;
	}

	bus->extractTimer.user_data = bus;
	status = add_timer(&bus->extractTimer, &handleTimerHook, 1 * 1000 * 1000, B_PERIODIC_TIMER);
	if (status != B_OK) {
		ERROR("timer setup failed (%s)\n", strerror(status));
		return status;
	}

	// trigger also now
	gDPC->queue_dpc(bus->dpcHandle, handleDPC, bus);

	*bus_cookie = bus;
	return B_OK;

}


static void
uninit_bus(void* bus_cookie)
{
	ccp_device_info* bus = (ccp_device_info*)bus_cookie;

	cancel_timer(&bus->extractTimer);
	gDPC->delete_dpc_queue(&bus->dpcHandle);

	if (bus->registersArea >= 0)
		delete_area(bus->registersArea);

}


//	#pragma mark -


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{ RANDOM_FOR_CONTROLLER_MODULE_NAME, (module_info**)&gRandom },
	{ B_DPC_MODULE_NAME, (module_info **)&gDPC },
	{}
};


static driver_module_info sCcpDeviceModule = {
	{
		CCP_DEVICE_MODULE_NAME,
		0,
		NULL
	},

	NULL,	// supports device
	register_device,
	init_bus,
	uninit_bus,
	NULL,	// register child devices
	NULL,	// rescan
	NULL, 	// device removed
};


module_info* modules[] = {
	(module_info* )&gCcpAcpiDevice,
	(module_info* )&sCcpDeviceModule,
	(module_info* )&gCcpPciDevice,
	NULL
};
