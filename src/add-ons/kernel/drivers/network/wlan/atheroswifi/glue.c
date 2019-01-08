/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <dev/ath/if_athvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(atheroswifi, if_ath_pci, pci)
NO_HAIKU_FBSD_MII_DRIVER();
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct ath_softc* sc = (struct ath_softc*)device_get_softc(dev);
	struct ath_hal* ah = sc->sc_ah;
	HAL_INT intr_status;

	if (sc->sc_invalid) {
		// The hardware is not ready/present, don't touch anything.
		// Note this can happen early on if the IRQ is shared.
		return 0;
	}

	if (!ath_hal_intrpend(ah)) {
		// shared irq, not for us
		return 0;
	}

	 // We have to save the isr status right now.
	 // Some devices don't like having the interrupt disabled
	 // before accessing the isr status.
	 //
	 // Those devices return status 0, when status access
	 // occurs after disabling the interrupts with ath_hal_intrset.
	 //
	 // Note: Haiku's pcnet driver uses the same technique of
	 //       appending a sc_lastisr field.
	ath_hal_getisr(ah, &intr_status);
	atomic_set((int32*)&sc->sc_intr_status, intr_status);

	ath_hal_intrset(ah, 0);
		// disable further intr's

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct ath_softc* sc = (struct ath_softc*)device_get_softc(dev);
	struct ath_hal* ah = sc->sc_ah;

	ath_hal_intrset(ah, sc->sc_imask);
}
