/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open SCSI bus manager

	Main file
*/

#include "scsi_internal.h"
#include <pnp_devfs.h>

locked_pool_interface *locked_pool;
device_manager_info *pnp;
fast_log_info *fast_log;

module_dependency module_dependencies[] = {
	{ DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{ LOCKED_POOL_MODULE_NAME, (module_info **)&locked_pool },
	{ FAST_LOG_MODULE_NAME, (module_info **)&fast_log },
	{}
};

_EXPORT 
module_info *modules[] = {
	(module_info *)&scsi_for_sim_module,
	(module_info *)&scsi_bus_module,
	(module_info *)&scsi_device_module,
	(module_info *)&scsi_bus_raw_module,
	NULL
};
