/* Copyright (c) 2003-2004 
 * Stefano Ceccherini <burton666@libero.it>. All rights reserved.
 */
 
#include "device.h"
#include "driver.h"
#include "debug.h"
#include "ether_driver.h"
#include "interface.h"
#include "wb840.h"

#include <ByteOrder.h>
#include <KernelExport.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ROUND_TO_PAGE_SIZE(x) (((x) + (B_PAGE_SIZE) - 1) & ~((B_PAGE_SIZE) - 1))

static uint32
physicalAddress(volatile void *addr, uint32 length)
{
	physical_entry table;
	
	get_memory_map((void*)addr, length, &table, 1);
	
	return (uint32)table.address;
}


#if DEBUG
static void
dump_registers(wb_device *device)
{
	LOG(("Registers dump:\n"));
	LOG(("WB_BUSCTL: %x\n", (int)read32(device->reg_base + WB_BUSCTL)));
	LOG(("WB_TXSTART: %x\n", (int)read32(device->reg_base + WB_TXSTART)));
	LOG(("WB_RXSTART: %x\n", (int)read32(device->reg_base + WB_RXSTART)));
	LOG(("WB_RXADDR: %x\n", (int)read32(device->reg_base + WB_RXADDR)));
	LOG(("WB_TXADDR: %x\n", (int)read32(device->reg_base + WB_TXADDR)));
	LOG(("WB_ISR: %x\n", (int)read32(device->reg_base + WB_ISR)));
	LOG(("WB_NETCFG: %x\n", (int)read32(device->reg_base + WB_NETCFG)));
	LOG(("WB_IMR: %x\n", (int)read32(device->reg_base + WB_IMR)));
	LOG(("WB_FRAMESDISCARDED: %x\n", (int)read32(device->reg_base + WB_FRAMESDISCARDED)));
	LOG(("WB_SIO: %x\n", (int)read32(device->reg_base + WB_SIO)));
	LOG(("WB_BOOTROMADDR: %x\n", (int)read32(device->reg_base + WB_BOOTROMADDR)));
	LOG(("WB_TIMER: %x\n", (int)read32(device->reg_base + WB_TIMER)));
	LOG(("WB_CURRXCTL: %x\n", (int)read32(device->reg_base + WB_CURRXCTL)));
	LOG(("WB_CURRXBUF: %x\n", (int)read32(device->reg_base + WB_CURRXBUF)));
	LOG(("WB_MAR0: %x\n", (int)read32(device->reg_base + WB_MAR0)));
	LOG(("WB_MAR1: %x\n", (int)read32(device->reg_base + WB_MAR1)));
	LOG(("WB_NODE0: %x\n", (int)read32(device->reg_base + WB_NODE0)));
	LOG(("WB_NODE1: %x\n", (int)read32(device->reg_base + WB_NODE1)));
	LOG(("WB_BOOTROMSIZE: %x\n", (int)read32(device->reg_base + WB_BOOTROMSIZE)));
	LOG(("WB_CURTXCTL: %x\n", (int)read32(device->reg_base + WB_CURTXCTL)));
	LOG(("WB_CURTXBUF: %x\n", (int)read32(device->reg_base + WB_CURTXBUF)));
}
#endif


// Prepares a RX descriptor to be used by the chip
void
wb_free_rx_descriptor(wb_desc *descriptor)
{
	descriptor->wb_status = WB_RXSTAT_OWN;
	descriptor->wb_ctl = WB_MAX_FRAMELEN | WB_RXCTL_RLINK;
}


void	
wb_enable_interrupts(struct wb_device *device)
{
	write32(device->reg_base + WB_IMR, WB_INTRS);
	write32(device->reg_base + WB_ISR, 0xFFFFFFFF);
}	


void
wb_disable_interrupts(struct wb_device *device)
{
	write32(device->reg_base + WB_IMR, 0L);
	write32(device->reg_base + WB_ISR, 0L);
}


