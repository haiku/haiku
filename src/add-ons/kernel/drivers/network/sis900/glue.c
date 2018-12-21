/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/sis/if_sisreg.h>


HAIKU_FBSD_DRIVER_GLUE(sis900, sis, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE);
NO_HAIKU_REENABLE_INTERRUPTS();


extern driver_t* DRIVER_MODULE_NAME(icsphy, miibus);
extern driver_t* DRIVER_MODULE_NAME(nsphyter, miibus);
extern driver_t* DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(icsphy, miibus),
		DRIVER_MODULE_NAME(nsphyter, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct sis_softc *sc = device_get_softc(dev);
	uint32_t status;

	/* Reading the ISR register clears all interrupts. */
	status = CSR_READ_4(sc, SIS_ISR);
	if ((status & SIS_INTRS) == 0) {
		/* Not ours. */
		return 0;
	}

	/* Disable interrupts. */
	CSR_WRITE_4(sc, SIS_IER, 0);

	sc->haiku_interrupt_status = status;
	return 1;
}
