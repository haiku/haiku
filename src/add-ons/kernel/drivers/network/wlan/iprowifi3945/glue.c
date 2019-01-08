/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * Copyright 2018, Haiku, Inc.
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>
#include <net80211/ieee80211_ratectl.h>

#include <dev/wpi/if_wpireg.h>
#include <dev/wpi/if_wpivar.h>


HAIKU_FBSD_WLAN_DRIVER_GLUE(iprowifi3945, wpi, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(2144);
HAIKU_FIRMWARE_NAME_MAP(1) = {{"wpifw", "iwlwifi-3945-2.ucode"}};


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct wpi_softc* sc = (struct wpi_softc*)device_get_softc(dev);
	uint32 r1, r2;

	r1 = WPI_READ(sc, WPI_INT);

	if (__predict_false(r1 == 0xffffffff ||
			(r1 & 0xfffffff0) == 0xa5a5a5a0))
		return 0;	/* Hardware gone! */

	r2 = WPI_READ(sc, WPI_FH_INT);

	if (r1 == 0 && r2 == 0)
		return 0;	/* Interrupt not for us. */

	/* Disable interrupts. */
	WPI_WRITE(sc, WPI_INT_MASK, 0);

	atomic_set((int32*)&sc->sc_intr_status_1, r1);
	atomic_set((int32*)&sc->sc_intr_status_2, r2);

	return 1;
}
