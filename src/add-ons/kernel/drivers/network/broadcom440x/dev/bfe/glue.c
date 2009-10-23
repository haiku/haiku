/*
 * Copyright 2008, Jérôme Duval. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>

#include "if_bfereg.h"


HAIKU_FBSD_DRIVER_GLUE(broadcom440x, bfe, pci);

extern driver_t *DRIVER_MODULE_NAME(bmtphy, miibus);

HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);
//HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE | FBSD_SWI_TASKQUEUE);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(bmtphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
__haiku_disable_interrupts(device_t dev)
{
	struct bfe_softc *sc = device_get_softc(dev);
	uint32 istat;
	HAIKU_INTR_REGISTER_STATE;
	
	HAIKU_INTR_REGISTER_ENTER();

	istat = CSR_READ_4(sc, BFE_ISTAT);
	if (istat == 0 || (sc->bfe_ifp->if_drv_flags & IFF_DRV_RUNNING) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}
	
	CSR_WRITE_4(sc, BFE_IMASK, 0);
	CSR_READ_4(sc, BFE_IMASK);

	HAIKU_INTR_REGISTER_LEAVE();
	return 1;
}


void
__haiku_reenable_interrupts(device_t dev)
{
	struct bfe_softc *sc = device_get_softc(dev);
	CSR_WRITE_4(sc, BFE_IMASK, BFE_IMASK_DEF);
	CSR_READ_4(sc, BFE_IMASK);
}


