/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include "if_fxpreg.h"
#include "if_fxpvar.h"


HAIKU_FBSD_DRIVER_GLUE(ipro100, fxp, pci)
HAIKU_DRIVER_REQUIREMENTS(0);
HAIKU_FBSD_MII_DRIVER(inphy);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct fxp_softc *sc = device_get_softc(dev);

	uint8_t statack = CSR_READ_1(sc, FXP_CSR_SCB_STATACK);
	if (statack == 0)
		return 0;

	CSR_WRITE_1(sc, FXP_CSR_SCB_INTRCNTL, FXP_SCB_INTR_DISABLE);
	atomic_set((int32*)&sc->sc_statack, statack);
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct fxp_softc *sc = device_get_softc(dev);

	CSR_WRITE_1(sc, FXP_CSR_SCB_INTRCNTL, 0);
}
