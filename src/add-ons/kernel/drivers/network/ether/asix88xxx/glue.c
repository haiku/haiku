/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>


extern driver_t* DRIVER_MODULE_NAME(axe, uhub);
extern driver_t* DRIVER_MODULE_NAME(axge, uhub);

extern driver_t *DRIVER_MODULE_NAME(axphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);

HAIKU_FBSD_DRIVERS_GLUE(asix88xxx)
HAIKU_DRIVER_REQUIREMENTS(0);
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(axphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_drivers(dev, drivers);
}


void
__haiku_init_hardware()
{
	driver_t *usb_drivers[] = {
		DRIVER_MODULE_NAME(axe, uhub),
		DRIVER_MODULE_NAME(axge, uhub),
		NULL
	};
	_fbsd_init_hardware_uhub(usb_drivers);
}
