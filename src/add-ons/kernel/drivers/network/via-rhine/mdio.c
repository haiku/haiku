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

int mdio_read(viarhine_private *device, int phy_id, int regnum)
{
	int boguscnt = 1024;

	/* Wait for a previous command to complete. */
	while ((read8(device->addr + MIICmd) & 0x60) && --boguscnt > 0)
		;

	write8(device->addr + MIICmd,     0x00);
	write8(device->addr + MIIPhyAddr, phy_id);
	write8(device->addr + MIIRegAddr, regnum);
	write8(device->addr + MIICmd,     0x40);    /* Trigger Read */

	boguscnt = 1024;
	while ((read8(device->addr + MIICmd) & 0x40) && --boguscnt > 0)
		;
		
	return read16(device->addr + MIIData);
}

void mdio_write(viarhine_private *device, int phy_id, int regnum, int value)
{
	int boguscnt = 1024;

	if ((phy_id == device->mii_phys[0]) && (regnum == 4))
		device->mii_advertising = value;

	/* Wait for a previous command to complete. */
	while ((read8(device->addr + MIICmd) & 0x60) && --boguscnt > 0)
		;

	write8(device->addr + MIICmd,     0x00);
	write8(device->addr + MIIPhyAddr, phy_id);
	write8(device->addr + MIIRegAddr, regnum);
	write8(device->addr + MIIData,    value);
	write8(device->addr + MIICmd,     0x20);     /* Trigger Write. */
}
