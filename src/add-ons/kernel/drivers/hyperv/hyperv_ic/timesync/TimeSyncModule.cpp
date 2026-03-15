/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "TimeSync.h"


static float
hyperv_timesync_supports_device(device_node* parent)
{
	CALLED();

	// Check if parent is the Hyper-V bus manager
	const char* bus;
	if (gDeviceManager->get_attr_string(parent, B_DEVICE_BUS, &bus, false) != B_OK)
		return -1;

	if (strcmp(bus, HYPERV_BUS_NAME) != 0)
		return 0.0f;

	// Check if parent is a Hyper-V time sync device
	const char* type;
	if (gDeviceManager->get_attr_string(parent, HYPERV_DEVICE_TYPE_STRING_ITEM, &type, false)
			!= B_OK)
		return 0.0f;

	if (strcmp(type, VMBUS_TYPE_TIMESYNC) != 0)
		return 0.0f;

	TRACE("Hyper-V Time Sync device found!\n");
	return 0.8f;
}


static status_t
hyperv_timesync_register_device(device_node* parent)
{
	CALLED();

	device_attr attributes[] = {
		{ B_DEVICE_PRETTY_NAME, B_STRING_TYPE,
			{ .string = HYPERV_PRETTYNAME_TIMESYNC }},
		{ NULL }
	};

	return gDeviceManager->register_node(parent, HYPERV_TIMESYNC_DRIVER_MODULE_NAME, attributes,
		NULL, NULL);
}


static status_t
hyperv_timesync_init_driver(device_node* node, void** _driverCookie)
{
	CALLED();

	TimeSync* timeSync = new(std::nothrow) TimeSync(node);
	if (timeSync == NULL) {
		ERROR("Unable to allocate Hyper-V Time Sync object\n");
		return B_NO_MEMORY;
	}

	status_t status = timeSync->InitCheck();
	if (status != B_OK) {
		ERROR("Failed to set up Hyper-V Time Sync object\n");
		delete timeSync;
		return status;
	}
	TRACE("Hyper-V Time Sync object created\n");

	*_driverCookie = timeSync;
	return B_OK;
}


static void
hyperv_timesync_uninit_driver(void* driverCookie)
{
	CALLED();
	TimeSync* timeSync = reinterpret_cast<TimeSync*>(driverCookie);
	delete timeSync;
}


driver_module_info gHyperVTimeSyncDriverModule = {
	{
		HYPERV_TIMESYNC_DRIVER_MODULE_NAME,
		0,
		NULL
	},

	hyperv_timesync_supports_device,
	hyperv_timesync_register_device,
	hyperv_timesync_init_driver,
	hyperv_timesync_uninit_driver,
	NULL,	// register child devices
	NULL,	// rescan
	NULL	// removed
};
