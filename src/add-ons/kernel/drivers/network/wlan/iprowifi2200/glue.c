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
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(300);
HAIKU_FIRMWARE_NAME_MAP({
	{"iwi_bss", "ipw2200-bss.fw"},
	{"iwi_ibss", "ipw2200-ibss.fw"},
	{"iwi_monitor", "ipw2200-sniffer.fw"}
});


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwi_softc* sc = (struct iwi_softc*)device_get_softc(dev);
	uint32 r;

	r = CSR_READ_4(sc, IWI_CSR_INTR);
	if (r  == 0 || r == 0xffffffff)
		return 0;

	atomic_set((int32*)&sc->sc_intr_status, r);

	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, 0);
		// disable interrupts
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct iwi_softc* sc = (struct iwi_softc*)device_get_softc(dev);

	CSR_WRITE_4(sc, IWI_CSR_INTR_MASK, IWI_INTR_MASK);
		// reenable interrupts
}
