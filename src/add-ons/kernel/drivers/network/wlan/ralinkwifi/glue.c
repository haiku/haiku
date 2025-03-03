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


HAIKU_FBSD_WLAN_DRIVERS_GLUE(ralinkwifi)
HAIKU_DRIVER_REQUIREMENTS(FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(0);
HAIKU_FIRMWARE_NAME_MAP({
	{"rt2561fw", "rt2561.ucode"},
	{"rt2561sfw", "rt2561s.ucode"},
	{"rt2661fw", "rt2661.ucode"},
	{"rt2860fw", "rt2860.ucode"},

	{"runfw", "rt2870.ucode"},

	{"/mediatek/mt7601u.bin", "mtw-mt7601u"},
});

NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();

extern driver_t* DRIVER_MODULE_NAME(ral, pci);
extern driver_t* DRIVER_MODULE_NAME(mtw, uhub);
extern driver_t* DRIVER_MODULE_NAME(ural, uhub);
extern driver_t* DRIVER_MODULE_NAME(run, uhub);
extern driver_t* DRIVER_MODULE_NAME(rum, uhub);


status_t
__haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[], driver_t *[]))
{
	driver_t *pci_drivers[] = {
		DRIVER_MODULE_NAME(ral, pci),
		NULL
	};
	driver_t *usb_drivers[] = {
		DRIVER_MODULE_NAME(mtw, uhub),
		DRIVER_MODULE_NAME(ural, uhub),
		DRIVER_MODULE_NAME(run, uhub),
		DRIVER_MODULE_NAME(rum, uhub),
		NULL
	};
	return (*handler)(pci_drivers, usb_drivers);
}


#define RT2661_INT_MASK_CSR			0x346c
#define RT2661_MCU_INT_MASK_CSR		0x0018


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	switch (pci_get_device(dev)) {
		case 0x0201:
		{
			struct rt2560_softc* sc = (struct rt2560_softc*)device_get_softc(dev);

			// disable interrupts
			RAL_WRITE(sc, RT2560_CSR8, 0xffffffff);

			if (!(sc->sc_flags & RT2560_F_RUNNING)) {
				// don't re-enable interrupts if we're shutting down
				return 0;
			}
			break;
		}
		case 0x0301:
		case 0x0302:
		case 0x0401:
		{
			struct rt2661_softc* sc = (struct rt2661_softc*)device_get_softc(dev);

			// disable MAC and MCU interrupts
			RAL_WRITE(sc, RT2661_INT_MASK_CSR, 0xffffff7f);
			RAL_WRITE(sc, RT2661_MCU_INT_MASK_CSR, 0xffffffff);

			if (!(sc->sc_flags & RAL_RUNNING)) {
				// don't re-enable interrupts if we're shutting down
				return 0;
			}
			break;
		}
		default:
		{
			struct rt2860_softc* sc =
				(struct rt2860_softc*)device_get_softc(dev);
			uint32 r = RAL_READ(sc, RT2860_INT_STATUS);
			if (r == 0 || r == 0xffffffff)
				return 0;

			atomic_set((int32*)&sc->sc_intr_status, r);
			break;
		}
	}

	return 1;
}
