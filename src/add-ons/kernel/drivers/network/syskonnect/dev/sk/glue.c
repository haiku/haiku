/*
 * Copyright 2007, Hugo Santos. All Rights Reserved.
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include "if_skreg.h"

#include "xmaciireg.h"

HAIKU_FBSD_DRIVER_GLUE(syskonnect, skc, pci)

extern driver_t *DRIVER_MODULE_NAME(e1000phy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(xmphy, miibus);

HAIKU_DRIVER_REQUIREMENTS(0);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(xmphy, miibus),
		DRIVER_MODULE_NAME(e1000phy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
__haiku_disable_interrupts(device_t dev)
{
	struct sk_softc* sc = device_get_softc(dev);
	u_int32_t status;
	u_int32_t mask;
	HAIKU_INTR_REGISTER_STATE;

	mask = sc->sk_intrmask;
	HAIKU_INTR_REGISTER_ENTER();
	
	status = CSR_READ_4(sc, SK_ISSR);
	if (status == 0 || status == 0xffffffff || sc->sk_suspended)
	{
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if (sc->sk_if[SK_PORT_A] != NULL)
	{
		mask &= ~SK_INTRS1;
	}

	if (sc->sk_if[SK_PORT_B] != NULL)
	{
		mask &= ~SK_INTRS2;
	}

	mask &= ~SK_ISR_EXTERNAL_REG;
	CSR_WRITE_4(sc, SK_IMR, mask);

	HAIKU_INTR_REGISTER_LEAVE();

	atomic_set((int32 *)&sc->sk_intstatus, status);
	return status & sc->sk_intrmask;
}

void
__haiku_reenable_interrupts(device_t dev)
{
	struct sk_softc *sc = device_get_softc(dev);

	CSR_READ_4(sc, SK_ISSR);
	CSR_WRITE_4(sc, SK_IMR, sc->sk_intrmask);
}


