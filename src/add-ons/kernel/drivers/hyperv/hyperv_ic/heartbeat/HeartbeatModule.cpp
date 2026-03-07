/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "Heartbeat.h"


static float
hyperv_heartbeat_supports_device(device_node* parent)
{
	CALLED();

	// Check if parent is the Hyper-V bus manager
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return -1;

	if (strcmp(bus, HYPERV_BUS_NAME) != 0)
		return 0.0f;

	// Check if parent is a Hyper-V heartbeat device
	const char* type;
	if (gDeviceManager->get_attr_string(parent, HYPERV_DEVICE_TYPE_STRING_ITEM, &type, false)
			!= B_OK)
		return 0.0f;

	if (strcmp(type, VMBUS_TYPE_HEARTBEAT) != 0)
		return 0.0f;

	TRACE("Hyper-V Heartbeat device found!\n");
	return 0.8f;
}


static status_t
hyperv_heartbeat_register_device(device_node* parent)
{
	CALLED();

	device_attr attributes[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = HYPERV_PRETTYNAME_HEARTBEAT }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, HYPERV_HEARTBEAT_DRIVER_MODULE_NAME, attributes,
		NULL, NULL);
}


static status_t
hyperv_heartbeat_init_driver(device_node* node, void** _driverCookie)
{
	CALLED();

	Heartbeat* heartbeat = new(std::nothrow) Heartbeat(node);
	if (heartbeat == NULL) {
		ERROR("Unable to allocate Hyper-V Heartbeat object\n");
		return B_NO_MEMORY;
	}

	status_t status = heartbeat->InitCheck();
	if (status != B_OK) {
		ERROR("Failed to set up Hyper-V Heartbeat object\n");
		delete heartbeat;
		return status;
	}
	TRACE("Hyper-V Heartbeat object created\n");

	*_driverCookie = heartbeat;
	return B_OK;
}


static void
hyperv_heartbeat_uninit_driver(void* driverCookie)
{
	CALLED();
	Heartbeat* heartbeat = reinterpret_cast<Heartbeat*>(driverCookie);
	delete heartbeat;
}


driver_module_info gHyperVHeartbeatDriverModule = {
	{
		HYPERV_HEARTBEAT_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	hyperv_heartbeat_supports_device,
	hyperv_heartbeat_register_device,
	hyperv_heartbeat_init_driver,
	hyperv_heartbeat_uninit_driver,
	NULL,	// register child devices
	NULL,	// rescan
	NULL	// removed
};
