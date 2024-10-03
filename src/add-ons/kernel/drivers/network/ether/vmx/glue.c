/*
 * Copyright 2024, Jacob Secunda <secundaja@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>

extern driver_t* DRIVER_MODULE_NAME(vmx, pci);

HAIKU_FBSD_DRIVERS_GLUE(vmx);
HAIKU_DRIVER_REQUIREMENTS(0);
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();


status_t
__haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[], driver_t *[]))
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(vmx, pci),
		NULL
	};
	return (*handler)(drivers, NULL);
}
