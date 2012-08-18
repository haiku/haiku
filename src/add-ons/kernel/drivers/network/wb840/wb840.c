/* Copyright (c) 2003-2011
 * Stefano Ceccherini <stefano.ceccherini@gmail.com>. All rights reserved.
 * This file is released under the MIT license
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
#define PHY_ID0_DAVICOM_DM9101	0x0181
#define PHY_ID1_DAVICOM_DM9101	0xb800
#define	MII_HOME	0x0001
#define MII_LAN		0x0002

struct mii_chip_info
{
	const char* name;
	uint16 id0;
	uint16 id1;
	uint8  types;
};


const static struct mii_chip_info
gMIIChips[] = {
	{"DAVICOM_DM9101", PHY_ID0_DAVICOM_DM9101, PHY_ID1_DAVICOM_DM9101, MII_LAN},
	{NULL, 0, 0, 0}
};


static int
mii_readstatus(wb_device* device)
{
	int i = 0;
	int status;

	// status bit has to be retrieved 2 times
	while (i++ < 2)
		status = wb_miibus_readreg(device, device->phy, MII_STATUS);

	return status;
}


static phys_addr_t
physicalAddress(volatile void* addr, uint32 length)
{
	physical_entry table;

	get_memory_map((void*)addr, length, &table, 1);

	return table.address;
}


// Prepares a RX descriptor to be used by the chip
void
wb_put_rx_descriptor(volatile wb_desc* descriptor)
{
	descriptor->wb_status = WB_RXSTAT_OWN;
	descriptor->wb_ctl |= WB_MAX_FRAMELEN | WB_RXCTL_RLINK;
}


void
wb_enable_interrupts(wb_device* device)
{
	write32(device->reg_base + WB_IMR, WB_INTRS);
	write32(device->reg_base + WB_ISR, 0xFFFFFFFF);
}


void
wb_disable_interrupts(wb_device* device)
{
	write32(device->reg_base + WB_IMR, 0L);
	write32(device->reg_base + WB_ISR, 0L);
}


static void
wb_selectPHY(wb_device* device)
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
wb_initPHYs(wb_device* device)
{
	uint16 phy;
	// search for total of 32 possible MII PHY addresses
	for (phy = 0; phy < 32; phy++) {
		struct mii_phy* mii;
		uint16 status;
		int i = 0;

		status = wb_miibus_readreg(device, phy, MII_STATUS);
		status = wb_miibus_readreg(device, phy, MII_STATUS);

		if (status == 0xffff || status == 0x0000)
			// this MII is not accessable
			continue;

		mii = (struct mii_phy*)calloc(1, sizeof(struct mii_phy));
		if (mii == NULL)
			return B_NO_MEMORY;

		mii->address = phy;
		mii->id0 = wb_miibus_readreg(device, phy, MII_PHY_ID0);
		mii->id1 = wb_miibus_readreg(device, phy, MII_PHY_ID1);
		mii->types = MII_HOME;
		mii->next = device->firstPHY;
		device->firstPHY = mii;

		while (gMIIChips[i].name != NULL) {
			if (gMIIChips[i].id0 == mii->id0
				&& gMIIChips[i].id1 == (mii->id1 & 0xfff0)) {
				dprintf("Found MII PHY: %s\n", gMIIChips[i].name);
				mii->types = gMIIChips[i].types;
				break;
			}
			i++;
		}
		if (gMIIChips[i].name == NULL) {
			dprintf("Unknown MII PHY transceiver: id = (%x, %x).\n",
				mii->id0, mii->id1);
		}
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
wb_init(wb_device* device)
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

	write32(device->reg_base + WB_BUSCTL,
		WB_BUSCTL_MUSTBEONE | WB_BUSCTL_ARBITRATION);
	WB_SETBIT(device->reg_base + WB_BUSCTL, WB_BURSTLEN_16LONG);

	write32(device->reg_base + WB_BUSCTL_SKIPLEN, WB_SKIPLEN_4LONG);

	// Disable early TX/RX interrupt, as we can't take advantage
	// from them, at least for now.
	WB_CLRBIT(device->reg_base + WB_NETCFG,
		(WB_NETCFG_TX_EARLY_ON | WB_NETCFG_RX_EARLY_ON));

	wb_set_rx_filter(device);
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
		LOG((DEVICE_NAME": reset hasn't completed!!!"));

	/* Wait a bit while the chip reorders his toughts */
	snooze(1000);
}


