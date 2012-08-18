/* SiS 900 chip specific functions
 *
 * Copyright © 2001-2005 pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */


#include <OS.h>
#include <KernelExport.h>
#include <Drivers.h>
#include <PCI.h>
#include <SupportDefs.h>

#include <stdlib.h>
#include <string.h>

#include "ether_driver.h"
#include "driver.h"
#include "device.h"
#include "interface.h"
#include "sis900.h"


// prototypes

uint16 sis900_resetPHY(struct sis_info *info);
void sis900_selectPHY(struct sis_info *info);
void sis900_setMode(struct sis_info *info, int32 mode);
int32 sis900_readMode(struct sis_info *info);


// MII chip info table

#define PHY_ID0_SiS900_INTERNAL	0x001d
#define PHY_ID1_SiS900_INTERNAL	0x8000
#define PHY_ID0_ICS_1893		0x0015
#define PHY_ID1_ICS_1893		0xff40
#define PHY_ID0_REALTEK_8201	0x0000
#define PHY_ID1_REALTEK_8201	0x8200
#define PHY_ID0_VIA_6103		0x0101
#define PHY_ID1_VIA_6103		0x8f20

#define	MII_HOME	0x0001
#define MII_LAN		0x0002

const static struct mii_chip_info {
	const char	*name;
	uint16		id0, id1;
	uint8		types;
} gMIIChips[] = {
	{"SiS 900 Internal MII PHY",
		PHY_ID0_SiS900_INTERNAL, PHY_ID1_SiS900_INTERNAL, MII_LAN},
	{"SiS 7014 Physical Layer Solution",
		0x0016, 0xf830, MII_LAN},
	{"AMD 79C901 10BASE-T PHY",
		0x0000, 0x6B70, MII_LAN},
	{"AMD 79C901 HomePNA PHY",
		0x0000, 0x6B90, MII_HOME},
	{"ICS 1893 LAN PHY",
		PHY_ID0_ICS_1893, PHY_ID1_ICS_1893, MII_LAN},
	{"NS 83851 PHY",
		0x2000, 0x5C20, MII_LAN | MII_HOME},
	{"Realtek RTL8201 PHY",
		PHY_ID0_REALTEK_8201, PHY_ID1_REALTEK_8201, MII_LAN},
	{"VIA 6103 PHY",
		PHY_ID0_VIA_6103, PHY_ID1_VIA_6103, MII_LAN},
	{NULL, 0, 0, 0}
};


/***************************** helper functions *****************************/


static phys_addr_t
physicalAddress(volatile void *address, uint32 length)
{
	physical_entry table;

	get_memory_map((void *)address, length, &table, 1);
	return table.address;
}


/***************************** interrupt handling *****************************/
// #pragma mark -
int32 intrCounter = 0;
int32 lastIntr[100];


static int32
sis900_rxInterrupt(struct sis_info *info)
{
	int32 handled = B_UNHANDLED_INTERRUPT;
	int16 releaseRxSem = 0;
	int16 limit;

	acquire_spinlock(&info->rxSpinlock);

	HACK(spin(10000));

	// check for packet ownership
	for (limit = info->rxFree; limit > 0; limit--) {
		if (!(info->rxDescriptor[info->rxInterruptIndex].status & SiS900_DESCR_OWN)) {
//			if (limit == info->rxFree)
//			{
				//dprintf("here!\n");
//				limit++;
//				continue;
//			}
			break;
		}
		//dprintf("received frame %d!\n",info->rxInterruptIndex);

		releaseRxSem++;
		info->rxInterruptIndex = (info->rxInterruptIndex + 1) & NUM_Rx_MASK;
		info->rxFree--;
	}
	release_spinlock(&info->rxSpinlock);

	// reenable rx queue
	write32(info->registers + SiS900_MAC_COMMAND, SiS900_MAC_CMD_Rx_ENABLE);

	if (releaseRxSem) {
		release_sem_etc(info->rxSem, releaseRxSem, B_DO_NOT_RESCHEDULE);
		return B_INVOKE_SCHEDULER;
	}

	return handled;
}


