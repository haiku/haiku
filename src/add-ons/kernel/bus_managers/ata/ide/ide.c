/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*!	Contains interface used by IDE controller driver. */

#include "ide_internal.h"
#include "ide_sim.h"


device_manager_info *pnp;


#if !_BUILDING_kernel && !BOOT
module_info *modules[] = {
	(module_info *)&ide_for_controller_module,
	(module_info *)&ide_sim_module,
	NULL
};
#endif
