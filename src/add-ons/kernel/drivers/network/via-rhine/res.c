/*
 * VIA VT86C100A Rhine-II and VIA VT3043 Rhine Based Card Driver
 * for the BeOS Release 5
 */
 
/*
 * Standard Libraries
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>

/*
 * Driver-Related Libraries
 */
#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>

/*
 * VIA Rhine Libraries
 */
#include "via-rhine.h"

status_t res_allocate(viarhine_private *device)
{
#ifdef __VIARHINE_DEBUG__
	debug_printf("device_allocate_resources\n");
#endif

	device->olock = create_sem(1, "via-rhine output");
	set_sem_owner(device->olock, B_SYSTEM_TEAM);
	device->ilock = create_sem(0, "via-rhine input");
	set_sem_owner(device->ilock, B_SYSTEM_TEAM);

	return B_OK;
}

void res_free(viarhine_private *device)
{
#ifdef __VIARHINE_DEBUG__
	debug_printf("device_free_resources\n");
#endif

	delete_sem(device->olock);
	delete_sem(device->ilock);
}