static int32
sis900_txInterrupt(struct sis_info *info)
{
	int16 releaseTxSem = 0;
	uint32 status;
	int16 limit;

	acquire_spinlock(&info->txSpinlock);

	HACK(spin(10000));

	for (limit = info->txSent; limit > 0; limit--) {
		status = info->txDescriptor[info->txInterruptIndex].status;

//dprintf("txIntr: %d: mem = %lx : hardware = %lx\n",info->txInterruptIndex,
//		physicalAddress(&info->txDescriptor[info->txInterruptIndex],sizeof(struct buffer_desc)),
//		read32(info->registers + SiS900_MAC_Tx_DESCR));

		/* Does the device generate extra interrupts? */
		if (status & SiS900_DESCR_OWN) {
			uint32 descriptor = read32(info->registers + SiS900_MAC_Tx_DESCR);
			int16 that;
			for (that = 0;
				that < NUM_Tx_DESCR
					&& physicalAddress(&info->txDescriptor[that],
						sizeof(struct buffer_desc)) != descriptor;
				that++) {
			}
			if (that == NUM_Tx_DESCR)
				that = 0;

//dprintf("tx busy %d: %lx (hardware status %d = %lx)!\n",info->txInterruptIndex,status,that,info->txDescriptor[that].status);
//			if (limit == info->txSent)
//			{
//dprintf("oh no!\n");
//				limit++;
//				continue;
//			}
			break;
		}

		if (status & (SiS900_DESCR_Tx_ABORT | SiS900_DESCR_Tx_UNDERRUN
				| SiS900_DESCR_Tx_OOW_COLLISION)) {
			dprintf("tx error: %" B_PRIx32 "\n", status);
		} else
			info->txDescriptor[info->txInterruptIndex].status = 0;

		releaseTxSem++;	/* this many buffers are free */
		info->txInterruptIndex = (info->txInterruptIndex + 1) & NUM_Tx_MASK;
		info->txSent--;

		if (info->txSent < 0 || info->txSent > NUM_Tx_DESCR)
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
sis900_interrupt(void *data)
{
	struct sis_info *info = data;
	int32 handled = B_UNHANDLED_INTERRUPT;
	int16 worklimit = 20;
	uint32 intr;
	cpu_status former;

	former = disable_interrupts();
	acquire_spinlock(&info->lock);

	while (worklimit-- > 0) {
		// reading the interrupt status register clears all interrupts
		intr = read32(info->registers + SiS900_MAC_INTR_STATUS);
		if (!intr)
			break;

		TRACE(("************ interrupt received: %08lx **************\n", intr));
		intrCounter = (intrCounter + 1) % 100;
		lastIntr[intrCounter] = intr;

		// wake-up event interrupt
		if (intr & SiS900_INTR_WAKEUP_EVENT) {
			TRACE(("wake-up event received: %ld\n",
				read32(info->registers + SiS900_MAC_WAKEUP_EVENT)));

			// clear PM event register
			write32(info->registers + SiS900_MAC_WAKEUP_EVENT,
				SiS900_WAKEUP_LINK_ON | SiS900_WAKEUP_LINK_LOSS);
			handled = B_HANDLED_INTERRUPT;
		}

		// receive packet interrupts
		if (intr & (/*SiS900_INTR_Rx_STATUS_OVERRUN |*/ SiS900_INTR_Rx_OVERRUN |
					SiS900_INTR_Rx_ERROR | SiS900_INTR_Rx_OK))
			handled = sis900_rxInterrupt(info);

		// transmit packet interrupts
		if (intr & (SiS900_INTR_Tx_UNDERRUN | SiS900_INTR_Tx_ERROR |
					SiS900_INTR_Tx_IDLE | SiS900_INTR_Tx_OK))
			handled = sis900_txInterrupt(info);
	}

	release_spinlock(&info->lock);
	restore_interrupts(former);

	return handled;
}


void
sis900_disableInterrupts(struct sis_info *info)
{
	write32(info->registers + SiS900_MAC_INTR_MASK, 0);
	write32(info->registers + SiS900_MAC_INTR_ENABLE, 0);
}


void
sis900_enableInterrupts(struct sis_info *info)
{
	write32(info->registers + SiS900_MAC_INTR_ENABLE, 0);

	// enable link detection
	write32(info->registers + SiS900_MAC_WAKEUP_CONTROL,
		SiS900_WAKEUP_LINK_ON | SiS900_WAKEUP_LINK_LOSS);

	// set interrupt mask
	write32(info->registers + SiS900_MAC_INTR_MASK,
		//SiS900_INTR_WAKEUP_EVENT |
		SiS900_INTR_Tx_UNDERRUN | SiS900_INTR_Tx_ERROR | SiS900_INTR_Tx_IDLE | SiS900_INTR_Tx_OK |
		SiS900_INTR_Rx_STATUS_OVERRUN | SiS900_INTR_Rx_OVERRUN |
		SiS900_INTR_Rx_ERROR | SiS900_INTR_Rx_OK);

	write32(info->registers + SiS900_MAC_INTR_ENABLE,1);
}


/***************************** PHY and link mode *****************************/
// #pragma mark -


int32
sis900_timer(timer *t)
{
	struct sis_info *info = (struct sis_info *)t;

	if (!info->autoNegotiationComplete) {
		int32 mode = sis900_readMode(info);
		if (mode)
			sis900_setMode(info, mode);

		return 0;
	}

	if (info->link)	{	// link lost
		uint16 status = mdio_status(info);

		if ((status & MII_STATUS_LINK) == 0) {
			info->link = false;
			dprintf(DEVICE_NAME ": link lost\n");

			// if it's the internal SiS900 MII PHY, reset it
			if (info->currentPHY->id0 == PHY_ID0_SiS900_INTERNAL
				&& (info->currentPHY->id1 & 0xfff0) == PHY_ID1_SiS900_INTERNAL)
				sis900_resetPHY(info);
		}
	}
	if (!info->link) {	// new link established
		uint16 status;

		sis900_selectPHY(info);

		status = mdio_status(info);
		if (status & MII_STATUS_LINK) {
			sis900_checkMode(info);
			info->link = true;
		}
	}

//	revision = info->pciInfo->revision;
//	if (!(revision == SiS900_REVISION_SiS630E || revision == SiS900_REVISION_SiS630EA1
//		|| revision == SiS900_REVISION_SiS630A || revision == SiS900_REVISION_SiS630ET))
//		bug(DEVICE_NAME ": set_eq() not needed!\n");
//	else
//		bug("********* set_eq() would be needed! ********\n");

	return 0;
}


uint16
sis900_resetPHY(struct sis_info *info)
{
	uint16 status = mdio_status(info);

	mdio_write(info, MII_CONTROL, MII_CONTROL_RESET);
	return status;
}


status_t
sis900_initPHYs(struct sis_info *info)
{
	uint16 phy;

	// search for total of 32 possible MII PHY addresses
	for (phy = 0; phy < 32; phy++) {
		struct mii_phy *mii;
		uint16 status;
		int32 i;

		status = mdio_statusFromPHY(info, phy);
		if (status == 0xffff || status == 0x0000)
			// this MII is not accessable
			continue;

		mii = (struct mii_phy *)malloc(sizeof(struct mii_phy));
		if (mii == NULL)
			return B_NO_MEMORY;

		mii->address = phy;
		mii->id0 = mdio_readFromPHY(info, phy, MII_PHY_ID0);
		mii->id1 = mdio_readFromPHY(info, phy, MII_PHY_ID1);
		mii->types = MII_HOME;
		mii->next = info->firstPHY;
		info->firstPHY = mii;

		for (i = 0; gMIIChips[i].name; i++) {
			if (gMIIChips[i].id0 != mii->id0
				|| gMIIChips[i].id1 != (mii->id1 & 0xfff0))
				continue;

			dprintf("Found MII PHY: %s\n", gMIIChips[i].name);

			mii->types = gMIIChips[i].types;
			break;
		}
		if (gMIIChips[i].name == NULL)
			dprintf("Unknown MII PHY transceiver: id = (%x, %x).\n", mii->id0, mii->id1);
	}

	if (info->firstPHY == NULL) {
		dprintf("No MII PHY transceiver found!\n");
		return B_ENTRY_NOT_FOUND;
	}

	sis900_selectPHY(info);

	// if the internal PHY is selected, reset it
	if (info->currentPHY->id0 == PHY_ID0_SiS900_INTERNAL
		&& (info->currentPHY->id1 & 0xfff0) == PHY_ID1_SiS900_INTERNAL) {
		if (sis900_resetPHY(info) & MII_STATUS_LINK) {
			uint16 poll = MII_STATUS_LINK;
			while (poll) {
				poll ^= mdio_read(info, MII_STATUS) & poll;
			}
		}
	}

	// workaround for ICS1893 PHY
	if (info->currentPHY->id0 == PHY_ID0_ICS_1893
		&& (info->currentPHY->id1 & 0xfff0) == PHY_ID1_ICS_1893)
		mdio_write(info, 0x0018, 0xD200);

	// SiS 630E has some bugs on default value of PHY registers
	if (info->pciInfo->revision == SiS900_REVISION_SiS630E) {
		mdio_write(info, MII_AUTONEG_ADV, 0x05e1);
		mdio_write(info, MII_CONFIG1, 0x22);
		mdio_write(info, MII_CONFIG2, 0xff00);
		mdio_write(info, MII_MASK, 0xffc0);
	}

	info->link = mdio_status(info) & MII_STATUS_LINK;

	return B_OK;
}


void
sis900_selectPHY(struct sis_info *info)
{
	uint16 status;

	// ToDo: need to be changed, select PHY in relation to the link mode
	info->currentPHY = info->firstPHY;
	info->phy = info->currentPHY->address;

	status = mdio_read(info, MII_CONTROL);
	status &= ~MII_CONTROL_ISOLATE;

	mdio_write(info, MII_CONTROL, status);
}


void
sis900_setMode(struct sis_info *info, int32 mode)
{
	uint32 address = info->registers + SiS900_MAC_CONFIG;
	uint32 txFlags = SiS900_Tx_AUTO_PADDING | SiS900_Tx_FILL_THRES;
	uint32 rxFlags = 0;

	info->speed = mode & LINK_SPEED_MASK;
	info->full_duplex = (mode & LINK_DUPLEX_MASK) == LINK_FULL_DUPLEX;

	if (read32(address) & SiS900_MAC_CONFIG_EDB_MASTER) {
		TRACE((DEVICE_NAME ": EDB master is set!\n"));
		txFlags |= 5 << SiS900_DMA_SHIFT;
		rxFlags = 5 << SiS900_DMA_SHIFT;
	}

	// link speed FIFO thresholds

	if (info->speed == LINK_SPEED_HOME || info->speed == LINK_SPEED_10_MBIT) {
		rxFlags |= SiS900_Rx_10_MBIT_DRAIN_THRES;
		txFlags |= SiS900_Tx_10_MBIT_DRAIN_THRES;
	} else {
		rxFlags |= SiS900_Rx_100_MBIT_DRAIN_THRES;
		txFlags |= SiS900_Tx_100_MBIT_DRAIN_THRES;
	}

	// duplex mode

	if (info->full_duplex) {
		txFlags |= SiS900_Tx_CS_IGNORE | SiS900_Tx_HB_IGNORE;
		rxFlags |= SiS900_Rx_ACCEPT_Tx_PACKETS;
	}

	write32(info->registers + SiS900_MAC_Tx_CONFIG, txFlags);
	write32(info->registers + SiS900_MAC_Rx_CONFIG, rxFlags);
}


int32
sis900_readMode(struct sis_info *info)
{
	struct mii_phy *phy = info->currentPHY;
	uint16 autoAdv, autoLinkPartner;
	int32 speed, duplex;

	uint16 status = mdio_status(info);
	if (!(status & MII_STATUS_LINK)) {
		dprintf(DEVICE_NAME ": no link detected (status = %x)\n", status);
		return 0;
	}

	// auto negotiation completed
	autoAdv = mdio_read(info, MII_AUTONEG_ADV);
	autoLinkPartner = mdio_read(info, MII_AUTONEG_LINK_PARTNER);
	status = autoAdv & autoLinkPartner;

	speed = status & (MII_NWAY_TX | MII_NWAY_TX_FDX) ? LINK_SPEED_100_MBIT : LINK_SPEED_10_MBIT;
	duplex = status & (MII_NWAY_TX_FDX | MII_NWAY_T_FDX) ? LINK_FULL_DUPLEX : LINK_HALF_DUPLEX;

	info->autoNegotiationComplete = true;

	// workaround for Realtek RTL8201 PHY issue
	if (phy->id0 == PHY_ID0_REALTEK_8201 && (phy->id1 & 0xFFF0) == PHY_ID1_REALTEK_8201) {
		if (mdio_read(info, MII_CONTROL) & MII_CONTROL_FULL_DUPLEX)
			duplex = LINK_FULL_DUPLEX;
		if (mdio_read(info, 0x0019) & 0x01)
			speed = LINK_SPEED_100_MBIT;
	}

	dprintf(DEVICE_NAME ": linked, 10%s MBit, %s duplex\n",
				speed == LINK_SPEED_100_MBIT ? "0" : "",
				duplex == LINK_FULL_DUPLEX ? "full" : "half");

	return speed | duplex;
}


static void
sis900_setAutoNegotiationCapabilities(struct sis_info *info)
{
	uint16 status = mdio_status(info);
	uint16 capabilities = MII_NWAY_CSMA_CD
		| (status & MII_STATUS_CAN_TX_FDX ? MII_NWAY_TX_FDX : 0)
		| (status & MII_STATUS_CAN_TX     ? MII_NWAY_TX : 0)
		| (status & MII_STATUS_CAN_T_FDX  ? MII_NWAY_T_FDX : 0)
		| (status & MII_STATUS_CAN_T      ? MII_NWAY_T : 0);

	TRACE((DEVICE_NAME ": write capabilities %d\n", capabilities));
	mdio_write(info, MII_AUTONEG_ADV, capabilities);
}


static void
sis900_autoNegotiate(struct sis_info *info)
{
	uint16 status = mdio_status(info);

	if ((status & MII_STATUS_LINK) == 0)
	{
		TRACE((DEVICE_NAME ": media link off\n"));
		info->autoNegotiationComplete = true;
		return;
	}
	TRACE((DEVICE_NAME ": auto negotiation started...\n"));

	// reset auto negotiation
	mdio_write(info, MII_CONTROL, MII_CONTROL_AUTO | MII_CONTROL_RESET_AUTONEG);
	info->autoNegotiationComplete = false;
}


void
sis900_checkMode(struct sis_info *info)
{
	uint32 address = info->registers + SiS900_MAC_CONFIG;

	if (info->fixedMode != 0) {
		TRACE((DEVICE_NAME ": link mode set via settings\n"));

		// ToDo: what about the excessive deferral timer?

		sis900_setMode(info, info->fixedMode);
		info->autoNegotiationComplete = true;
	} else if (info->currentPHY->types == MII_LAN) {
		TRACE((DEVICE_NAME ": PHY type is LAN\n"));

		// enable excessive deferral timer
		write32(address, ~SiS900_MAC_CONFIG_EXCESSIVE_DEFERRAL & read32(address));

		sis900_setAutoNegotiationCapabilities(info);
		sis900_autoNegotiate(info);
	} else {
		TRACE((DEVICE_NAME ": PHY type is not LAN\n"));

		// disable excessive deferral timer
		write32(address, SiS900_MAC_CONFIG_EXCESSIVE_DEFERRAL | read32(address));

		sis900_setMode(info, LINK_SPEED_HOME | LINK_HALF_DUPLEX);
		info->autoNegotiationComplete = true;
	}
}


/***************************** MACs, rings & config *****************************/
// #pragma mark -


bool
sis900_getMACAddress(struct sis_info *info)
{
	if (info->pciInfo->revision >= SiS900_REVISION_SiS96x) {
		// SiS 962 & 963 are using a different method to access the EEPROM
		// than the standard SiS 630
		addr_t eepromAccess = info->registers + SiS900_MAC_EEPROM_ACCESS;
		uint32 tries = 0;

		write32(eepromAccess, SiS96x_EEPROM_CMD_REQ);

		while (tries < 1000) {
			if (read32(eepromAccess) & SiS96x_EEPROM_CMD_GRANT) {
				int i;

				/* get MAC address from EEPROM */
				for (i = 0; i < 3; i++) {
					uint16 id;

					id = eeprom_read(info, SiS900_EEPROM_MAC_ADDRESS + i);
					info->address.ebyte[i*2 + 1] = id >> 8;
					info->address.ebyte[i*2] = id & 0xff;
				}

				write32(eepromAccess, SiS96x_EEPROM_CMD_DONE);
				return true;
			} else {
				spin(2);
				tries++;
			}
		}
		write32(eepromAccess, SiS96x_EEPROM_CMD_DONE);
		return false;
	} else if (info->pciInfo->revision >= SiS900_REVISION_SiS630E) {
		/* SiS630E and above are using CMOS RAM to store MAC address */
		pci_info isa;
		int32 index;

		for (index = 0; pci->get_nth_pci_info(index, &isa) == B_OK; index++) {
			if (isa.vendor_id == VENDOR_ID_SiS
				&& isa.device_id == DEVICE_ID_SiS_ISA_BRIDGE) {
				uint8 reg,i;
				addr_t registers = isa.u.h0.base_registers[0];

				reg = pci->read_pci_config(isa.bus,isa.device,isa.function,0x48,1);
				pci->write_pci_config(isa.bus,isa.device,isa.function,0x48,1,reg | 0x40);

				for (i = 0; i < 6; i++) {
					write8(registers + 0x70,0x09 + i);
					info->address.ebyte[i] = read8(registers + 0x71);
				}

				pci->write_pci_config(isa.bus,isa.device,isa.function,0x48,1,reg & ~0x40);
				return true;
			}
		}
		return false;
	} else {
		/* SiS630 stores the MAC in an eeprom */
		uint16 signature;
		int i;

		/* check to see if we have sane EEPROM */
		signature = eeprom_read(info,SiS900_EEPROM_SIGNATURE);
		if (signature == 0xffff || signature == 0x0000) {
			dprintf(DEVICE_NAME ": cannot read EEPROM signature\n");
			return false;
		}

		/* get MAC address from EEPROM */
		for (i = 0; i < 3; i++) {
			uint16 id;

			id = eeprom_read(info,SiS900_EEPROM_MAC_ADDRESS + i);
			info->address.ebyte[i*2 + 1] = id >> 8;
			info->address.ebyte[i*2] = id & 0xff;
		}

		return true;
	}
	return false;
}


status_t
sis900_reset(struct sis_info *info)
{
	addr_t address = info->registers + SiS900_MAC_COMMAND;
	int16 tries = 1000;

	TRACE(("sis900 reset\n"));

	write32(address, SiS900_MAC_CMD_RESET);

	write32(info->registers + SiS900_MAC_Rx_FILTER_CONTROL, SiS900_RxF_ENABLE |
			SiS900_RxF_ACCEPT_ALL_BROADCAST | SiS900_RxF_ACCEPT_ALL_MULTICAST);

	// wait until the chip leaves reset state
	while ((read32(address) & SiS900_MAC_CMD_RESET) && tries-- > 0)
		snooze(2);

	write32(info->registers + SiS900_MAC_COMMAND, SiS900_MAC_CMD_Tx_ENABLE);

	return B_OK;
}


void
sis900_setPromiscuous(struct sis_info *info, bool on)
{
	addr_t filterControl = info->registers + SiS900_MAC_Rx_FILTER_CONTROL;
	int32 filter = read32(filterControl);

	// accept all incoming packets (or not)
	if (on) {
		write32(filterControl, SiS900_RxF_ENABLE |
			SiS900_RxF_ACCEPT_ALL_BROADCAST | SiS900_RxF_ACCEPT_ALL_MULTICAST);
	} else
		write32(filterControl, filter | ~SiS900_RxF_ACCEPT_ALL_ADDRESSES);
}


void
sis900_setRxFilter(struct sis_info *info)
{
	addr_t filterControl = info->registers + SiS900_MAC_Rx_FILTER_CONTROL;
	addr_t filterData = info->registers + SiS900_MAC_Rx_FILTER_DATA;
	int32 i;

	// set MAC address as receive filter
	for (i = 0; i < 3; i++) {
		write32(filterControl, i << SiS900_Rx_FILTER_ADDRESS_SHIFT);
		write32(filterData, info->address.ebyte[i*2] | (info->address.ebyte[i*2 + 1] << 8));
	}
	write32(filterControl, SiS900_RxF_ENABLE
		| SiS900_RxF_ACCEPT_ALL_BROADCAST
		| SiS900_RxF_ACCEPT_ALL_MULTICAST /*| SiS900_RxF_ACCEPT_ALL_ADDRESSES*/);
}


void
sis900_deleteRings(struct sis_info *info)
{
	delete_area(info->txArea);
	delete_area(info->rxArea);
}


status_t
sis900_createRings(struct sis_info *info)
{
	uint16 i;

	// create transmit buffer area
	info->txArea = create_area("sis900 tx buffer", (void **)&info->txBuffer[0],
		B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(BUFFER_SIZE * NUM_Tx_DESCR),
		B_32_BIT_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (info->txArea < B_OK)
		return info->txArea;

	// initialize transmit buffer descriptors
	for (i = 1; i < NUM_Tx_DESCR; i++)
		info->txBuffer[i] = (void *)(((addr_t)info->txBuffer[0]) + (i * BUFFER_SIZE));

	for (i = 0; i < NUM_Tx_DESCR; i++) {
		info->txDescriptor[i].status = 0;
		info->txDescriptor[i].buffer = physicalAddress(info->txBuffer[i], BUFFER_SIZE);
		info->txDescriptor[i].link =
#if (NUM_Tx_DESCR == 1)
			0;
#else
			physicalAddress(&info->txDescriptor[(i + 1) & NUM_Tx_MASK], sizeof(struct buffer_desc));
#endif
	}

	// create receive buffer area
	info->rxArea = create_area("sis900 rx buffer", (void **)&info->rxBuffer[0],
		B_ANY_KERNEL_ADDRESS,
		ROUND_TO_PAGE_SIZE(BUFFER_SIZE * NUM_Rx_DESCR),
		B_32_BIT_FULL_LOCK, B_READ_AREA | B_WRITE_AREA);
	if (info->rxArea < B_OK) {
		delete_area(info->txArea);
		return info->rxArea;
	}

	// initialize receive buffer descriptors
	for (i = 1; i < NUM_Rx_DESCR; i++)
		info->rxBuffer[i] = (void *)(((addr_t)info->rxBuffer[0]) + (i * BUFFER_SIZE));

	for (i = 0; i < NUM_Rx_DESCR; i++) {
		info->rxDescriptor[i].status = MAX_FRAME_SIZE;
		info->rxDescriptor[i].buffer = physicalAddress(info->rxBuffer[i], BUFFER_SIZE);
		info->rxDescriptor[i].link = physicalAddress(&info->rxDescriptor[(i + 1) & NUM_Rx_MASK],
			sizeof(struct buffer_desc));
	}
	info->rxFree = NUM_Rx_DESCR;

	// set descriptor pointer registers
	write32(info->registers + SiS900_MAC_Tx_DESCR,
		physicalAddress(&info->txDescriptor[0], sizeof(struct buffer_desc)));
	write32(info->registers + SiS900_MAC_Rx_DESCR,
		physicalAddress(&info->rxDescriptor[0], sizeof(struct buffer_desc)));

	return B_OK;
}

