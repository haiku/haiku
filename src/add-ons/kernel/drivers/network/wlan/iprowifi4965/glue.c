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

#include <dev/iwn/if_iwnreg.h>
#include <dev/iwn/if_iwnvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi4965, iwn, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(44417);
HAIKU_FIRMWARE_NAME_MAP(8) = {
	{"iwn1000fw", "iwlwifi-1000-5.ucode"},
	{"iwn4965fw", "iwlwifi-4965-2.ucode"},
	{"iwn5000fw", "iwlwifi-5000-5.ucode"},
	{"iwn5150fw", "iwlwifi-5150-2.ucode"},
	{"iwn6000fw", "iwlwifi-6000-4.ucode"},
	{"iwn6000g2afw", "iwlwifi-6000g2a-5.ucode"},
	{"iwn6000g2bfw", "iwlwifi-6000g2b-6.ucode"},
	{"iwn6050fw", "iwlwifi-6050-5.ucode"}
};


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwn_softc* sc = (struct iwn_softc*)device_get_softc(dev);
	uint32 r1, r2;

	r1 = IWN_READ(sc, IWN_INT);
	r2 = IWN_READ(sc, IWN_FH_INT);

	if (r1 == 0 && r2 == 0) {
		// not for us
		IWN_WRITE(sc, IWN_INT_MASK, sc->int_mask);
		return 0;
	}

	if (r1 == 0xffffffff) {
		// hardware gone
		return 0;
	}

	atomic_set((int32*)&sc->sc_intr_status_1, r1);
	atomic_set((int32*)&sc->sc_intr_status_2, r2);

	IWN_WRITE(sc, IWN_INT_MASK, 0);
		// disable interrupts

	return 1;
}
