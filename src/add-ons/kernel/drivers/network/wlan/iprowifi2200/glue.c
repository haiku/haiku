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

#include <dev/iwi/if_iwireg.h>
#include <dev/iwi/if_iwivar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi2200, iwi, pci)
NO_HAIKU_FBSD_MII_DRIVER();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES|FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(300);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwi_softc* sc = (struct iwi_softc*)device_get_softc(dev);
	uint32 r;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();
	if ((r = CSR_READ_4(sc, IWI_CSR_INTR)) == 0 || r == 0xffffffff) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* disable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, 0);
	
	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct iwi_softc* sc = (struct iwi_softc*)device_get_softc(dev);

	/* enable interrupts */
	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, IWI_INTR_MASK);
}
