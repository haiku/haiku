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


int
__haiku_disable_interrupts(device_t dev)
{
	struct fxp_softc *sc = device_get_softc(dev);

	// TODO: check interrupt status!
	CSR_WRITE_1(sc, FXP_CSR_SCB_INTRCNTL, FXP_SCB_INTR_DISABLE);
	return 1;
}


void
__haiku_reenable_interrupts(device_t dev)
{
	struct fxp_softc *sc = device_get_softc(dev);

	CSR_WRITE_1(sc, FXP_CSR_SCB_INTRCNTL, 0);
}


HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);
HAIKU_FBSD_MII_DRIVER(inphy);
