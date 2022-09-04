/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>


HAIKU_DRIVER_REQUIREMENTS(FBSD_SWI_TASKQUEUE);
HAIKU_FBSD_DRIVER_GLUE(attansic_l2, ae, pci)
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}
