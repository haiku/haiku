/*
 * Copyright 2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#include <module.h>
#include <device_manager.h>
#include <bus/SCSI.h>


#define MODULE_NAME "example_scsi"


static scsi_for_sim_interface *sSimInterface;
static device_manager_info *sDeviceManager;


// module functions
static status_t
example_std_ops(int32 op, ...)
{
	dprintf(MODULE_NAME": std ops\n");

	switch (op) {
		case B_MODULE_INIT: {
			dprintf(MODULE_NAME": B_MODULE_INIT\n");
			return B_OK;
		}

		case B_MODULE_UNINIT: {
			dprintf(MODULE_NAME": B_MODULE_UNINIT\n");
			return B_OK;
		}
	}

	return B_ERROR;
}


// driver functions
static float
example_supports_device(device_node_handle parent, bool *noConnection)
{
	dprintf(MODULE_NAME": supports device\n");
	return 0.0f;
}


static status_t
example_register_device(device_node_handle parent)
{
	dprintf(MODULE_NAME": register device\n");
	return B_OK;
}


static status_t
example_init_driver(device_node_handle node, void *userCookie, void **cookie)
{
	dprintf(MODULE_NAME": init driver\n");
	return B_OK;
}


static status_t
example_uninit_driver(void *cookie)
{
	dprintf(MODULE_NAME": uninit driver\n");
	return B_OK;
}


static void
example_device_removed(device_node_handle node, void *cookie)
{
	dprintf(MODULE_NAME": device removed\n");
}


static void
example_device_cleanup(device_node_handle node)
{
	dprintf(MODULE_NAME": device cleanup\n");
}


static void
example_get_supported_paths(const char ***busses, const char ***devices)
{
	static const char *sBusses[] = { "pci", NULL };
	static const char *sDevices[] = { "drivers/dev/example", NULL };

	dprintf(MODULE_NAME": get supported paths\n");
	*busses = sBusses;
	*devices = sDevices;
}


// sim functions
static void
example_scsi_io(scsi_sim_cookie cookie, scsi_ccb *ccb)
{
	dprintf(MODULE_NAME": scsi io\n");
}


static uchar
example_abort(scsi_sim_cookie cookie, scsi_ccb *ccbToAbort)
{
	dprintf(MODULE_NAME": abort\n");
	return 0;
}


static uchar
example_reset_device(scsi_sim_cookie cookie, uchar targetID, uchar targetLUN)
{
	dprintf(MODULE_NAME": reset device\n");
	return 0;
}


static uchar
example_term_io(scsi_sim_cookie cookie, scsi_ccb *ccbToTerminate)
{
	dprintf(MODULE_NAME": terminate io\n");
	return 0;
}


static uchar
example_path_inquiry(scsi_sim_cookie cookie, scsi_path_inquiry *inquiryData)
{
	dprintf(MODULE_NAME": path inquiry\n");
	return 0;
}


static uchar
example_scan_bus(scsi_sim_cookie cookie)
{
	dprintf(MODULE_NAME": scan bus\n");
	return 0;
}


static uchar
example_reset_bus(scsi_sim_cookie cookie)
{
	dprintf(MODULE_NAME": reset bus\n");
	return 0;
}


static void
example_get_restrictions(scsi_sim_cookie cookie, uchar targetID, bool *isATAPI,
	bool *noAutosense, uint32 *maxBlocks)
{
	dprintf(MODULE_NAME": get restrictions\n");
}


static status_t
example_ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void *buffer,
	size_t bufferLength)
{
	dprintf(MODULE_NAME": io control\n");
	return B_ERROR;
}


// module management
module_dependency module_dependencies[] = {
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&sSimInterface },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&sDeviceManager },
	{}
};


scsi_sim_interface example_sim = {
	{									// driver_module_info
		{									// module_info
			"busses/scsi/example/v1",			// module name
			0,									// module flags
			example_std_ops						// module standard ops
		},

		example_supports_device,			// supports device
		example_register_device,			// register device

		example_init_driver,				// init driver
		example_uninit_driver,				// uninit driver

		example_device_removed,				// device removed
		example_device_cleanup,				// device cleanup

		example_get_supported_paths			// get supported paths
	},

	example_scsi_io,					// scsi io
	example_abort,						// abort
	example_reset_device,				// reset device
	example_term_io,					// terminate io

	example_path_inquiry,				// path inquiry
	example_scan_bus,					// scan bus
	example_reset_bus,					// reset bus

	example_get_restrictions,			// get restrictions

	example_ioctl						// io control
};


module_info *modules[] = {
	(module_info *)&example_sim,
	NULL
};
