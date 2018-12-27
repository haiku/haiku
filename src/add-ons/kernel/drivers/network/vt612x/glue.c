/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/systm.h>
#include <machine/bus.h>
#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/mutex.h>


#include <dev/vge/if_vgereg.h>
#include <dev/vge/if_vgevar.h>


HAIKU_FBSD_DRIVER_GLUE(vt612x, vge, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);


extern driver_t *DRIVER_MODULE_NAME(ciphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(ciphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
__haiku_disable_interrupts(device_t dev)
{
	struct vge_softc *sc = device_get_softc(dev);

	if (CSR_READ_4(sc, VGE_ISR) == 0)
		return 0;

	CSR_WRITE_4(sc, VGE_IMR, 0x00000000);
	return 1;
}


void
__haiku_reenable_interrupts(device_t dev)
{
	struct vge_softc *sc = device_get_softc(dev);

	CSR_WRITE_4(sc, VGE_IMR, VGE_INTRS);
}

