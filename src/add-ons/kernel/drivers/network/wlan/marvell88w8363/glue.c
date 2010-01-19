/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <machine/bus.h>

#include <dev/mwl/if_mwlvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(marvell88w8363, mwl, pci)
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_FIRMWARE_NAME_MAP();


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct mwl_softc* sc = (struct mwl_softc*)device_get_softc(dev);
	struct mwl_hal* mh = sc->sc_mh;
	uint32_t intr_status;

	if (sc->sc_invalid) {
		 // The hardware is not ready/present, don't touch anything.
		 // Note this can happen early on if the IRQ is shared.
		return 0;
	}

	mwl_hal_getisr(mh, &intr_status);
		// NB: clears ISR too

	if (intr_status == 0) {
		// must be a shared irq
		return 0;
	}

	atomic_set((int32*)&sc->sc_intr_status, intr_status);

	mwl_hal_intrset(mh, 0);
		// disable further intr's
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct mwl_softc* sc = (struct mwl_softc*)device_get_softc(dev);
	struct mwl_hal* mh = sc->sc_mh;

	mwl_hal_intrset(mh, sc->sc_imask);
}
