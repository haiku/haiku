#include <sys/bus.h>

#include <net/ethernet.h>
#include <net/if.h>
#include <net/if_media.h>

#include <machine/bus.h>

#include <dev/pcn/if_pcnreg.h>

#define PCN_CSR_SETBIT(sc, reg, x)			\
	pcn_csr_write(sc, reg, pcn_csr_read(sc, reg) | (x))

#define PCN_CSR_CLRBIT(sc, reg, x)			\
	pcn_csr_write(sc, reg, pcn_csr_read(sc, reg) & ~(x))


int check_disable_interrupts_pcn(device_t dev);
void reenable_interrupts_pcn(device_t dev);


static u_int32_t
pcn_csr_read(sc, reg)
	struct pcn_softc	*sc;
	int			reg;
{
	CSR_WRITE_4(sc, PCN_IO32_RAP, reg);
	return(CSR_READ_4(sc, PCN_IO32_RDP));
}


static void
pcn_csr_write(sc, reg, val)
	struct pcn_softc	*sc;
	int			reg;
	int			val;
{
	CSR_WRITE_4(sc, PCN_IO32_RAP, reg);
	CSR_WRITE_4(sc, PCN_IO32_RDP, val);
	return;
}


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

	/* set the new flags, disable interrupts */
	PCN_CSR_CLRBIT(sc, PCN_CSR_CSR, PCN_CSR_INTEN);

	HAIKU_INTR_REGISTER_LEAVE();

	return 1;
}


void
reenable_interrupts_pcn(device_t dev)
{
	struct pcn_softc *sc = device_get_softc(dev);
	PCN_CSR_SETBIT(sc, PCN_CSR_CSR, PCN_CSR_INTEN);
}
