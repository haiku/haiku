#include <sys/bus.h>
#include <sys/mutex.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>

#include <machine/bus.h>

#include <dev/le/lancereg.h>
#include <dev/le/lancevar.h>
#include <dev/le/am79900var.h>

HAIKU_FBSD_DRIVER_GLUE(pcnet, le, pci);
HAIKU_DRIVER_REQUIREMENTS(0);
NO_HAIKU_FBSD_MII_DRIVER();

NO_HAIKU_REENABLE_INTERRUPTS();

/* from if_le_pci.c */
#define	PCNET_PCI_RDP	0x10
#define	PCNET_PCI_RAP	0x12

struct le_pci_softc {
	struct am79900_softc	sc_am79900;	/* glue to MI code */

	int			sc_rrid;
	struct resource		*sc_rres;
	bus_space_tag_t		sc_regt;
	bus_space_handle_t	sc_regh;

	int			sc_irid;
	struct resource		*sc_ires;
	void			*sc_ih;

	bus_dma_tag_t		sc_pdmat;
	bus_dma_tag_t		sc_dmat;
	bus_dmamap_t		sc_dmam;
};

int HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev) {
	struct le_pci_softc *lesc = (struct le_pci_softc *)device_get_softc(dev);
	HAIKU_INTR_REGISTER_STATE;
	uint16_t value;

	HAIKU_INTR_REGISTER_ENTER();

	/* get current flags */
	bus_space_write_2(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RAP, LE_CSR0);
	bus_space_barrier(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RAP, 2,
	    BUS_SPACE_BARRIER_WRITE);
	value = bus_space_read_2(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RDP);

	/* is there a pending interrupt? */
	if (value & LE_C0_INTR) {
		/* set the new flags, disable interrupts */
		bus_space_write_2(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RAP, LE_CSR0);
		bus_space_barrier(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RAP, 2,
			BUS_SPACE_BARRIER_WRITE);
		bus_space_write_2(lesc->sc_regt, lesc->sc_regh, PCNET_PCI_RDP,
			value & ~LE_C0_INEA);
		lesc->sc_am79900.lsc.sc_lastisr |= value;
	}

	HAIKU_INTR_REGISTER_LEAVE();

	return value & LE_C0_INTR;
}