void
wb_reset(wb_device *device)
{
	int i = 0;
	cpu_status status;
	
	LOG((DEVICE_NAME": reset()\n"));
	
	status = disable_interrupts();
	
	device->wb_txthresh = WB_TXTHRESH_INIT;
	
	write32(device->reg_base + WB_NETCFG, 0L);
	write32(device->reg_base + WB_BUSCTL, 0L);
	write32(device->reg_base + WB_TXADDR, 0L);
	write32(device->reg_base + WB_RXADDR, 0L);
	
	WB_SETBIT(device->reg_base + WB_BUSCTL, WB_BUSCTL_RESET);
	WB_SETBIT(device->reg_base + WB_BUSCTL, WB_BUSCTL_RESET);
	
	for (i = 0; i < WB_TIMEOUT; i++) {
		if (!(read32(device->reg_base + WB_BUSCTL) & WB_BUSCTL_RESET))
			break;
	}
	
	if (i == WB_TIMEOUT)
		LOG(("reset hasn't completed!!!"));
	
	write32(device->reg_base + WB_BUSCTL, WB_BUSCTL_MUSTBEONE|WB_BUSCTL_ARBITRATION);
	WB_SETBIT(device->reg_base + WB_BUSCTL, WB_BURSTLEN_16LONG);
	
	switch(device->wb_cachesize) {
		case 32:
			WB_SETBIT(device->reg_base + WB_BUSCTL, WB_CACHEALIGN_32LONG);
			break;	
		case 16:
			WB_SETBIT(device->reg_base + WB_BUSCTL, WB_CACHEALIGN_16LONG);
			break;
		case 8:
			WB_SETBIT(device->reg_base + WB_BUSCTL, WB_CACHEALIGN_8LONG);
			break;
		case 0:
		default:
			WB_SETBIT(device->reg_base + WB_BUSCTL, WB_CACHEALIGN_NONE);
			break;
	}
	
	write32(device->reg_base + WB_BUSCTL_SKIPLEN, WB_SKIPLEN_4LONG);
	
	/* Early TX interrupt; doesn't tend to work too well at 100Mbps */
	WB_SETBIT(device->reg_base + WB_NETCFG, (WB_NETCFG_TX_EARLY_ON|WB_NETCFG_RX_EARLY_ON));
	
	// fullduplex
	WB_SETBIT(device->reg_base + WB_NETCFG, WB_NETCFG_FULLDUPLEX);
	
	//100 MBits
	WB_SETBIT(device->reg_base + WB_NETCFG, WB_NETCFG_100MBPS);
	
	/* Program the multicast filter, if necessary */
	//wb_setmulti(device);
	
	//Disable promiscuos mode
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_ALLPHYS);
		
	//Disable capture broadcast
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_BROAD);
				
	restore_interrupts(status);
	
	/* Wait a bit while the chip reorders his toughts */
	snooze(1000);
	
	//XXX Initialize MII interface
}

void
wb_updateLink(struct wb_device *device)
{	
}


int32
wb_tick(timer *arg)
{
	struct wb_device *device = (wb_device*)arg;
		
	wb_updateLink(device);
	
	return B_OK;
}

