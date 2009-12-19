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
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);
HAIKU_FIRMWARE_VERSION(2144);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct wpi_softc* sc = (struct wpi_softc*)device_get_softc(dev);
	uint32 r;
	HAIKU_INTR_REGISTER_STATE;

	HAIKU_INTR_REGISTER_ENTER();
	if ((r = WPI_READ(sc, WPI_INTR)) == 0 || r == 0xffffffff) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* disable interrupts */
	WPI_WRITE(sc, WPI_MASK, 0);

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct wpi_softc* sc = (struct wpi_softc*)device_get_softc(dev);

	/* enable interrupts */
	WPI_WRITE(sc, WPI_MASK, WPI_INTR_MASK);
}
