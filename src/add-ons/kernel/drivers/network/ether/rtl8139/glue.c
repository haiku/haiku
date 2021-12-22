#include <sys/bus.h>
#include <sys/mutex.h>
#include <sys/systm.h>
#include <sys/taskqueue.h>

#include <machine/bus.h>

#include <dev/rl/if_rlreg.h>

HAIKU_FBSD_DRIVER_GLUE(rtl8139, rl, pci);
HAIKU_FBSD_MII_DRIVER(rlphy);
HAIKU_DRIVER_REQUIREMENTS(0);


int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev)
{
	struct rl_softc *sc = device_get_softc(dev);
	uint16_t status;

	status = CSR_READ_2(sc, RL_ISR);
	if (status == 0xffff)
		return 0;
	if (status != 0 && (status & RL_INTRS) == 0) {
		CSR_WRITE_2(sc, RL_ISR, status);
		return 0;
	}
	if ((status & RL_INTRS) == 0)
		return 0;

	CSR_WRITE_2(sc, RL_IMR, 0);
	return 1;
}


void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct rl_softc *sc = device_get_softc(dev);
	RL_LOCK(sc);
	CSR_WRITE_2(sc, RL_IMR, RL_INTRS);
	RL_UNLOCK(sc);
}