status_t
wb_stop(wb_device* device)
{
	uint32 cfgAddress = (uint32)device->reg_base + WB_NETCFG;
	int32 i = 0;

	if (read32(cfgAddress) & (WB_NETCFG_TX_ON | WB_NETCFG_RX_ON)) {
		WB_CLRBIT(cfgAddress, (WB_NETCFG_TX_ON | WB_NETCFG_RX_ON));

		for (i = 0; i < WB_TIMEOUT; i++) {
			if ((read32(device->reg_base + WB_ISR) & WB_ISR_TX_IDLE) &&
				(read32(device->reg_base + WB_ISR) & WB_ISR_RX_IDLE))
				break;
		}
	}

	if (i < WB_TIMEOUT)
		return B_OK;

	return B_ERROR;
}


static void
wb_updateLink(wb_device* device)
{
	if (!device->autoNegotiationComplete) {
		int32 mode = wb_read_mode(device);
		if (mode)
			wb_set_mode(device, mode);

		return;
	}

	if (device->link) {
		uint16 status = mii_readstatus(device);

		// Check if link lost
		if ((status & MII_STATUS_LINK) == 0)
			device->link = false;
	} else {
		uint16 status;
		wb_selectPHY(device);

		// Check if we have a new link
		status = mii_readstatus(device);
		if (status & MII_STATUS_LINK)
			device->link = true;
	}
}


int32
wb_tick(timer* arg)
{
	wb_device* device = (wb_device*)arg;

	wb_updateLink(device);

	return B_OK;
}


/*
 * Program the rx filter.
 */
void
wb_set_rx_filter(wb_device* device)
{
	// TODO: Basically we just config the filter to accept broadcasts
	// packets. We'll need also to configure it to multicast.
	WB_SETBIT(device->reg_base + WB_NETCFG, WB_NETCFG_RX_BROAD);
}


