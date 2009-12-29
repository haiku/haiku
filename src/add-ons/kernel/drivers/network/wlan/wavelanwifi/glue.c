/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <machine/bus.h>

#include <dev/wi/if_wavelan_ieee.h>
#include <dev/wi/if_wireg.h>
#include <dev/wi/if_wivar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(wavelanwifi, wi, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct wi_softc* sc = (struct wi_softc*)device_get_softc(dev);
	struct ifnet* ifp = sc->sc_ifp;

	if (sc->wi_gone || !sc->sc_enabled || (ifp->if_flags & IFF_UP) == 0) {
		CSR_WRITE_2(sc, WI_INT_EN, 0);
		CSR_WRITE_2(sc, WI_EVENT_ACK, 0xFFFF);
		return 0;
	}

	CSR_WRITE_2(sc, WI_INT_EN, 0);
		// Disable interrupts.

	return 1;
}
