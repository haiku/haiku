/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <dev/pci/pcivar.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_amrr.h>
#include <net80211/ieee80211_ratectl.h>

#include <dev/ral/rt2560reg.h>
#include <dev/ral/rt2560var.h>
#include <dev/ral/rt2661var.h>
#include <dev/ral/rt2860reg.h>
#include <dev/ral/rt2860var.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(ralinkwifi, ral, pci)
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
HAIKU_FIRMWARE_NAME_MAP(4) = {
	{"rt2561fw", "rt2561.bin"},
	{"rt2561sfw", "rt2561s.bin"},
	{"rt2661fw", "rt2661.bin"},
	{"rt2860fw", "rt2860.bin"}
};

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();


#define RT2661_INT_MASK_CSR			0x346c
#define RT2661_MCU_INT_MASK_CSR		0x0018


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct rt2560_softc* sc = (struct rt2560_softc*)device_get_softc(dev);
		// sc_ifp is common between context data structures

	switch (pci_get_device(dev)) {
		case 0x0201:
			// disable interrupts
			RAL_WRITE(sc, RT2560_CSR8, 0xffffffff);

			if (!(sc->sc_flags & RT2560_F_RUNNING)) {
				// don't re-enable interrupts if we're shutting down
				return 0;
			}
			break;
		case 0x0301:
		case 0x0302:
		case 0x0401:
			// disable MAC and MCU interrupts
			RAL_WRITE(sc, RT2661_INT_MASK_CSR, 0xffffff7f);
			RAL_WRITE(sc, RT2661_MCU_INT_MASK_CSR, 0xffffffff);

			if (!(sc->sc_flags & RAL_RUNNING)) {
				// don't re-enable interrupts if we're shutting down
				return 0;
			}
			break;
		default:
		{
			uint32 r;
			struct rt2860_softc* sc =
				(struct rt2860_softc*)device_get_softc(dev);
			r = RAL_READ(sc, RT2860_INT_STATUS);
			if (r == 0 || r == 0xffffffff)
				return 0;

			atomic_set((int32*)&sc->sc_intr_status, r);
			break;
		}
	}

	return 1;
}