/***************** Interrupt handling ******************************/
status_t
wb_rxok(struct wb_device *device)
{
	uint32 releaseRxSem = 0;
	int16 limit;
	LOG(("Rx interrupt\n"));
	
	acquire_spinlock(&device->rxSpinlock);
	
	for (limit = device->rxFree; limit > 0; limit--) {
		if (device->rxDescriptor[device->rxInterruptIndex].wb_status & WB_RXSTAT_OWN) {
			//LOG((DEVICE_NAME ": RX_STAT_OWN\n"));
			break;
		}
		
		releaseRxSem++;
		device->rxInterruptIndex = (device->rxInterruptIndex + 1) & WB_RX_CNT_MASK;
		device->rxFree--;
	}
	
	//Re-enable receive queue
	write32(device->reg_base + WB_RXSTART, 0xFFFFFFFF);
	
	release_spinlock(&device->rxSpinlock);
	
	//LOG((DEVICE_NAME": RxCurrent = %d\n", device->rxInterruptIndex));
	
	if (releaseRxSem > 0) {
		//dprintf("releasing read sem %d times\n", releaseRxSem);
		release_sem_etc(device->rxSem, releaseRxSem, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}
	
	return B_HANDLED_INTERRUPT;
}


status_t
wb_tx_nobuf(struct wb_device *info)
{
	int16 releaseTxSem = 0;
	int16 limit;
	status_t status;
	
	acquire_spinlock(&info->txSpinlock);

	for (limit = info->txSent; limit > 0; limit--) {
		status =info->txDescriptor[info->txInterruptIndex].wb_status;

		if (status & WB_TXSTAT_TXERR) {
			LOG(("TX_STAT_ERR\n"));
			break;
		} else if (status & WB_UNSENT) {
			LOG(("TX_STAT_UNSENT\n"));
			break;
		} else {
			//dprintf("releasing TX descriptor to the ring\n");
			info->txDescriptor[info->txInterruptIndex].wb_status = 0;
		}
		releaseTxSem++;	// this many buffers are free
		info->txInterruptIndex = (info->txInterruptIndex + 1) & WB_TX_CNT_MASK;
		info->txSent--;

		if (info->txSent < 0 || info->txSent > WB_TX_LIST_CNT)
			dprintf("ERROR interrupt: txSent = %d\n", info->txSent);
	}
	
	release_spinlock(&info->txSpinlock);

	if (releaseTxSem) {
		release_sem_etc(info->txSem, releaseTxSem, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


int32
wb_interrupt(void *arg)
{
	struct wb_device *device = (wb_device*)arg;
	int32 retval = B_UNHANDLED_INTERRUPT;
	int16 worklimit = 20;
	uint32 status;
		
	acquire_spinlock(&device->intLock);
	wb_disable_interrupts(device);
	
	while (worklimit-- > 0) {		
		status = read32(device->reg_base + WB_ISR);
		
		/* Is our card the one which requested the interrupt ? */
		if (!(status & WB_INTRS)) {
			/* No, bail out. */
			break;
		}
		
		// Clean all the interrupts
		if (status)
			write32(device->reg_base + WB_ISR, status);
				
		LOG((DEVICE_NAME":*** Interrupt received ***\n"));	
					
		if (status & WB_ISR_RX_NOBUF) {
			LOG((DEVICE_NAME ": WB_ISR_RX_NOBUF\n"));	
			//retval = wb_rx_nobuf(device);
			continue;
		}
		
		if (status & WB_ISR_RX_ERR) {
			LOG((DEVICE_NAME": WB_ISR_RX_ERR\n"));
			continue;
		}
		
		if (status & WB_ISR_RX_OK) {
			LOG((DEVICE_NAME": WB_ISR_RX_OK\n"));
			retval = wb_rxok(device);
			continue;
		}
		
		if (status & WB_ISR_RX_IDLE) {
			LOG((DEVICE_NAME": WB_ISR_RX_IDLE\n"));
			continue;
		}
				
		if (status & WB_ISR_TX_NOBUF) {
			LOG((DEVICE_NAME ": WB_ISR_TX_NOBUF\n"));
			retval = wb_tx_nobuf(device);
			continue;
		}
		
		if (status & WB_ISR_TX_UNDERRUN) {
			LOG((DEVICE_NAME ": WB_ISR_TX_UNDERRUN\n"));
			// TODO: Jack up TX Threshold
			continue;
		}
		
		if (status & WB_ISR_TX_IDLE) {
			LOG((DEVICE_NAME ": WB_ISR_TX_IDLE\n"));
			continue;
		}
		
		if (status & WB_ISR_TX_OK) {
			LOG((DEVICE_NAME": WB_ISR_TX_OK\n"));
			continue;
		}
							
		if (status & WB_ISR_BUS_ERR) {
			LOG((DEVICE_NAME": BUS_ERROR: %x\n", (int)(status & WB_ISR_BUSERRTYPE) >> 4));
			//wb_reset(device);
			continue;
		}
		
		LOG((DEVICE_NAME ": unknown interrupt received\n"));
	}
	
	wb_enable_interrupts(device);
	release_spinlock(&device->intLock);
	
	return retval;
}
	

/*
 * Print an ethernet address
 */
void
print_address(ether_address_t *addr)
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3*i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3*5], "%02x", addr->ebyte[5]);
	dprintf("%s\n", buf);
}


status_t
wb_create_semaphores(struct wb_device *device)
{	
	device->rxSem = create_sem(0, "wb840 receive");
	if (device->rxSem < B_OK) {
		LOG(("Couldn't create sem, sem_id %d\n", (int)device->rxSem));
		return device->rxSem;
	}
	set_sem_owner(device->rxSem, B_SYSTEM_TEAM);		
	
	device->txSem = create_sem(WB_TX_LIST_CNT, "wb840 transmit");
	if (device->txSem < B_OK) {
		LOG(("Couldn't create sem, sem_id %d\n", (int)device->txSem));
		delete_sem(device->rxSem);
		return device->txSem;
	}
	set_sem_owner(device->txSem, B_SYSTEM_TEAM);
	
	device->rxLock = 0;
	device->txLock = 0;
	
	return B_OK;
}


status_t
wb_create_rings(struct wb_device *device)
{
	int i;
		
	if ((device->rxArea = create_area("wb840 rx buffer", (void **)&device->rxBuffer[0],
			B_ANY_KERNEL_ADDRESS, ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_RX_LIST_CNT),
			B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA)) < B_OK)
		return device->rxArea;
	
	for (i = 1; i < WB_RX_LIST_CNT; i++)
		device->rxBuffer[i] = (void *)(((uint32)device->rxBuffer[0]) + (i * WB_BUFBYTES));

	for (i = 0; i < WB_RX_LIST_CNT; i++) {
		wb_free_rx_descriptor(&device->rxDescriptor[i]);
		device->rxDescriptor[i].wb_data = physicalAddress(device->rxBuffer[i], WB_BUFBYTES);
		device->rxDescriptor[i].wb_next = physicalAddress(&device->rxDescriptor[(i + 1)
											& WB_RX_CNT_MASK], sizeof(struct wb_desc));
	}
	device->rxFree = WB_RX_LIST_CNT;
	device->rxDescriptor[WB_RX_LIST_CNT - 1].wb_ctl |= WB_RXCTL_RLAST;
	
	if ((device->txArea = create_area("wb840 tx buffer", (void **)&device->txBuffer[0],
			B_ANY_KERNEL_ADDRESS, ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_TX_LIST_CNT),
			B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA)) < B_OK) {
		delete_area(device->rxArea);
		return device->txArea;
	}
	
	for (i = 1; i < WB_TX_LIST_CNT; i++)
		device->txBuffer[i] = (void *)(((uint32)device->txBuffer[0]) + (i * WB_BUFBYTES));

	for (i = 0; i < WB_TX_LIST_CNT; i++) {
		device->txDescriptor[i].wb_status = 0;
		device->txDescriptor[i].wb_ctl = WB_TXCTL_TLINK;
		device->txDescriptor[i].wb_data = physicalAddress(device->txBuffer[i], WB_BUFBYTES);
		device->txDescriptor[i].wb_next = physicalAddress(&device->txDescriptor[(i + 1)
												& WB_TX_CNT_MASK], sizeof(struct wb_desc));
	}
	
	device->txDescriptor[WB_TX_LIST_CNT - 1].wb_ctl |= WB_TXCTL_TLAST;
	
	/* Load the address of the RX list */
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_ON);
	write32(device->reg_base + WB_RXADDR, physicalAddress(&device->rxDescriptor[0],
													sizeof(struct wb_desc)));
	
	/* Load the address of the TX list */
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_TX_ON);
	write32(device->reg_base + WB_TXADDR, physicalAddress(&device->txDescriptor[0],
													sizeof(struct wb_desc)));
		
	return B_OK;
}


void
wb_delete_rings(struct wb_device *device)
{
	delete_area(device->rxArea);
	delete_area(device->txArea);
}