/***************** Interrupt handling ******************************/
static status_t
wb_rxok(wb_device* device)
{
	uint32 releaseRxSem = 0;
	int16 limit;

	acquire_spinlock(&device->rxSpinlock);

	for (limit = device->rxFree; limit > 0; limit--) {
		if (device->rxDescriptor[device->rxInterruptIndex].wb_status
			& WB_RXSTAT_OWN) {
			break;
		}

		releaseRxSem++;
		device->rxInterruptIndex = (device->rxInterruptIndex + 1)
			& WB_RX_CNT_MASK;
		device->rxFree--;
	}

	// Re-enable receive queue
	write32(device->reg_base + WB_RXSTART, 0xFFFFFFFF);

	release_spinlock(&device->rxSpinlock);

	if (releaseRxSem > 0) {
		release_sem_etc(device->rxSem, releaseRxSem, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return B_HANDLED_INTERRUPT;
}


static status_t
wb_tx_nobuf(wb_device* info)
{
	int16 releaseTxSem = 0;
	int16 limit;
	status_t status;

	acquire_spinlock(&info->txSpinlock);

	for (limit = info->txSent; limit > 0; limit--) {
		status = info->txDescriptor[info->txInterruptIndex].wb_status;

		LOG(("wb_tx_nobuf, status: %lx\n", status));
		if (status & WB_TXSTAT_TXERR) {
			LOG(("TX_STAT_ERR\n"));
			break;
		} else if (status & WB_UNSENT) {
			LOG(("TX_STAT_UNSENT\n"));
			break;
		} else if (status & WB_TXSTAT_OWN) {
			LOG((DEVICE_NAME": Device still owns the descriptor\n"));
			break;
		} else
			info->txDescriptor[info->txInterruptIndex].wb_status = 0;

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
wb_interrupt(void* arg)
{
	wb_device* device = (wb_device*)arg;
	int32 retval = B_UNHANDLED_INTERRUPT;
	uint32 status;

	// TODO: Handle other interrupts

	acquire_spinlock(&device->intLock);

	status = read32(device->reg_base + WB_ISR);

	// Did this card request the interrupt ?
	if (status & WB_INTRS) {
		// Clean all the interrupts bits
		if (status)
			write32(device->reg_base + WB_ISR, status);

		if (status & WB_ISR_ABNORMAL)
			LOG((DEVICE_NAME": *** Abnormal Interrupt received ***\n"));
		else
			LOG((DEVICE_NAME": interrupt received: \n"));

		if (status & WB_ISR_RX_EARLY) {
			LOG(("WB_ISR_RX_EARLY\n"));
		}

		if (status & WB_ISR_RX_NOBUF) {
			LOG(("WB_ISR_RX_NOBUF\n"));
			// Something is screwed
		}

		if (status & WB_ISR_RX_ERR) {
			LOG(("WB_ISR_RX_ERR\n"));
			// TODO: Do something useful
		}

		if (status & WB_ISR_RX_OK) {
			LOG(("WB_ISR_RX_OK\n"));
			retval = wb_rxok(device);
		}

		if (status & WB_ISR_RX_IDLE) {
			LOG(("WB_ISR_RX_IDLE\n"));
			// ???
		}

		if (status & WB_ISR_TX_EARLY) {
			LOG(("WB_ISR_TX_EARLY\n"));

		}

		if (status & WB_ISR_TX_NOBUF) {
			LOG(("WB_ISR_TX_NOBUF\n"));
			retval = wb_tx_nobuf(device);
		}

		if (status & WB_ISR_TX_UNDERRUN) {
			LOG(("WB_ISR_TX_UNDERRUN\n"));
			// TODO: Jack up TX Threshold
		}

		if (status & WB_ISR_TX_IDLE) {
			LOG(("WB_ISR_TX_IDLE\n"));
		}

		if (status & WB_ISR_TX_OK) {
			LOG(("WB_ISR_TX_OK\n"));
			// So what ?
		}

		if (status & WB_ISR_BUS_ERR) {
			LOG(("WB_ISR_BUS_ERROR: %lx\n", (status & WB_ISR_BUSERRTYPE) >> 4));
			//wb_reset(device);
		}

		if (status & WB_ISR_TIMER_EXPIRED) {
			LOG(("WB_ISR_TIMER_EXPIRED\n"));
			// ??
		}
	}

	release_spinlock(&device->intLock);

	return retval;
}


/*
 * Print an ethernet address
 */
void
print_address(ether_address_t* addr)
{
	int i;
	char buf[3 * 6 + 1];

	for (i = 0; i < 5; i++) {
		sprintf(&buf[3 * i], "%02x:", addr->ebyte[i]);
	}
	sprintf(&buf[3 * 5], "%02x", addr->ebyte[5]);
	dprintf("%s\n", buf);
}


status_t
wb_create_semaphores(wb_device* device)
{
	device->rxSem = create_sem(0, "wb840 receive");
	if (device->rxSem < B_OK) {
		LOG(("Couldn't create sem, sem_id %ld\n", device->rxSem));
		return device->rxSem;
	}

	device->txSem = create_sem(WB_TX_LIST_CNT, "wb840 transmit");
	if (device->txSem < B_OK) {
		LOG(("Couldn't create sem, sem_id %ld\n", device->txSem));
		delete_sem(device->rxSem);
		return device->txSem;
	}

	set_sem_owner(device->rxSem, B_SYSTEM_TEAM);
	set_sem_owner(device->txSem, B_SYSTEM_TEAM);

	device->rxLock = 0;
	device->txLock = 0;

	return B_OK;
}


void
wb_delete_semaphores(wb_device* device)
{
	if (device->rxSem >= 0)
		delete_sem(device->rxSem);
	if (device->txSem >= 0)
		delete_sem(device->txSem);
}


status_t
wb_create_rings(wb_device* device)
{
	int i;

	device->rxArea = create_area("wb840 rx buffer",
		(void**)&device->rxBuffer[0], B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_RX_LIST_CNT),
		B_32_BIT_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (device->rxArea < B_OK)
		return device->rxArea;

	for (i = 1; i < WB_RX_LIST_CNT; i++) {
		device->rxBuffer[i] = (void*)(((addr_t)device->rxBuffer[0])
			+ (i * WB_BUFBYTES));
	}

	for (i = 0; i < WB_RX_LIST_CNT; i++) {
		device->rxDescriptor[i].wb_status = 0;
		device->rxDescriptor[i].wb_ctl = WB_RXCTL_RLINK;
		wb_put_rx_descriptor(&device->rxDescriptor[i]);
		device->rxDescriptor[i].wb_data = physicalAddress(
			device->rxBuffer[i], WB_BUFBYTES);
		device->rxDescriptor[i].wb_next = physicalAddress(
			&device->rxDescriptor[(i + 1) & WB_RX_CNT_MASK],
			sizeof(struct wb_desc));
	}

	device->rxFree = WB_RX_LIST_CNT;

	device->txArea = create_area("wb840 tx buffer",
		(void**)&device->txBuffer[0], B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(WB_BUFBYTES * WB_TX_LIST_CNT),
		B_32_BIT_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (device->txArea < B_OK) {
		delete_area(device->rxArea);
		return device->txArea;
	}

	for (i = 1; i < WB_TX_LIST_CNT; i++) {
		device->txBuffer[i] = (void*)(((addr_t)device->txBuffer[0])
			+ (i * WB_BUFBYTES));
	}

	for (i = 0; i < WB_TX_LIST_CNT; i++) {
		device->txDescriptor[i].wb_status = 0;
		device->txDescriptor[i].wb_ctl = WB_TXCTL_TLINK;
		device->txDescriptor[i].wb_data = physicalAddress(
			device->txBuffer[i], WB_BUFBYTES);
		device->txDescriptor[i].wb_next = physicalAddress(
			&device->txDescriptor[(i + 1) & WB_TX_CNT_MASK],
			sizeof(struct wb_desc));
	}

	if (wb_stop(device) == B_OK) {
		write32(device->reg_base + WB_RXADDR,
			physicalAddress(&device->rxDescriptor[0], sizeof(struct wb_desc)));
		write32(device->reg_base + WB_TXADDR,
			physicalAddress(&device->txDescriptor[0], sizeof(struct wb_desc)));
	}

	return B_OK;
}


void
wb_delete_rings(wb_device* device)
{
	delete_area(device->rxArea);
	delete_area(device->txArea);
}


int32
wb_read_mode(wb_device* info)
{
	uint16 autoAdv;
	uint16 autoLinkPartner;
	int32 speed;
	int32 duplex;

	uint16 status = mii_readstatus(info);
	if (!(status & MII_STATUS_LINK)) {
		LOG((DEVICE_NAME ": no link detected (status = %x)\n", status));
		return 0;
	}

	// auto negotiation completed
	autoAdv = wb_miibus_readreg(info, info->phy, MII_AUTONEG_ADV);
	autoLinkPartner = wb_miibus_readreg(info, info->phy,
		MII_AUTONEG_LINK_PARTNER);
	status = autoAdv & autoLinkPartner;

	speed = status & (MII_NWAY_TX | MII_NWAY_TX_FDX)
		? LINK_SPEED_100_MBIT : LINK_SPEED_10_MBIT;
	duplex = status & (MII_NWAY_TX_FDX | MII_NWAY_T_FDX)
		? LINK_FULL_DUPLEX : LINK_HALF_DUPLEX;

	info->autoNegotiationComplete = true;

	LOG((DEVICE_NAME ": linked, 10%s MBit, %s duplex\n",
				speed == LINK_SPEED_100_MBIT ? "0" : "",
				duplex == LINK_FULL_DUPLEX ? "full" : "half"));

	return speed | duplex;
}


void
wb_set_mode(wb_device* info, int mode)
{
	uint32 cfgAddress = (uint32)info->reg_base + WB_NETCFG;

	uint32 configFlags = 0;
	status_t status;

	info->speed = mode & LINK_SPEED_MASK;
	info->full_duplex = mode & LINK_DUPLEX_MASK;

	status = wb_stop(info);

	if ((mode & LINK_DUPLEX_MASK) == LINK_FULL_DUPLEX)
		configFlags |= WB_NETCFG_FULLDUPLEX;

	if (info->speed == LINK_SPEED_100_MBIT)
		configFlags |= WB_NETCFG_100MBPS;

	write32(cfgAddress, configFlags);

	if (status == B_OK)
		WB_SETBIT(cfgAddress, WB_NETCFG_TX_ON|WB_NETCFG_RX_ON);
}
