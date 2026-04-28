/*
 * Copyright 2026, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/haiku-module.h>


HAIKU_FBSD_WLAN_DRIVERS_GLUE(zydwifi1211)
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
NO_HAIKU_FIRMWARE_NAME_MAP();

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

extern driver_t* DRIVER_MODULE_NAME(zyd, uhub);


void
__haiku_init_hardware()
{
	driver_t *usb_drivers[] = {
		DRIVER_MODULE_NAME(zyd, uhub),
		NULL
	};
	_fbsd_init_hardware_uhub(usb_drivers);
}
