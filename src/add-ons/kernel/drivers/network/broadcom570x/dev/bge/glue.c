/*
 * Copyright 2007-2008, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */

#include <dev/pci/pcivar.h>
#include <sys/bus.h>
#include <sys/malloc.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

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


int
__haiku_disable_interrupts(device_t dev)
{
	struct bge_softc *sc = device_get_softc(dev);

	uint32 notInterrupted = pci_read_config(sc->bge_dev, BGE_PCI_PCISTATE, 4)
		& BGE_PCISTATE_INTR_STATE; 
	// bit of a strange register name. a nonzero actually means 
	// it is _not_ interrupted by the network chip

	if (notInterrupted)
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
