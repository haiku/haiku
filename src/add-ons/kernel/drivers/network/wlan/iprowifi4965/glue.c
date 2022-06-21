/*
 * Copyright 2009, Colin Günther, coling@gmx.de. All rights reserved.
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_ratectl.h>

#include <dev/iwn/if_iwnreg.h>
#include <dev/iwn/if_iwnvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi4965, iwn, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(44417);
HAIKU_FIRMWARE_NAME_MAP({
	{"iwn100fw", "iwlwifi-100-39.ucode"},
	{"iwn105fw", "iwlwifi-105-6-18.ucode"},
	{"iwn135fw", "iwlwifi-135-6-18.ucode"},
	{"iwn1000fw", "iwlwifi-1000-39.ucode"},
	{"iwn2000fw", "iwlwifi-2000-18.ucode"},
	{"iwn2030fw", "iwlwifi-2030-18.ucode"},
	{"iwn4965fw", "iwlwifi-4965-228.ucode"},
	{"iwn5000fw", "iwlwifi-5000-8.ucode"},
	{"iwn5150fw", "iwlwifi-5150-8.ucode"},
	{"iwn6000fw", "iwlwifi-6000-9.ucode"},
	{"iwn6000g2afw", "iwlwifi-6000g2a-18.ucode"},
	{"iwn6000g2bfw", "iwlwifi-6000g2b-18.ucode"},
	{"iwn6050fw", "iwlwifi-6050-41.ucode"}
});


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwn_softc* sc = (struct iwn_softc*)device_get_softc(dev);
	uint32 r1, r2;

	/* Disable interrupts. */
	IWN_WRITE(sc, IWN_INT_MASK, 0);

	r1 = IWN_READ(sc, IWN_INT);
	if (r1 == 0xffffffff || (r1 & 0xfffffff0) == 0xa5a5a5a0) {
		return 0; /* Hardware gone! */
	}
	r2 = IWN_READ(sc, IWN_FH_INT);

	if (r1 == 0 && r2 == 0) {
		// not for us
		if (sc->sc_flags & IWN_FLAG_RUNNING)
			IWN_WRITE(sc, IWN_INT_MASK, sc->int_mask);
		return 0;
	}

	atomic_set((int32*)&sc->sc_intr_status_1, r1);
	atomic_set((int32*)&sc->sc_intr_status_2, r2);

	return 1;
}
