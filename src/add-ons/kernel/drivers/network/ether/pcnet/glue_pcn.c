#include <sys/bus.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>

#include <machine/bus.h>

#include <dev/pcn/if_pcnreg.h>


int check_disable_interrupts_pcn(device_t dev);
void reenable_interrupts_pcn(device_t dev);


int
check_disable_interrupts_pcn(device_t dev)
{
	struct pcn_softc *sc = device_get_softc(dev);
	HAIKU_INTR_REGISTER_STATE;
	uint32_t status;

	HAIKU_INTR_REGISTER_ENTER();

	/* get current flags */
	CSR_WRITE_4(sc, PCN_IO32_RAP, PCN_CSR_CSR);
	status = CSR_READ_4(sc, PCN_IO32_RDP);

	/* is there a pending interrupt? */
	if ((status & PCN_CSR_INTR) == 0) {
		HAIKU_INTR_REGISTER_LEAVE();
		return 0;
	}

	/* disable interrupts */
	CSR_WRITE_4(sc, PCN_IO32_RDP, 0);

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
reenable_interrupts_pcn(device_t dev)
{
	struct pcn_softc *sc = device_get_softc(dev);
	CSR_WRITE_4(sc, PCN_IO32_RAP, PCN_CSR_CSR);
	CSR_WRITE_4(sc, PCN_IO32_RDP, PCN_CSR_INTEN);
}
