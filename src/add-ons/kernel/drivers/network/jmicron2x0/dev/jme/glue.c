/*
 * Copyright 2009, Axel Dörfler. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/rman.h>

#include "if_jmereg.h"

HAIKU_FBSD_DRIVER_GLUE(jmicron2x0, jme, pci);

extern driver_t *DRIVER_MODULE_NAME(jmephy, miibus);

HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);


#if 0
driver_t*
__haiku_select_miibus_driver(device_t dev)
{
	driver_t* drivers[] = {
		DRIVER_MODULE_NAME(jmephy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}
#endif
