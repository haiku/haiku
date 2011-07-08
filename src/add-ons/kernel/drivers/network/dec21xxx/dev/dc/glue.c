/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <machine/bus.h>

#include "if_dcreg.h"

HAIKU_FBSD_DRIVER_GLUE(dec21xxx, dc, pci)

HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE | FBSD_SWI_TASKQUEUE);

extern driver_t *DRIVER_MODULE_NAME(acphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(amphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(dcphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(pnphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(acphy, miibus),
		DRIVER_MODULE_NAME(amphy, miibus),
		DRIVER_MODULE_NAME(dcphy, miibus),
		DRIVER_MODULE_NAME(pnphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct dc_softc *sc = device_get_softc(dev);
	uint16_t status;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();

	status = CSR_READ_4(sc, DC_ISR);
	if (status == 0xffff) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if (status != 0 && (status & DC_INTRS) == 0) {
		CSR_WRITE_4(sc, DC_ISR, status);
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if ((status & DC_INTRS) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	CSR_WRITE_4(sc, DC_IMR, 0);

	HAIKU_INTR_REGISTER_LEAVE();
	
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct dc_softc *sc = device_get_softc(dev);
	DC_LOCK(sc);
	CSR_WRITE_4(sc, DC_IMR, DC_INTRS);
	DC_UNLOCK(sc);
}
