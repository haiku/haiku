/*
 * Copyright 2009 S.Zharski <imker@gmx.li>
 * Copyright 2018, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <PCI.h>

#include <sys/bus.h>
#include <sys/rman.h>
#include <sys/systm.h>

#include <machine/bus.h>

#include <net/if.h>
#include <net/if_media.h>

#include <dev/pci/pcivar.h>
#include <dev/sge/if_sgereg.h>


HAIKU_FBSD_DRIVER_GLUE(sis19x, sge, pci);
HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE);
NO_HAIKU_REENABLE_INTERRUPTS();


extern pci_module_info *gPci;
extern driver_t* DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct sge_softc *sc = device_get_softc(dev);
	uint32_t status;

	status = CSR_READ_4(sc, IntrStatus);
	if (status == 0xFFFFFFFF || (status & SGE_INTRS) == 0) {
		/* Not ours. */
		return 0;
	}

	/* Acknowledge interrupts. */
	CSR_WRITE_4(sc, IntrStatus, status);
	/* Disable further interrupts. */
	CSR_WRITE_4(sc, IntrMask, 0);

	sc->haiku_interrupt_status = status;
	return 1;
}


int
haiku_sge_get_mac_addr_apc(device_t dev, uint8_t* dest, int* rgmii)
{
	// SiS96x can use APC CMOS RAM to store MAC address,
	// this is accessed through ISA bridge.

	// look for PCI-ISA bridge
	uint16 ids[] = { 0x0965, 0x0966, 0x0968 };

	pci_info pciInfo = {0};
	long i;
	for (i = 0; B_OK == (*gPci->get_nth_pci_info)(i, &pciInfo); i++) {
		size_t idx;
		if (pciInfo.vendor_id != 0x1039)
			continue;

		for (idx = 0; idx < B_COUNT_OF(ids); idx++) {
			if (pciInfo.device_id == ids[idx]) {
				size_t i;
				// enable ports 0x78 0x79 to access APC registers
				uint32 reg = gPci->read_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1);
				reg &= ~0x02;
				gPci->write_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1, reg);
				snooze(50);
				reg = gPci->read_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1);

				// read factory MAC address
				for (i = 0; i < 6; i++) {
					gPci->write_io_8(0x78, 0x09 + i);
					dest[i] = gPci->read_io_8(0x79);
				}

				// check MII/RGMII
				gPci->write_io_8(0x78, 0x12);
				if ((gPci->read_io_8(0x79) & 0x80) != 0)
					*rgmii = 1;

				// close access to APC registers
				gPci->write_pci_config(pciInfo.bus,
					pciInfo.device, pciInfo.function, 0x48, 1, reg);

				return B_OK;
			}
		}
	}

	return B_ERROR;
}
