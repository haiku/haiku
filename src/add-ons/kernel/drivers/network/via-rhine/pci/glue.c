/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>

#include "if_vrreg.h"


HAIKU_FBSD_DRIVER_GLUE(via_rhine, vr, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);


extern driver_t *DRIVER_MODULE_NAME(ciphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(ciphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus)
	};

	return __haiku_probe_miibus(dev, drivers, 2);
}


int
__haiku_disable_interrupts(device_t dev)
{
	struct vr_softc *sc = device_get_softc(dev);

	if (CSR_READ_2(sc, VR_ISR) == 0)
		return 0;

	CSR_WRITE_2(sc, VR_IMR, 0x0000);
	return 1;
}


void
__haiku_reenable_interrupts(device_t dev)
{
	struct vr_softc *sc = device_get_softc(dev);

	CSR_WRITE_2(sc, VR_IMR, VR_INTRS);
}


