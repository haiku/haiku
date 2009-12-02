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
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES|FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);

int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct bwi_softc* sc = (struct bwi_softc*)device_get_softc(dev);
	HAIKU_INTR_REGISTER_STATE;
	
	HAIKU_INTR_REGISTER_ENTER();
	if (CSR_READ_4(sc, BWI_MAC_INTR_STATUS) == 0xffffffff) {
		/* Not for us */
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* Disable all interrupts */
	CSR_CLRBITS_4(sc, BWI_MAC_INTR_MASK, BWI_ALL_INTRS);
	
	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct bwi_softc* sc = (struct bwi_softc*)device_get_softc(dev);

	/* enable interrupts */
	CSR_SETBITS_4(sc, BWI_MAC_INTR_MASK, BWI_INIT_INTRS);
}
