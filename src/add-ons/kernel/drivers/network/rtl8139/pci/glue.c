#include <sys/bus.h>

#include <pci/if_rlreg.h>

HAIKU_FBSD_DRIVER_GLUE(rtl8139exp, rl, pci);
HAIKU_DRIVER_REQUIREMENTS(0);

extern driver_t *DRIVER_MODULE_NAME(rlphy, miibus);

driver_t *
__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(rlphy, miibus)
	};

	return __haiku_probe_miibus(dev, drivers, 1);
}

int
HAIKU_CHECK_DISABLE_INTERRUPTS(device_t dev) {
	struct rl_softc *sc = device_get_softc(dev);

	HAIKU_PROTECT_INTR_REGISTER(CSR_WRITE_2(sc, RL_IMR, 0));

	/* we don't read the status register, so we just assume it was for us. */
	return 1;
}

void
HAIKU_REENABLE_INTERRUPTS(device_t dev)
{
	struct rl_softc *sc = device_get_softc(dev);
	RL_LOCK(sc);
	HAIKU_PROTECT_INTR_REGISTER(CSR_WRITE_2(sc, RL_IMR, RL_INTRS));
	RL_UNLOCK(sc);
}
