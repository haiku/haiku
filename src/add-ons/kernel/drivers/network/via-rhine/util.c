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

uint32 mem_virt2phys(void *addr, unsigned int length)
{
	physical_entry tbl;

	get_memory_map(addr, length, &tbl, 1);
	return (uint32)tbl.address;
}

void rx_setmode(viarhine_private *device)
{
	uint8 rx_mode; // Note: 0x02 = Accept Runt, 0x01 = Accept Errors
	
#if defined(__VIARHINE_PROMISCUOUS___)
	{
		rx_mode  = 0x1C;
	}
#elif defined(__VIARHINE_ALLOW_MULTICAST__)
	{
		uint32  nMCFilter[2];   /* Multicast Hash Filter */
		int     i;
		
		memset(nMCFilter, 0, sizeof(nMCFilter);
		for (i = 0; i < device->nMulti; i++)
		{
#error TODO: Code Multicast Filter
		}
	}

	rx_mode = 0x0C;
#else

	write32(device->addr + MulticastFilter0, 0);	
	write32(device->addr + MulticastFilter1, 0);	
	rx_mode = 0x1A;
#endif

	write8(device->addr + RxConfig, device->rx_thresh | rx_mode);
}

void duplex_check(viarhine_private *device)
{
	int mii_reg5;
	int negotiated;
	int duplex;

	mii_reg5   = mdio_read(device, device->mii_phys[0], 5);
	negotiated = mii_reg5 & device->mii_advertising;

	if (device->duplex_lock || mii_reg5 == 0xffff)
		return;

	duplex = (negotiated & 0x0100) || (negotiated & 0x01C0) == 0x0040;
	if (device->duplex_full != duplex)
	{
		device->duplex_full = duplex;

		if (duplex)
			device->cmd |= CmdFDuplex;
		else
			device->cmd &= ~CmdFDuplex;

		write16(device->addr + ChipCmd, device->cmd);
	}
}

bool is_mine(viarhine_private *device, char *addr)
{
	static char broadcast[6] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

	if (memcmp(&device->macaddr, addr, 6) == 0)
	{
		return true;
	}

	if (memcmp(broadcast, addr, 6) == 0)
	{
		return true;
	}

	return false;
}

#ifdef __VIARHINE_DEBUG__
int debug_printf(char *format, ...)
{
	va_list vl;
	char    c[2048];
	int     i;
	static int f = 0;
	
	va_start(vl, format);
	i = vsprintf(c, format, vl);
	va_end(vl);

	if (f == 0)
		f = open("/boot/via-rhine.log", O_WRONLY | O_APPEND | O_TEXT);

	write(f, c, strlen(c));
	return i;
}
#endif
