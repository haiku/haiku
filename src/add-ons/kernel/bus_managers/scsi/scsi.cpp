/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include "scsi_internal.h"

device_manager_info *pnp;

module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};

module_info *modules[] = {
	(module_info *)&scsi_for_sim_module,
	(module_info *)&scsi_bus_module,
	(module_info *)&scsi_device_module,
	(module_info *)&gSCSIBusRawModule,
	NULL
};
