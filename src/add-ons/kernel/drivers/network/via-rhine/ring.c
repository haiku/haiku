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

status_t ring_init(viarhine_private *device)
{
	uint32   size;
	uint16   i;
		
	// Create Transmit Buffer Area
	size = RNDUP(BUFFER_SIZE * TX_BUFFERS, B_PAGE_SIZE);
	if ((device->tx_buf_area = create_area( DevName " tx buffers",
											(void **)&device->tx_buf,
											B_ANY_KERNEL_ADDRESS,
											size,
											B_FULL_LOCK,
											B_READ_AREA | B_WRITE_AREA)) < 0)
	{
#ifdef __VIARHINE_DEBUG__
		debug_printf(DevName ": Create TX Buffer Area Failed %x\n", device->tx_buf_area);
#endif
		return device->tx_buf_area;
	}

	// Initialize TX Buffer Descriptors
	for ( i = 0; i < TX_BUFFERS; i++)
	{
		device->tx_desc[i].tx_status   = 0;
		device->tx_desc[i].desc_length = 0;
		device->tx_desc[i].addr        = mem_virt2phys(&device->tx_buf[i * BUFFER_SIZE], BUFFER_SIZE);
		device->tx_desc[i].next        = mem_virt2phys(&device->tx_desc[i + 1], sizeof(device->tx_desc[0]));
	}
	device->tx_desc[i - 1].next = mem_virt2phys(&device->tx_desc[0], sizeof(device->tx_desc[0]));

	// Create RX Buffer Area
	size = RNDUP(BUFFER_SIZE * RX_BUFFERS, B_PAGE_SIZE);
	if ((device->rx_buf_area = create_area( DevName " rx buffers",
											(void **)&device->rx_buf,
											B_ANY_KERNEL_ADDRESS,
											size,
											B_FULL_LOCK,
											B_READ_AREA | B_WRITE_AREA)) < 0)
	{
#ifdef __VIARHINE_DEBUG__
		debug_printf(DevName ": Create RX Buffer Area Failed %x\n", device->rx_buf_area);
#endif
		delete_area(device->tx_buf_area);
		return device->rx_buf_area;
	}

	// Initialize RX Buffer Descriptors
	for ( i = 0; i < RX_BUFFERS; i++)
	{
		device->rx_desc[i].rx_status   = DescOwn;
		device->rx_desc[i].desc_length = MAX_FRAME_SIZE;
		device->rx_desc[i].addr        = mem_virt2phys(&device->rx_buf[i * BUFFER_SIZE], BUFFER_SIZE);
		device->rx_desc[i].next        = mem_virt2phys(&device->rx_desc[i + 1], sizeof(device->rx_desc[0]));
	}
	device->rx_desc[i - 1].next = mem_virt2phys(&device->rx_desc[0], sizeof(device->tx_desc[0]));

	return B_OK;
}

void ring_free(viarhine_private *device)
{
	delete_area(device->tx_buf_area);
	delete_area(device->rx_buf_area);
}
