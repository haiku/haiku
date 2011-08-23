#include <sys/bus.h>


extern driver_t *DRIVER_MODULE_NAME(em, pci);
extern driver_t *DRIVER_MODULE_NAME(lem, pci);

HAIKU_FBSD_DRIVERS_GLUE(ipro1000);

NO_HAIKU_CHECK_DISABLE_INTERRUPTS();
NO_HAIKU_REENABLE_INTERRUPTS();
NO_HAIKU_FBSD_MII_DRIVER();


status_t
__haiku_handle_fbsd_drivers_list(status_t (*handler)(driver_t *[]))
{
	driver_t *drivers[] = {
		DRIVER_MODULE_NAME(em, pci),
		DRIVER_MODULE_NAME(lem, pci),
		NULL
	};
	return (*handler)(drivers);
}


#ifdef EM_FAST_INTR
	HAIKU_DRIVER_REQUIREMENTS(FBSD_TASKQUEUES | FBSD_FAST_TASKQUEUE);
#else
	HAIKU_DRIVER_REQUIREMENTS(0);
#endif
