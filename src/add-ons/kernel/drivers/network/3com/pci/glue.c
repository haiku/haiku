#include <sys/bus.h>

HAIKU_FBSD_DRIVER_GLUE(3com, xl, pci);

extern driver_t *DRIVER_MODULE_NAME(bmtphy, miibus);
extern driver_t *DRIVER_MODULE_NAME(xlphy, miibus);

driver_t *__haiku_select_miibus_driver(device_t dev)
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(bmtphy, miibus),
		DRIVER_MODULE_NAME(xlphy, miibus)
	};

	return __haiku_probe_miibus(dev, drivers, 2);
}

/* TODO interrupt handling */
NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();

HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_SWI_TASKQUEUE);
