/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/endian.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_ratectl.h>

#include <dev/iwm/if_iwmreg.h>
#include <dev/iwm/if_iwmvar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(idualwifi7260, iwm, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(1);
HAIKU_FIRMWARE_NAME_MAP({
	{"iwm3160fw", "iwm-3160-17.ucode"},
	{"iwm3168fw", "iwm-3168-22.ucode"},
	{"iwm7260fw", "iwm-7260-17.ucode"},
	{"iwm7265fw", "iwm-7265-17.ucode"},
	{"iwm7265Dfw", "iwm-7265D-22.ucode"},
	{"iwm8000Cfw", "iwm-8000C-22.ucode"},
	{"iwm8265fw", "iwm-8265-22.ucode"},
	{"iwm9000fw", "iwm-9000-34.ucode"},
	{"iwm9260fw", "iwm-9260-34.ucode"},
});


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct iwm_softc* sc = (struct iwm_softc*)device_get_softc(dev);
	uint32 r1, r2;

	r1 = IWM_READ(sc, IWM_CSR_INT);
	/* "hardware gone" (where, fishing?) */
	if (r1 == 0xffffffff || (r1 & 0xfffffff0) == 0xa5a5a5a0)
		return 0;
	r2 = IWM_READ(sc, IWM_CSR_FH_INT_STATUS);

	if (r1 == 0 && r2 == 0)
		return 0; // not for us

	atomic_set((int32*)&sc->sc_intr_status_1, r1);
	atomic_set((int32*)&sc->sc_intr_status_2, r2);

	IWM_WRITE(sc, IWM_CSR_INT_MASK, 0);
		// disable interrupts

	return 1;
}
