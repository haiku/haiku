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

// MII chip info table

#define PHY_ID0_DAVICOM_DM9101 	0x0181
#define PHY_ID1_DAVICOM_DM9101	0xb800
#define	MII_HOME	0x0001
#define MII_LAN		0x0002

const static struct mii_chip_info
{
	const char *name;
	uint16 id0, id1;
	uint8  types;
} 
gMIIChips[] = {
	{"DAVICOM_DM9101 MII PHY", PHY_ID0_DAVICOM_DM9101, PHY_ID1_DAVICOM_DM9101, MII_LAN},
	{NULL,0,0,0}
};


static int
mii_readstatus(wb_device *device)
{
	// status bit has to be retrieved 2 times
	int i = 0;
	int status;
	
	while (i++ < 2)
		status = wb_miibus_readreg(device, device->phy, MII_STATUS);
	
	return status;
}


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
wb_put_rx_descriptor(wb_desc *descriptor)
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
wb_selectPHY(wb_device *device)
{
	uint16 status;
	
	// ToDo: need to be changed, select PHY in relation to the link mode
	device->currentPHY = device->firstPHY;
	device->phy = device->currentPHY->address;
	status = wb_miibus_readreg(device, device->phy, MII_CONTROL);
	status &= ~MII_CONTROL_ISOLATE;

	wb_miibus_writereg(device, device->phy, MII_CONTROL, status);
	
	wb_read_mode(device);
}


status_t
wb_initPHYs(wb_device *device)
{
	uint16 phy;
	// search for total of 32 possible MII PHY addresses
	for (phy = 0; phy < 32; phy++) {
		struct mii_phy *mii;
		uint16 status;
		int i = 0;
	
		status = wb_miibus_readreg(device, phy, MII_STATUS);
		status = wb_miibus_readreg(device, phy, MII_STATUS);
		
		if (status == 0xffff || status == 0x0000)
			// this MII is not accessable
			continue;

		mii = (struct mii_phy *)malloc(sizeof(struct mii_phy));
		if (mii == NULL)
			return B_NO_MEMORY;

		mii->address = phy;
		mii->id0 = wb_miibus_readreg(device, phy, MII_PHY_ID0);
		mii->id1 = wb_miibus_readreg(device, phy, MII_PHY_ID1);
		mii->types = MII_HOME;
		mii->next = device->firstPHY;
		device->firstPHY = mii;
	
		while (gMIIChips[i].name != NULL) {
			if (gMIIChips[i].id0 == mii->id0 && gMIIChips[i].id1 == (mii->id1 & 0xfff0)) {				
				dprintf("Found MII PHY: %s\n", gMIIChips[i].name);
				mii->types = gMIIChips[i].types;
				break;
			}
			i++;
		}
		if (gMIIChips[i].name == NULL)
			dprintf("Unknown MII PHY transceiver: id = (%x, %x).\n",mii->id0, mii->id1);
	}

	if (device->firstPHY == NULL) {
		dprintf("No MII PHY transceiver found!\n");
		return B_ENTRY_NOT_FOUND;
	}
	
	wb_selectPHY(device);
	device->link = mii_readstatus(device) & MII_STATUS_LINK;

	return B_OK;
}


void
wb_init(wb_device *device)
{
	LOG((DEVICE_NAME": init()\n"));
	
	wb_reset(device);
	
	device->wb_txthresh = WB_TXTHRESH_INIT;
	
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
	
	write32(device->reg_base + WB_BUSCTL, WB_BUSCTL_MUSTBEONE|WB_BUSCTL_ARBITRATION);
	WB_SETBIT(device->reg_base + WB_BUSCTL, WB_BURSTLEN_16LONG);
	
	write32(device->reg_base + WB_BUSCTL_SKIPLEN, WB_SKIPLEN_4LONG);
	
	// Early TX interrupt
	WB_SETBIT(device->reg_base + WB_NETCFG, (WB_NETCFG_TX_EARLY_ON|WB_NETCFG_RX_EARLY_ON));
	
	// fullduplex
	//WB_SETBIT(device->reg_base + WB_NETCFG, WB_NETCFG_FULLDUPLEX);
	
	//100 MBits
	//WB_SETBIT(device->reg_base + WB_NETCFG, WB_NETCFG_100MBPS);
	
	/* Program the multicast filter, if necessary */
	//wb_setmulti(device);
	
	//Disable promiscuos mode
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_ALLPHYS);
		
	//Disable capture broadcast
	WB_CLRBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_BROAD);
}


void
wb_reset(wb_device *device)
{
	int i = 0;
	
	LOG((DEVICE_NAME": reset()\n"));
	
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
					
	/* Wait a bit while the chip reorders his toughts */
	snooze(1000);
}


