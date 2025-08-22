/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>


extern driver_t* DRIVER_MODULE_NAME(aue, uhub);

HAIKU_FBSD_DRIVERS_GLUE(pegasus)
HAIKU_DRIVER_REQUIREMENTS(0);
HAIKU_FBSD_MII_DRIVER(acphy);
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();


void
__haiku_init_hardware()
{
	driver_t *usb_drivers[] = {
		DRIVER_MODULE_NAME(aue, uhub),
		NULL
	};
	_fbsd_init_hardware_uhub(usb_drivers);
}
