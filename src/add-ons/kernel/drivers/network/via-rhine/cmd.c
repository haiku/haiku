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

/*
 * VIA Rhine Commands
 */
void cmd_reset(viarhine_private *device)
{
	write16(device->addr + ChipCmd, CmdReset);
	
#ifdef __VIARHINE_DEBUG__
	debug_printf("cmd_reset\n");
#endif
}

void cmd_getmacaddr(viarhine_private *device)
{
	int i;

	for (i = 0; i < 6; i++)
		device->macaddr.ebyte[i] = read8(device->addr + StationAddr + i);

#ifdef __VIARHINE_DEBUG__
	debug_printf("cmd_getmacaddr (%02x:%02x:%02x:%02x:%02x:%02x)\n", 
						(unsigned char)device->macaddr.ebyte[0],
						(unsigned char)device->macaddr.ebyte[1],
						(unsigned char)device->macaddr.ebyte[2],
						(unsigned char)device->macaddr.ebyte[3],
						(unsigned char)device->macaddr.ebyte[4],
						(unsigned char)device->macaddr.ebyte[5]);
#endif
}
