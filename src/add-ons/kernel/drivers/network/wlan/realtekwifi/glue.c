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


extern driver_t* DRIVER_MODULE_NAME(rtwn_pci, pci);
extern driver_t* DRIVER_MODULE_NAME(rtwn_usb, uhub);

HAIKU_FBSD_WLAN_DRIVERS_GLUE(realtekwifi)
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
HAIKU_FIRMWARE_NAME_MAP({
	{"rtwn-rtl8188eefw", "rtwn-rtl8188eefw.ucode"},
	{"rtwn-rtl8188eufw", "rtwn-rtl8188eufw.ucode"},
	{"rtwn-rtl8192cfwE", "rtwn-rtl8192cfwE.ucode"},
	{"rtwn-rtl8192cfwE_B", "rtwn-rtl8192cfwE_B.ucode"},
	{"rtwn-rtl8192cfwU", "rtwn-rtl8192cfwU.ucode"},
	{"rtwn-rtl8192cfwT", "rtwn-rtl8192cfwT.ucode"},
	{"rtwn-rtl8192eufw", "rtwn-rtl8192eufw.ucode"},
	{"rtwn-rtl8812aufw", "rtwn-rtl8812aufw.ucode"},
	{"rtwn-rtl8821aufw", "rtwn-rtl8821aufw.ucode"},
});

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();


void
__haiku_init_hardware()
{
	driver_t *pci_drivers[] = {
		DRIVER_MODULE_NAME(rtwn_pci, pci),
		NULL
	};
	driver_t *usb_drivers[] = {
		DRIVER_MODULE_NAME(rtwn_usb, uhub),
		NULL
	};
	_fbsd_init_hardware_pci(pci_drivers);
	_fbsd_init_hardware_uhub(usb_drivers);
}


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
