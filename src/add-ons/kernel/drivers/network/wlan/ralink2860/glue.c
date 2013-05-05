/*
 * Copyright 2010, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 */


#include <sys/bus.h>
#include <sys/kernel.h>

#include <dev/pci/pcivar.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <net80211/ieee80211_var.h>

#include "dev/rt2860/rt2860_io.h"
#include "dev/rt2860/rt2860_reg.h"
#include "dev/rt2860/rt2860_softc.h"


HAIKU_FBSD_WLAN_DRIVER_GLUE(ralink2860, rt2860, pci)
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_WLAN);

HAIKU_FIRMWARE_VERSION(0);
NO_HAIKU_FIRMWARE_NAME_MAP();

NO_HAIKU_FBSD_MII_DRIVER();


static void rt2860_intr_enable(struct rt2860_softc* sc, uint32_t intr_mask)
{
	uint32_t tmp;

	sc->intr_disable_mask &= ~intr_mask;

	tmp = sc->intr_enable_mask & ~sc->intr_disable_mask;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, tmp);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct rt2860_softc* sc = (struct rt2860_softc*)device_get_softc(dev);
	struct ifnet* ifp = sc->ifp;

	// acknowledge interrupts
	uint32_t status = rt2860_io_mac_read(sc, RT2860_REG_SCHDMA_INT_STATUS);
	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_STATUS, status);

	if (status == 0xffffffff || status == 0)
		// device likely went away || not for us
		return 0;

	sc->interrupts++;

	if (!(ifp->if_drv_flags & IFF_DRV_RUNNING))
		return 0;

	rt2860_io_mac_write(sc, RT2860_REG_SCHDMA_INT_MASK, 0);
		// Disable interrupts

	sc->interrupt_status = status;

	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct rt2860_softc* sc = (struct rt2860_softc*)device_get_softc(dev);

	rt2860_intr_enable(sc, 0);
}
