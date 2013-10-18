#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/rman.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>

#include <machine/bus.h>
#include <shared.h>

#include <dev/le/lancereg.h>
#include <dev/le/lancevar.h>
#include <dev/le/am79900var.h>

HAIKU_FBSD_DRIVERS_GLUE(pcnet);
HAIKU_DRIVER_REQUIREMENTS(0);

extern driver_t *DRIVER_MODULE_NAME(nsphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(nsphyter, miibus);
extern driver_t *DRIVER_MODULE_NAME(ukphy, miibus);


driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(nsphy, miibus),
		DRIVER_MODULE_NAME(nsphyter, miibus),
		DRIVER_MODULE_NAME(ukphy, miibus),
		NULL
	};

	return __haiku_probe_miibus(dev, drivers);
}

int check_disable_interrupts_le(device_t dev);
void reenable_interrupts_le(device_t dev);

extern int check_disable_interrupts_pcn(device_t dev);
extern void reenable_interrupts_pcn(device_t dev);


extern driver_t *DRIVER_MODULE_NAME(le, pci);
extern driver_t *DRIVER_MODULE_NAME(pcn, pci);


status_t
__haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[]))
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(le, pci),
		DRIVER_MODULE_NAME(pcn, pci),
		NULL
	};
	return (*handler)(drivers);
}


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	switch (dev->device_name[0]) {
		case 'l':
			return check_disable_interrupts_le(dev);
		case 'p':
			return check_disable_interrupts_pcn(dev);
		default:
			break;
	}

	panic("Unsupported device: %#x (%s)!", dev->device_name[0],
		dev->device_name);
	return 0;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	switch (dev->device_name[0]) {
		case 'l':
			break;
		case 'p':
			reenable_interrupts_pcn(dev);
			break;
		default:
			panic("Unsupported device: %#x (%s)!", dev->device_name[0],
				dev->device_name);
			break;
	}
}



/* from if_le_pci.c */
#define	PCNET_PCI_RDP	0x10
#define	PCNET_PCI_RAP	0x12

struct le_pci_softc {
	struct am79900_softc	sc_am79900;	/* glue to MI code */

	struct resource		*sc_rres;

	struct resource		*sc_ires;
	void				*sc_ih;

	bus_dma_tag_t		sc_pdmat;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmam;
};

int
check_disable_interrupts_le(device_t dev)
{
	struct le_pci_softc *lesc = (struct le_pci_softc *)device_get_softc(dev);
	HAIKU_INTR_REGISTER_STATE;
	uint16_t isr;

	HAIKU_INTR_REGISTER_ENTER();

	/* get current flags */
	bus_write_2(lesc->sc_rres, PCNET_PCI_RAP, LE_CSR0);
	bus_barrier(lesc->sc_rres, PCNET_PCI_RAP, 2, BUS_SPACE_BARRIER_WRITE);
	isr = (bus_read_2(lesc->sc_rres, PCNET_PCI_RDP));

	/* is there a pending interrupt? */
	if ((isr & LE_C0_INTR) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* set the new flags, disable interrupts */
	bus_write_2(lesc->sc_rres, PCNET_PCI_RAP, LE_CSR0);
	bus_barrier(lesc->sc_rres, PCNET_PCI_RAP, 2, BUS_SPACE_BARRIER_WRITE);
	bus_write_2(lesc->sc_rres, PCNET_PCI_RDP, isr & ~(LE_C0_INEA));

	lesc->sc_am79900.lsc.sc_lastisr |= isr;

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}
