/*
 * Copyright 2026 John Davis. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "ICDriver.h"


device_manager_info* gDeviceManager;


module_dependency module_dependencies[] = {
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info**)&gDeviceManager },
	{}
};


module_info* modules[] = {
	(module_info*)&gHyperVHeartbeatDriverModule,
	(module_info*)&gHyperVTimeSyncDriverModule,
	NULL
};
