/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <dev/pci/pcivar.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>


HAIKU_FBSD_DRIVER_GLUE(rdc, vte, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

extern driver_t *DRIVER_MODULE_NAME(rdcphy, miibus);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(rdcphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}

