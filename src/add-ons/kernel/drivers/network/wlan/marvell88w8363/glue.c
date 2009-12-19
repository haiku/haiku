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
NO_HAIKU_FBSD_MII_DRIVER();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct mwl_softc* sc = (struct mwl_softc*)device_get_softc(dev);
	struct mwl_hal* mh = sc->sc_mh;
	HAIKU_INTR_REGISTER_STATE;

	if (sc->sc_invalid) {
		/*
		 * The hardware is not ready/present, don't touch anything.
		 * Note this can happen early on if the IRQ is shared.
		 */
		return 0;
	}

	HAIKU_INTR_REGISTER_ENTER();

	/*
	 * We have to save the isr status right now.
	 * Some devices don't like having the interrupt disabled
	 * before accessing the isr status.
	 *
	 * Those devices return status 0, when status access
	 * occurs after disabling the interrupts with mwl_hal_intrset.
	 *
	 * Note: This glue.c is based on the one for the atheros wlan driver.
	 *       So the comment above isn't based on tests on real marvell hardware.
	 *       But due to the similarities in both drivers I just go the safe
	 *       route here. It doesn't do any harm, but may prevent hard to spot
	 *       bugs.
	 */
	mwl_hal_getisr(mh, &sc->sc_lastisr);	/* NB: clears ISR too */
	if (sc->sc_lastisr == 0) {				/* must be a shared irq */
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	mwl_hal_intrset(mh, 0); // disable further intr's

	HAIKU_INTR_REGISTER_LEAVE();
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct mwl_softc* sc = (struct mwl_softc*)device_get_softc(dev);
	struct mwl_hal* mh = sc->sc_mh;

	mwl_hal_intrset(mh, sc->sc_imask);
}
