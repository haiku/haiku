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

#include <dev/rtwn/if_rtwnvar.h>
#include <dev/rtwn/pci/rtwn_pci_var.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(realtekwifi, rtwn_pci, pci)
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
HAIKU_FIRMWARE_NAME_MAP({
	{"rtwn-rtl8188eefw", "rtl8188eefw.ucode"},
	{"rtwn-rtl8192cfwE", "rtl8192cfwE.ucode"},
	{"rtwn-rtl8192cfwE_B", "rtl8192cfwE_B.ucode"},
});

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct rtwn_pci_softc* pc = (struct rtwn_pci_softc*)device_get_softc(dev);
	int32_t status, tx_rings;

	status = rtwn_pci_get_intr_status(pc, &tx_rings);
	if (status == 0 && tx_rings == 0)
		return 0;

	atomic_set(&pc->pc_intr_status, status);
	atomic_set(&pc->pc_intr_tx_rings, tx_rings);

	return 1;
}
