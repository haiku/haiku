/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <dev/pci/pcivar.h>
#include <sys/bus.h>
#include <sys/rman.h>

#include "if_bgereg.h"

HAIKU_FBSD_DRIVER_GLUE(broadcom570x, bge, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);


extern driver_t *DRIVER_MODULE_NAME(brgphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(brgphy, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


// copied from if_bge.c
static void
bge_writembx(struct bge_softc *sc, int off, int val)
{
	if (sc->bge_asicrev == BGE_ASICREV_BCM5906)
		off += BGE_LPMBX_IRQ0_HI - BGE_MBX_IRQ0_HI;

	CSR_WRITE_4(sc, off, val);
}


int
__haiku_disable_interrupts(device_t dev)
{
	struct bge_softc *sc = device_get_softc(dev);

	uint32 statusword = CSR_READ_4(sc, BGE_MAC_STS) & BGE_MACSTAT_LINK_CHANGED;

	if ((sc->bge_ifp->if_drv_flags & IFF_DRV_RUNNING) == 0 && !statusword
		&& (pci_read_config(sc->bge_dev, BGE_PCI_PCISTATE,4) & BGE_PCISTATE_INTR_STATE))
		return 0;

	BGE_SETBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_MASK_PCI_INTR);
	bge_writembx(sc, BGE_MBX_IRQ0_LO, 1);

	return 1;
}


void
__haiku_reenable_interrupts(device_t dev)
{
	struct bge_softc *sc = device_get_softc(dev);
	BGE_SETBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_CLEAR_INTA);
	BGE_CLRBIT(sc, BGE_PCI_MISC_CTL, BGE_PCIMISCCTL_MASK_PCI_INTR);
	bge_writembx(sc, BGE_MBX_IRQ0_LO, 0);
}
