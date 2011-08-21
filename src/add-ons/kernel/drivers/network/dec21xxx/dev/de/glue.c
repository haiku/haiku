/*
 * Copyright 2011, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Author(s):
 *		Siarzhuk Zharski <imker@gmx.li>
 */


#include <sys/bus.h>
#include <sys/systm.h>
#include <machine/bus.h>
#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>

#include "dc21040reg.h"
#include "if_devar.h"


int check_disable_interrupts_de(device_t dev);
void reenable_interrupts_de(device_t dev);


int
check_disable_interrupts_de(device_t dev)
{
	struct tulip_softc *sc = device_get_softc(dev);
	uint32_t status;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();

	status = TULIP_CSR_READ(sc, csr_status);
	if (status == 0xffffffff) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if (status != 0 && (status & sc->tulip_intrmask) == 0) {
		TULIP_CSR_WRITE(sc, csr_status, status);
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	if ((status & sc->tulip_intrmask) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	TULIP_CSR_WRITE(sc, csr_intr, 0);

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
reenable_interrupts_de(device_t dev)
{
	struct tulip_softc *sc = device_get_softc(dev);
	TULIP_LOCK(sc);
	TULIP_CSR_WRITE(sc, csr_intr, sc->tulip_intrmask);
	TULIP_UNLOCK(sc);
}

