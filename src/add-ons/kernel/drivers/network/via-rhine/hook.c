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

int32 nOpenMask = 0;

int32 hook_interrupt(void *_device)
{
	viarhine_private *device    = (viarhine_private*)_device;
	int32             handled   = B_UNHANDLED_INTERRUPT;
	int16             worklimit = 20;
	uint16            reg;
	uint32            i, j;
	int               wakeup_reader = 0;
	int               wakeup_writer = 0;

	do
	{
		reg = read16(device->addr + IntrStatus);
	
		// Acknowledge All of the Current Interrupt Sources ASAP.
		write16(device->addr + IntrStatus, reg & 0xffff);

		if (reg == 0)
			break;

		if (reg & IntrRxDone)
		{
			wakeup_reader++;
			//write16(device->addr + ChipCmd, CmdRxDemand | read16(device->addr + ChipCmd));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (reg & IntrTxDone)
		{
			wakeup_writer++;
			//write16(device->addr + ChipCmd, CmdTxDemand | read16(device->addr + ChipCmd));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}

		if (reg & (IntrRxErr | IntrRxDropped | IntrRxWakeUp | IntrRxEmpty | IntrRxNoBuf))
		{
			write16(device->addr + ChipCmd, CmdRxDemand | read16(device->addr + ChipCmd));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (reg & IntrTxAbort)
		{
			/* Just restart Tx. */
			write16(device->addr + ChipCmd, CmdTxDemand | read16(device->addr + ChipCmd));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
		if (reg & IntrTxUnderrun)
		{
			if (device->tx_thresh < 0xE0)
				write8(device->addr + TxConfig, device->tx_thresh += 0x20);
			write16(device->addr + ChipCmd, CmdTxDemand | read16(device->addr + ChipCmd));
			handled = B_HANDLED_INTERRUPT;
			continue;
		}
/*		if (reg & IntrTxDone)
		{
			if (device->tx > 0)
				device->tx--;

			if (device->tx > 0)
				write16(device->addr + ChipCmd, CmdTxDemand | read16(device->addr + ChipCmd));
		}
*/
		if (--worklimit < 0)
			break;
	} while (true);

	if (wakeup_reader)
	{

/*		for (i = 0, j = 0; i < RX_BUFFERS; i++)
		{
			if (!(device->rx_desc[i].rx_status & DescOwn))
			{
				j++;
			}
		}
		
		device->cur_rx = j;
*/
		input_unwait(device, 1);
	}

	if (wakeup_writer)
	{
		output_unwait(device, 1);
	}

	return handled;
}

status_t hook_open(const char *name, uint32 flags,  void **cookie)
{
	char             *devName;
	int               i;
	int32             devID;
	int32             mask;
	area_id           cookieID;
	status_t          status;
	uint32            size;
	viarhine_private *device;

	(void)flags; /* get rid of compiler warning */

	// Find Device Name
	for (devID = 0; (devName = pDevNameList[devID]); devID++)
	{
		if (strcmp(name, devName) == 0)
			break;
	}

	if (!devName)
		return EINVAL;

	// Check if the Device is Busy and Set in-use Flag if Not
	mask = 1 << devID;
	if (atomic_or(&nOpenMask, mask) & mask)
		return B_BUSY;

	// Allocate Storage for the Cookie
	size = RNDUP(sizeof(viarhine_private), B_PAGE_SIZE);
	cookieID = create_area( DevName " cookie",
							cookie,
							B_ANY_KERNEL_ADDRESS,
							size,
							B_FULL_LOCK,
							B_READ_AREA | B_WRITE_AREA);

	if (cookieID < 0)
	{
		status = B_NO_MEMORY;
		goto Err0;
	}

	device = (viarhine_private*)(*cookie);
	memset(device, 0, sizeof(viarhine_private));

	// Setup the Cookie
	device->cookieID = cookieID;
	device->devID    = devID;
	device->pciInfo  = pDevList[devID];
	device->addr     = device->pciInfo->u.h0.base_registers[0];

	if ((status = res_allocate(device)) != B_OK)
		goto Err1;

	cmd_reset(device);
	snooze(2000);
	cmd_getmacaddr(device);

	// Initialize MII Chip
	if (cap_tbl[device->devID].flags & CanHaveMII)
	{
		int phy,
		    phy_idx = 0;
		
		device->mii_phys[0] = 1;  // Standard for this chip.
		for (phy = 1; (phy < 32) && (phy_idx < 4); phy++)
		{
			int mii_status = mdio_read(device, phy, 1);
			
			if ((mii_status != 0xffff) && (mii_status != 0x0000))
			{
				device->mii_phys[phy_idx++] = phy;
				device->mii_advertising     = mdio_read(device, phy, 4);
				
#ifdef __VIARHINE_DEBUG__
				debug_printf("mii status=%lx, advertising=%lx\n", mii_status, device->mii_advertising);
#endif
			}
		}
		
		device->mii_cnt = phy_idx;
	}

	// Allocate and Initialize Frame Buffer Rings & Descriptors
	if (ring_init(device) != B_OK)
		goto Err2;

	for (i = 0; i < 6; i++)
		write8(device->addr + StationAddr + i, device->macaddr.ebyte[i]);

	write32(device->addr + TxRingPtr, mem_virt2phys(&device->tx_desc[0], sizeof(device->tx_desc[0])));
	write32(device->addr + RxRingPtr, mem_virt2phys(&device->rx_desc[0], sizeof(device->rx_desc[0])));

	// Initialize Other Registers.
	write16(device->addr + PCIBusConfig, 0x0006);

	// Configure the FIFO Thresholds.
	device->tx_thresh = 0x20;
	device->rx_thresh = 0x60;  /* Written in device_rx_setmode(). */

	write8(device->addr + TxConfig, 0x20);
	rx_setmode(device);

	// Setup Interrupt Handler
	install_io_interrupt_handler(device->pciInfo->u.h0.interrupt_line, hook_interrupt, *cookie, 0);

	// Enable interrupts by setting the interrupt mask.
	write16(device->addr + IntrEnable,
			IntrRxDone | IntrRxErr    | IntrRxEmpty    | IntrRxOverflow | 
		    IntrTxDone | IntrTxAbort  | IntrTxUnderrun | IntrRxDropped  |
		    IntrPCIErr | IntrStatsMax | IntrLinkChange | IntrMIIChange);

	device->cmd = CmdStart | CmdTxOn | CmdRxOn | /*CmdNoTxPoll |*/ CmdRxDemand;

	if (device->duplex_lock)
		device->cmd |= CmdFDuplex;

	write16(device->addr + ChipCmd, device->cmd);

	duplex_check(device);

#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_open success!\n");
#endif
	return B_NO_ERROR;

Err2:
	ring_free(device);	

Err1:
	res_free(device);
	delete_area(device->cookieID);

Err0:
	atomic_and(&nOpenMask, ~mask);
#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_open failed!\n");
#endif
	return status;
}

status_t hook_close(void *_data)
{
	viarhine_private *device = (viarhine_private*)_data;

	device->interrupted = 1;
	input_unwait(device, 1);
	output_unwait(device, 1);

	res_free(device);

#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_close\n");
#endif
	return B_NO_ERROR;
}

status_t hook_free(void *_data)
{
	viarhine_private *device = (viarhine_private*)_data;

    // Remove Interrupt Handler
	remove_io_interrupt_handler(device->pciInfo->u.h0.interrupt_line, hook_interrupt, _data);
	write16(device->addr + ChipCmd, CmdStop);

	ring_free(device);
	
	// Device is Now Available Again
	atomic_and(&nOpenMask, ~(1 << device->devID));

	delete_area(device->cookieID);

#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_free\n");
#endif
	return B_NO_ERROR;
}

status_t hook_ioctl(void *_data, uint32 msg, void *buf, size_t len)
{
	viarhine_private *device = (viarhine_private*)_data;

	(void)len; /* get rid of compiler warning */

	switch (msg)
	{
	case ETHER_GETADDR:
		if (_data == NULL)
			return B_ERROR;

		memcpy(buf, &device->macaddr, 6);
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: getaddr\n");
#endif
		return B_NO_ERROR;

	case ETHER_INIT:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: init\n");
#endif
		return B_NO_ERROR;

	case ETHER_GETFRAMESIZE:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: getframesize (%u)\n", MAX_FRAME_SIZE);
#endif
		*(uint32*)buf = MAX_FRAME_SIZE;
		return B_NO_ERROR;

	case ETHER_SETPROMISC:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: setpromiscuous\n");
#endif
		break;

	case ETHER_NONBLOCK:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: nonblock\n");
#endif

		if (device == NULL)
			return B_ERROR;

		memcpy(&device->nonblocking, buf, sizeof(device->nonblocking));
		return B_NO_ERROR;

	case ETHER_ADDMULTI:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: add multicast\n");
#endif
		break;

	default:
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_control: unknown msg %x\n", msg);
#endif
		break;
	}
	
	return B_ERROR;
}

status_t hook_read(void *_data, off_t pos, void *buf, size_t *len)
{
	viarhine_private *device = (viarhine_private*)_data;
	ulong             buflen;
	unsigned int      entry;
	char             *addr;
	uint32            data_size;
	uint32            desc_status;

	(void)pos; /* get rid of compiler warning */
	
	buflen = *len;

	//write16(device->addr + ChipCmd, CmdRxDemand | read16(device->addr + ChipCmd));
	input_wait(device);
		
	if (device->interrupted)
	{
		*len = 0;
		return B_INTERRUPTED;
	}

	if (device->nonblocking)
	{
		*len = 0;
		return 0;
	}
	
	entry = device->cur_rx;
	while (device->rx_desc[entry].rx_status & DescOwn)
	{
		entry++;
		entry %= RX_BUFFERS;
	}

	desc_status = device->rx_desc[entry].rx_status;
	data_size   = (desc_status >> 16) - 4;
	
	if (buflen < data_size)
	{
		data_size = buflen;
	}
	
	addr = &device->rx_buf[entry * BUFFER_SIZE];

#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_read: received (%u)(%u) (%02x:%02x:%02x:%02x:%02x:%02x)\n",
			data_size,
			entry,
			(unsigned char)addr[0],
			(unsigned char)addr[1],
			(unsigned char)addr[2],
			(unsigned char)addr[3],
			(unsigned char)addr[4],
			(unsigned char)addr[5]);
#endif

	*len = data_size;
	memcpy(buf, addr, data_size);

	device->rx_desc[entry].desc_length = MAX_FRAME_SIZE;
	device->rx_desc[entry].rx_status   = DescOwn;

	entry++;
	entry %= RX_BUFFERS;
	device->cur_rx = entry;

	return B_OK;
}

status_t hook_write(void *_data, off_t pos, const void *buf, size_t *len)
{
	viarhine_private *device = (viarhine_private*)_data;
	ulong             buflen;
	status_t          status;
	unsigned int      entry;
	void             *addr;

	(void)pos; /* get rid of compiler warning */
	
	if (device->interrupted)
		return B_INTERRUPTED;

	status = output_wait(device, ETHER_TRANSMIT_TIMEOUT);
	if (status != B_OK) {
#ifdef __VIARHINE_DEBUG__
		debug_printf("hook_write: leave (%s)\n", strerror(status));
#endif
		return status;
	}

	entry = device->cur_tx;
	while(device->tx_desc[entry].tx_status & DescOwn)
	{
		entry++;
		entry %= RX_BUFFERS;
	}

	buflen = *len;

	device->tx_desc[entry].tx_status = DescOwn;
	addr = &device->tx_buf[entry * BUFFER_SIZE];
	memcpy(addr, buf, buflen);

	if (buflen < 60)
		buflen = 60;

	if (buflen & 3) {
		buflen &= (~3);
		buflen += 4;
	}
	
	device->tx_desc[entry].desc_length = 0x00E08000 | buflen;
	entry++;
	entry %= TX_BUFFERS;
	device->cur_tx = entry;
	//write16(device->addr + ChipCmd, CmdTxDemand | read16(device->addr + ChipCmd));

#ifdef __VIARHINE_DEBUG__
	debug_printf("hook_write: sent (%u)(%u)\n", buflen, entry);
#endif

	*len = buflen;
	return B_OK;
}

device_hooks pDeviceHooks = 
{
	hook_open,   // open     entry point
	hook_close,  // close    entry point
	hook_free,   // free     entry point
	hook_ioctl,  // ioctl    entry point
	hook_read,   // read     entry point
	hook_write,  // write    entry point
	NULL,        // select   entry point
	NULL,        // deselect entry point
	NULL,        // readv    entry point
	NULL         // writev   entry point
};