void
wb_updateLink(struct wb_device *device)
{
	if (!device->autoNegotiationComplete) {
		int32 mode = wb_read_mode(device);
		if (mode)
			wb_set_mode(device, mode);
		
		return;
	}

	if (device->link) {	// link lost
		uint16 status = mii_readstatus(device);

		if ((status & MII_STATUS_LINK) == 0)
			device->link = false;
	}
	
	if (!device->link) { // new link established
		uint16 status;
		wb_selectPHY(device);

		status = mii_readstatus(device);
		if (status & MII_STATUS_LINK)		
			device->link = true;
	}
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
			break;
		}
		
		releaseRxSem++;
		device->rxInterruptIndex = (device->rxInterruptIndex + 1) & WB_RX_CNT_MASK;
		device->rxFree--;
	}
	
	// Re-enable receive queue
	write32(device->reg_base + WB_RXSTART, 0xFFFFFFFF);
	
	release_spinlock(&device->rxSpinlock);
	
	
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
		status = info->txDescriptor[info->txInterruptIndex].wb_status;

		if (status & WB_TXSTAT_TXERR) {
			LOG(("TX_STAT_ERR\n"));
			break;
		} else if (status & WB_UNSENT) {
			LOG(("TX_STAT_UNSENT\n"));
			break;
		} else {
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
		LOG(("Couldn't create sem, sem_id %ld\n", device->rxSem));
		return device->rxSem;
	}
	
	set_sem_owner(device->rxSem, B_SYSTEM_TEAM);		
	
	device->txSem = create_sem(WB_TX_LIST_CNT, "wb840 transmit");
	if (device->txSem < B_OK) {
		LOG(("Couldn't create sem, sem_id %ld\n", device->txSem));
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
		
	device->rxArea = create_area("wb840 rx buffer", (void **)&device->rxBuffer[0],
			B_ANY_KERNEL_ADDRESS, ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_RX_LIST_CNT),
			B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);	
	if (device->rxArea < B_OK)
		return device->rxArea;
	
	for (i = 1; i < WB_RX_LIST_CNT; i++)
		device->rxBuffer[i] = (void *)(((uint32)device->rxBuffer[0]) + (i * WB_BUFBYTES));

	for (i = 0; i < WB_RX_LIST_CNT; i++) {
		wb_put_rx_descriptor(&device->rxDescriptor[i]);
		device->rxDescriptor[i].wb_data = physicalAddress(device->rxBuffer[i], WB_BUFBYTES);
		device->rxDescriptor[i].wb_next = physicalAddress(&device->rxDescriptor[(i + 1)
											& WB_RX_CNT_MASK], sizeof(struct wb_desc));
	}
	device->rxFree = WB_RX_LIST_CNT;
	device->rxDescriptor[WB_RX_LIST_CNT - 1].wb_ctl |= WB_RXCTL_RLAST;
	
	device->txArea = create_area("wb840 tx buffer", (void **)&device->txBuffer[0],
			B_ANY_KERNEL_ADDRESS, ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_TX_LIST_CNT),
			B_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (device->txArea < B_OK) {
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


int32
wb_read_mode(wb_device *info)
{
	uint16 autoAdv, autoLinkPartner;
	int32 speed, duplex;

	uint16 status = mii_readstatus(info);
	if (!(status & MII_STATUS_LINK)) {
		dprintf(DEVICE_NAME ": no link detected (status = %x)\n", status);
		return 0;
	}

	// auto negotiation completed
	autoAdv = wb_miibus_readreg(info, info->phy, MII_AUTONEG_ADV);
	autoLinkPartner = wb_miibus_readreg(info, info->phy, MII_AUTONEG_LINK_PARTNER);
	status = autoAdv & autoLinkPartner;

	speed = status & (MII_NWAY_TX | MII_NWAY_TX_FDX) ? LINK_SPEED_100_MBIT : LINK_SPEED_10_MBIT;
	duplex = status & (MII_NWAY_TX_FDX | MII_NWAY_T_FDX) ? LINK_FULL_DUPLEX : LINK_HALF_DUPLEX;
	
	info->autoNegotiationComplete = true;

	dprintf(DEVICE_NAME ": linked, 10%s MBit, %s duplex\n",
				speed == LINK_SPEED_100_MBIT ? "0" : "",
				duplex == LINK_FULL_DUPLEX ? "full" : "half");

	return speed | duplex;
}


void
wb_set_mode(wb_device *info, int mode)
{
	uint32 cfgAddress = (uint32)info->reg_base + WB_NETCFG;
	int32 speed = mode & LINK_SPEED_MASK;
	uint32 configFlags = 0;
	
	bool restart = false;
	int32 i = 0;
	
	// Stop TX and RX queue.
	if (read32(cfgAddress) & (WB_NETCFG_TX_ON|WB_NETCFG_RX_ON)) {
		restart = 1;
		WB_CLRBIT(cfgAddress, (WB_NETCFG_TX_ON|WB_NETCFG_RX_ON));

		for (i = 0; i < WB_TIMEOUT; i++) {
			//delay(10)
			if ((read32(info->reg_base + WB_ISR) & WB_ISR_TX_IDLE) &&
				(read32(info->reg_base + WB_ISR) & WB_ISR_RX_IDLE))
				break;
		}

		if (i == WB_TIMEOUT)
			dprintf(DEVICE_NAME" Failed to put RX and TX in idle state\n");			
	}
	
	if ((mode & LINK_DUPLEX_MASK) == LINK_FULL_DUPLEX)
		configFlags |= WB_NETCFG_FULLDUPLEX;			
	
	if (speed == LINK_SPEED_100_MBIT)
		configFlags |= WB_NETCFG_100MBPS;

	write32(cfgAddress, configFlags);
	
	if (restart)
		WB_SETBIT(cfgAddress, WB_NETCFG_TX_ON|WB_NETCFG_RX_ON);	
}
