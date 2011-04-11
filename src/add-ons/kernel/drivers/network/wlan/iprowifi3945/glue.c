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
	uint32 r;

	if ((r = WPI_READ(sc, WPI_INTR)) == 0 || r == 0xffffffff)
		return 0;

	atomic_set((int32*)&sc->sc_intr_status, r);

	WPI_WRITE(sc, WPI_MASK, 0);
		// disable interrupts

	return 1;
}
