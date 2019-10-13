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
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(44417);
HAIKU_FIRMWARE_NAME_MAP({
	{"iwn100fw", "iwlwifi-100-5.ucode"},
	{"iwn105fw", "iwlwifi-105-6.ucode"},
	{"iwn135fw", "iwlwifi-135-6.ucode"},
	{"iwn1000fw", "iwlwifi-1000-5.ucode"},
	{"iwn2000fw", "iwlwifi-2000-6.ucode"},
	{"iwn2030fw", "iwlwifi-2030-6.ucode"},
	{"iwn4965fw", "iwlwifi-4965-2.ucode"},
	{"iwn5000fw", "iwlwifi-5000-5.ucode"},
	{"iwn5150fw", "iwlwifi-5150-2.ucode"},
	{"iwn6000fw", "iwlwifi-6000-4.ucode"},
	{"iwn6000g2afw", "iwlwifi-6000g2a-6.ucode"},
	{"iwn6000g2bfw", "iwlwifi-6000g2b-6.ucode"},
	{"iwn6050fw", "iwlwifi-6050-5.ucode"}
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
