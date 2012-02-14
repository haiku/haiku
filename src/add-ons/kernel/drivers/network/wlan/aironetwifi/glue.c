/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>
#include <sys/rman.h>

#include <net/if.h>
#include <net/ethernet.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include <machine/bus.h>

#include <dev/an/if_aironet_ieee.h>
#include <dev/an/if_anreg.h>


// Netgraph function variables.
void (*ng_ether_attach_p)(struct ifnet *ifp) = NULL;
void (*ng_ether_detach_p)(struct ifnet *ifp) = NULL;


HAIKU_FBSD_WLAN_DRIVER_GLUE(aironetwifi, an, pci)
NO_HAIKU_FBSD_MII_DRIVER();
NO_HAIKU_REENABLE_INTERRUPTS();
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct an_softc* sc = (struct an_softc*)device_get_softc(dev);

	if (sc->an_gone)
		return 0;

	CSR_WRITE_2(sc, AN_INT_EN(sc->mpi350), 0);
		// Disable interrupts.

	return 1;
}
