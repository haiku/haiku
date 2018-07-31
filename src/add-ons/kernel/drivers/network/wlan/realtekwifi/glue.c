/*
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <dev/pci/pcivar.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>

#include <dev/rtwn/if_rtwnreg.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(realtekwifi, rtwn, pci)
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
HAIKU_FIRMWARE_NAME_MAP(2) = {
	{"rtwn-rtl8192cfwU", "rtl8192cfwU.ucode"},
	{"rtwn-rtl8192cfwU_B", "rtl8192cfwU_B.ucode"},
};

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();

void	 rtwn_write_4(struct rtwn_softc *, uint16_t, uint32_t);
uint32_t rtwn_read_4(struct rtwn_softc *, uint16_t);

int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct rtwn_softc* sc = (struct rtwn_softc*)device_get_softc(dev);
	uint32_t status;

	status = rtwn_read_4(sc, R92C_HISR);
	if (status == 0 || status == 0xffffffff) {
		return 0;
	}

	atomic_set((int32*)&sc->sc_intr_status, status);

	/* Disable interrupts. */
	rtwn_write_4(sc, R92C_HIMR, 0x00000000);

	return 1;
}
