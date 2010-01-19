/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>

#include <dev/bwi/bitops.h>
#include <dev/bwi/if_bwireg.h>
#include <dev/bwi/if_bwivar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(broadcom43xx, bwi, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
NO_HAIKU_FIRMWARE_NAME_MAP();


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct bwi_softc* sc = (struct bwi_softc*)device_get_softc(dev);
	struct ifnet* ifp = sc->sc_ifp;
	uint32 intr_status;

	if ((ifp->if_drv_flags & IFF_DRV_RUNNING) == 0
		|| (sc->sc_flags & BWI_F_STOP))
		return 0;

	intr_status = CSR_READ_4(sc, BWI_MAC_INTR_STATUS);
	if (intr_status == 0xffffffff) {
		// Not for us
		return 0;
	}

	intr_status &= CSR_READ_4(sc, BWI_MAC_INTR_MASK);
	if (intr_status == 0) {
		// nothing interesting
		return 0;
	}

	atomic_set((int32*)&sc->sc_intr_status, intr_status);

	CSR_CLRBITS_4(sc, BWI_MAC_INTR_MASK, BWI_ALL_INTRS);
		// Disable all interrupts

	return 1;
}
