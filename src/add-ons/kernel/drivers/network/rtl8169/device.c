/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 *
 * Permission to use, copy, modify and distribute this software and its 
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appear in all copies, and that both the
 * copyright notice and this permission notice appear in supporting documentation.
 *
 * Marcus Overhagen makes no representations about the suitability of this software
 * for any purpose. It is provided "as is" without express or implied warranty.
 *
 * MARCUS OVERHAGEN DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
 * ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL MARCUS
 * OVERHAGEN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

//#define DEBUG

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "hardware.h"
#include "util.h"

static int32 gOpenMask = 0;

void
write_phy_reg(rtl8169_device *device, int reg, uint16 value)
{
	int i;
	write32(REG_PHYAR, 0x80000000 | (reg & 0x1f) << 16 | value);
	snooze(1000);
	for (i = 0; i < 2000; i++) {
		if ((read32(REG_PHYAR) & 0x80000000) == 0)
			break;
		snooze(100);
	}
}

uint16
read_phy_reg(rtl8169_device *device, int reg)
{
	uint32 v;
	int i;
	write32(REG_PHYAR, (reg & 0x1f) << 16);
	snooze(1000);
	for (i = 0; i < 2000; i++) {
		v = read32(REG_PHYAR);
		if (v & 0x80000000)
			return v & 0xffff;
		snooze(100);
	}
	return 0xffff;
}

static inline void
write_phy_reg_bit(rtl8169_device *device, int reg, int bitnum, int bitval)
{
	uint16 val = read_phy_reg(device, reg);
	if (bitval == 1)
		val |= (1 << bitnum);
	else
		val &= ~(1 << bitnum);
	write_phy_reg(device, reg, val);
}

void
phy_config(rtl8169_device *device)
{
	TRACE("phy_config()\n");

	if (device->phy_version == 0 || device->phy_version == 1) {
		uint16 val;
		TRACE("performing phy init\n");
		// XXX this should probably not be done if the phy wasn't
		// identified, but BSD does it too for mac_version == 0 (=> phy_version also 0)
		// doing the same as the BSD and Linux driver here
		// see IEE 802.3-2002 (is also uses decimal numbers when refering
		// to the registers, as do we). Added a little documentation
		write_phy_reg(device, 31, 0x0001); // vendor specific (enter programming mode?)
		write_phy_reg(device, 21, 0x1000); // vendor specific	
		write_phy_reg(device, 24, 0x65c7); // vendor specific	
		write_phy_reg_bit(device, 4, 11, 0); // reset T (T=toggle) bit in reg 4 (ability)
		val = read_phy_reg(device, 4) & 0x0fff;	// get only the message code fields
		write_phy_reg(device, 4, val); // and write them back. this clears the page and makes it unformatted (see 37.2.4.3.1)
		write_phy_reg(device, 3, 0x00a1); // assign 32 bit phy identifier high word
		write_phy_reg(device, 2, 0x0008); // assign 32 bit phy identifier low word
		write_phy_reg(device, 1, 0x1020); // set status: 10 MBit full duplex and auto negotiation completed
		write_phy_reg(device, 0, 0x1000); // reset the phy!
		write_phy_reg_bit(device, 4, 11, 1); // set toggle bit high
		write_phy_reg_bit(device, 4, 11, 0); // set toggle bit low => this is a toggle
		val = (read_phy_reg(device, 4) & 0x0fff) | 0x7000; // set ack1, ack2, indicate formatted page
		write_phy_reg(device, 4, val); // write the value from above
		write_phy_reg(device, 3, 0xff41); // assign another
		write_phy_reg(device, 2, 0xde60); // 32 bit phy identifier
		write_phy_reg(device, 1, 0x0140); // phy will accept management frames with preamble suppressed, extended capability in reg 15
		write_phy_reg(device, 0, 0x0077); //
		write_phy_reg_bit(device, 4, 11, 1);	// set toggle bit high
		write_phy_reg_bit(device, 4, 11, 0);	// set toggle bit low => this is a toggle
		val = ( read_phy_reg(device, 4) & 0x0fff) | 0xa000;
		write_phy_reg(device, 4, val);	//
		write_phy_reg(device, 3, 0xdf01); // assign another
		write_phy_reg(device, 2, 0xdf20); // 32 bit phy identifier
		write_phy_reg(device, 1, 0xff95); // phy will do 100Mbit and 10Mbit in full and half duplex, something reserved and
										  // remote fault detected, link is up and extended capability in reg 15
		write_phy_reg(device, 0, 0xfa00); // select 10 MBit, disable auto neg., half duplex normal operation
		write_phy_reg_bit(device, 4, 11, 1);	// set toggle bit high
		write_phy_reg_bit(device, 4, 11, 0);	// set toggle bit low => this is a toggle
		val = ( read_phy_reg(device, 4) & 0x0fff) | 0xb000;
		write_phy_reg(device, 4, val); // write capabilites
		write_phy_reg(device, 3, 0xff41); // assign another
		write_phy_reg(device, 2, 0xde20); // 32 bit phy identifier
		write_phy_reg(device, 1, 0x0140); // phy will accept management frames with preamble suppressed, extended capability in reg 15
		write_phy_reg(device, 0, 0x00bb); // write status
		write_phy_reg_bit(device, 4, 11, 1); // set toggle bit high
		write_phy_reg_bit(device, 4, 11, 0); // set toggle bit low => this is a toggle
		val = ( read_phy_reg(device, 4) & 0x0fff) | 0xf000;
		write_phy_reg(device, 4, val);	//w 4 15 12 f
		write_phy_reg(device, 3, 0xdf01); // assign another
		write_phy_reg(device, 2, 0xdf20); // 32 bit phy identifier
		write_phy_reg(device, 1, 0xff95); // write capabilites
		write_phy_reg(device, 0, 0xbf00); // write status
		write_phy_reg_bit(device, 4, 11, 1); // set toggle bit high
		write_phy_reg_bit(device, 4, 11, 0); // set toggle bit low => this is a toggle
		write_phy_reg(device, 31, 0x0000); // vendor specific (leave programming mode?)
	}	

	write_phy_reg(device, 4, 0x01e1); // 10/100 capability
	write_phy_reg(device, 9, 0x0200); // 1000 capability
	write_phy_reg(device, 0, 0x1200); // enable auto negotiation and restart it
}


void
dump_phy_stat(rtl8169_device *device)
{
	uint32 v = read8(REG_PHY_STAT);
	if (v & PHY_STAT_EnTBI) {
		uint32 tbi = read32(REG_TBICSR);
		TRACE("TBI mode active\n");
		if (tbi & TBICSR_ResetTBI)
			TRACE("TBICSR_ResetTBI\n");
		if (tbi & TBICSR_TBILoopBack)
			TRACE("TBICSR_TBILoopBack\n");
		if (tbi & TBICSR_TBINWEn)
			TRACE("TBICSR_TBINWEn\n");
		if (tbi & TBICSR_TBIReNW)
			TRACE("TBICSR_TBIReNW\n");
		if (tbi & TBICSR_TBILinkOk)
			TRACE("TBICSR_TBILinkOk\n");
		if (tbi & TBICSR_NWComplete)
			TRACE("TBICSR_NWComplete\n");
	} else {
		TRACE("TBI mode NOT active\n");
		if (v & PHY_STAT_1000MF)
			TRACE("PHY_STAT_1000MF\n");
		if (v & PHY_STAT_100M)
			TRACE("PHY_STAT_100M\n");
		if (v & PHY_STAT_10M)
			TRACE("PHY_STAT_10M\n");
	}
	if (v & PHY_STAT_TxFlow)
		TRACE("PHY_STAT_TxFlow\n");
	if (v & PHY_STAT_RxFlow)
		TRACE("PHY_STAT_RxFlow\n");
	if (v & PHY_STAT_LinkSts)
		TRACE("PHY_STAT_LinkSts\n");
	if (v & PHY_STAT_FullDup)
		TRACE("PHY_STAT_FullDup\n");
}
			

status_t
init_buf_desc(rtl8169_device *device)
{
	void *rx_buf_desc_virt, *rx_buf_desc_phy;
	void *tx_buf_desc_virt, *tx_buf_desc_phy;
	void *tx_buf_virt, *tx_buf_phy;
	void *rx_buf_virt, *rx_buf_phy;
	int i;

	device->txBufArea = alloc_mem(&tx_buf_virt, &tx_buf_phy, TX_DESC_COUNT * FRAME_SIZE, 0, "rtl8169 tx buf");
	device->rxBufArea = alloc_mem(&rx_buf_virt, &rx_buf_phy, RX_DESC_COUNT * FRAME_SIZE, 0, "rtl8169 rx buf");
	device->txDescArea = alloc_mem(&tx_buf_desc_virt, &tx_buf_desc_phy, TX_DESC_COUNT * sizeof(buf_desc), 0, "rtl8169 tx desc");
	device->rxDescArea = alloc_mem(&rx_buf_desc_virt, &rx_buf_desc_phy, RX_DESC_COUNT * sizeof(buf_desc), 0, "rtl8169 rx desc");
	if (device->txBufArea < B_OK || device->rxBufArea < B_OK || device->txDescArea < B_OK || device->rxDescArea < B_OK)
		return B_NO_MEMORY;
	
	device->txDesc = (buf_desc *) tx_buf_desc_virt;
	device->rxDesc = (buf_desc *) rx_buf_desc_virt;

	// setup transmit descriptors
	for (i = 0; i < TX_DESC_COUNT; i++) {
		device->txBuf[i] = (char *)tx_buf_virt + (i * FRAME_SIZE);
		device->txDesc[i].stat_len = TX_DESC_FS | TX_DESC_LS;
		device->txDesc[i].buf_low  = (uint32)((char *)tx_buf_phy + (i * FRAME_SIZE));
		device->txDesc[i].buf_high = 0;
	}
	device->txDesc[i - 1].stat_len |= TX_DESC_EOR;

	// setup receive descriptors
	for (i = 0; i < RX_DESC_COUNT; i++) {
		device->rxBuf[i] = (char *)rx_buf_virt + (i * FRAME_SIZE);
		device->rxDesc[i].stat_len = RX_DESC_OWN | FRAME_SIZE;
		device->rxDesc[i].buf_low  = (uint32)((char *)rx_buf_phy + (i * FRAME_SIZE));
		device->rxDesc[i].buf_high = 0;
	}
	device->rxDesc[i - 1].stat_len |= RX_DESC_EOR;
	
	write32(REG_RDSAR_LOW, (uint32)rx_buf_desc_phy);
	write32(REG_RDSAR_HIGH, 0);
	write32(REG_TNPDS_LOW, (uint32)tx_buf_desc_phy);
	write32(REG_TNPDS_HIGH, 0);
	write32(REG_THPDS_LOW, 0);	// high priority tx is unused
	write32(REG_THPDS_HIGH, 0);

	return B_OK;
}

static inline void
rtl8169_tx_int(rtl8169_device *device)
{
	int32 limit;
	int32 count;

	acquire_spinlock(&device->txSpinlock);

	for (count = 0, limit = device->txUsed; limit > 0; limit--) {
		if (device->txDesc[device->txIntIndex].stat_len & TX_DESC_OWN)
			break;
		device->txIntIndex = (device->txIntIndex + 1) % TX_DESC_COUNT;
		count++;
	}

//	dprintf("tx int, txUsed %d, count %d\n", device->txUsed, count);

	device->txUsed -= count;

	release_spinlock(&device->txSpinlock);

	if (count)
		release_sem_etc(device->txFreeSem, count, B_DO_NOT_RESCHEDULE);
}


static inline void
rtl8169_rx_int(rtl8169_device *device)
{
	int32 limit;
	int32 count;
	
	acquire_spinlock(&device->rxSpinlock);

	for (count = 0, limit = device->rxFree; limit > 0; limit--) {
		if (device->rxDesc[device->rxIntIndex].stat_len & RX_DESC_OWN)
			break;
		device->rxIntIndex = (device->rxIntIndex + 1) % RX_DESC_COUNT;
		count++;
	}

//	dprintf("rx int, rxFree %d, count %d\n", device->rxFree, count);
	
	device->rxFree -= count;

	release_spinlock(&device->rxSpinlock);

	if (count)
		release_sem_etc(device->rxReadySem, count, B_DO_NOT_RESCHEDULE);
}


int32
rtl8169_int(void *data)
{
	rtl8169_device *device = (rtl8169_device *)data;
	uint16 stat;
	int32 ret;

	stat = read16(REG_INT_STAT);
	if (stat == 0 || stat == 0xffff)
		return B_UNHANDLED_INTERRUPT;
	
	write16(REG_INT_STAT, stat);
	ret = B_HANDLED_INTERRUPT;

	if (stat & INT_FOVW) {
		TRACE("INT_FOVW\n");
	}

	if (stat & INT_PUN) {
		TRACE("INT_PUN\n");
		dump_phy_stat(device);
	}

	if (stat & (INT_TOK | INT_TER)) {
		rtl8169_tx_int(device);
		ret = B_INVOKE_SCHEDULER;
	}

	if (stat & (INT_ROK | INT_RER)) {
		rtl8169_rx_int(device);
		ret = B_INVOKE_SCHEDULER;
	}

  	return ret;
}


status_t
rtl8169_open(const char *name, uint32 flags, void** cookie)
{
	rtl8169_device *device;
	char *deviceName;
	uint32 val;
	int dev_id;
	int mask;
	int i;
	
	TRACE("rtl8169_open()\n");

	for (dev_id = 0; (deviceName = gDevNameList[dev_id]) != NULL; dev_id++) {
		if (!strcmp(name, deviceName))
			break;
	}		
	if (deviceName == NULL) {
		ERROR("invalid device name");
		return B_ERROR;
	}
	
	// allow only one concurrent access
	mask = 1 << dev_id;
	if (atomic_or(&gOpenMask, mask) & mask)
		return B_BUSY;
		
	*cookie = device = (rtl8169_device *)malloc(sizeof(rtl8169_device));
	if (!device) {
		atomic_and(&gOpenMask, ~(1 << dev_id));
		return B_NO_MEMORY;
	}
	
	memset(device, 0, sizeof(*device));

	device->devId = dev_id;
	device->pciInfo = gDevList[dev_id];
	device->nonblocking = (flags & O_NONBLOCK) ? true : false;
	device->closed = false;
	
	device->rxSpinlock = 0;
	device->rxNextIndex = 0;
	device->rxIntIndex = 0;
	device->rxFree = RX_DESC_COUNT;
	device->rxReadySem = create_sem(0, "rtl8169 rx ready");
	set_sem_owner(device->rxReadySem, B_SYSTEM_TEAM);
	
	device->txSpinlock = 0;
	device->txNextIndex = 0;
	device->txIntIndex = 0;
	device->txUsed = 0;
	device->txFreeSem = create_sem(TX_DESC_COUNT, "rtl8169 tx free");
	set_sem_owner(device->txFreeSem, B_SYSTEM_TEAM);
	
	// enable busmaster and memory mapped access, disable io port access
	val = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_command, 2);
	val = PCI_PCICMD_BME | PCI_PCICMD_MSE | (val & ~PCI_PCICMD_IOS);
	gPci->write_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_command, 2, val);

	// adjust PCI latency timer
	TRACE("changing PCI latency to 0x40\n");
	gPci->write_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_latency, 1, 0x40);

	// get IRQ
	device->irq = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_interrupt_line, 1);
	if (device->irq == 0 || device->irq == 0xff) {
		ERROR("no IRQ assigned\n");
		goto err;
	}

	TRACE("IRQ %d\n", device->irq);
	
	// map registers into memory
	val = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, 0x14, 4);
	val &= PCI_address_memory_32_mask;
	TRACE("hardware register address %p\n", (void *) val);
	device->refArea = map_mem(&device->regAddr, (void *)val, 256, 0, "rtl8169 register");
	if (device->refArea < B_OK) {
		ERROR("can't map hardware registers\n");
		goto err;
	}
		
	TRACE("mapped registers to %p\n", device->regAddr);

	// disable receiver & transmitter XXX might be removed
	write8(REG_CR, read8(REG_CR) & ~(CR_RE | CR_TE));

	// do a soft reset
	write8(REG_CR, read8(REG_CR) | CR_RST);
	for (i = 0; (read8(REG_CR) & CR_RST) && i < 1000; i++)
		snooze(10);
	if (i == 1000) {
		ERROR("hardware reset failed\n");
		goto err;
	}

	TRACE("reset done\n");

	// get MAC hardware version
	device->mac_version = ((read32(REG_TX_CONFIG) & 0x7c000000) >> 25) | ((read32(REG_TX_CONFIG) & 0x00800000) >> 23);
	TRACE("8169 Mac Version %d\n", device->mac_version);
	if (device->mac_version > 0) { // this is a RTL8169s single chip
		// get PHY hardware version
		device->phy_version = read_phy_reg(device, 0x03) & 0x000f;
		TRACE("8169 Phy Version %d\n", device->phy_version);
	} else {
		// we should probably detect what kind of phy is used
		device->phy_version = 0;
		TRACE("8169 Phy Version unknown\n");
	}
	
	if (device->mac_version == 1) {
		// as it's done by the BSD driver...
		TRACE("Setting MAC Reg C+CR 0x82h = 0x01h\n");
		write8(0x82, 0x01); // don't know what this does
		TRACE("Setting PHY Reg 0x0bh = 0x00h\n");
		write_phy_reg(device, 0x0b, 0x0000); // 0xb is a reserved (vendor specific register), don't know what this does
	}
	
	// configure PHY
	phy_config(device);

	// initialize MAC address	
	for (i = 0; i < 6; i++)
		device->macaddr[i] = read8(i);
		
	dprintf("card %p, mac: ", device);
	for (i = 0; i < 6; i++)
		dprintf("%02x:", device->macaddr[i]);
		
	TRACE("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		device->macaddr[0], device->macaddr[1], device->macaddr[2],
		device->macaddr[3], device->macaddr[4], device->macaddr[5]);
	
	// setup interrupt handler
	if (install_io_interrupt_handler(device->irq, rtl8169_int, device, 0) < B_OK) {
		ERROR("can't install interrupt handler\n");
		goto err;
	}
	
	write16(0xe0, read16(0xe0)); // write CR+ command

	write16(0xe0, read16(0xe0) | 0x0003); // don't know what this does, BSD says "enable C+ Tx/Rx"

	if (device->mac_version == 1) {
		TRACE("Setting Reg C+CR bit 3 and bit 14 to 1\n");
		// bit 3 is PCI multiple read/write enable (max Tx/Rx DMA burst size setting is no longer valid then)
		// bit 14 ??? (need more docs)
		write16(0xe0, read16(0xe0) | 0x4008);
	}

	// setup buffer descriptors and buffers
	if (init_buf_desc(device) != B_OK) {
		ERROR("setting up buffer descriptors failed\n");
		goto err;
	}

	// enable receiver & transmitter
	write8(REG_CR, read8(REG_CR) | CR_RE | CR_TE);
	
	write8(REG_9346CR, 0xc0);	// enable config access
	write8(REG_CONFIG1, read8(REG_CONFIG1) & ~1);	// disable power management
	write8(REG_9346CR, 0x00);	// disable config access
	
	write8(0xec, 0x3f); 		// disable early transmit treshold
	write16(0xda, FRAME_SIZE);	// receive packet maximum size

	write16(0x5c, read16(0x5c) & 0xf000); // disable early receive interrupts
	
	write32(0x4c, 0); // RxMissed ???

	// setup receive config, can only be done when receiver is enabled!
	// 1024 byte FIFO treshold, 1024 DMA burst
	write32(REG_RX_CONFIG, (read32(REG_RX_CONFIG) & RX_CONFIG_MASK)
		| (0x6 << RC_CONFIG_RXFTH_Shift) | (0x6 << RC_CONFIG_MAXDMA_Shift)
		| RX_CONFIG_AcceptBroad | RX_CONFIG_AcceptMulti | RX_CONFIG_AcceptMyPhys);

	write32(0x8, 0); // multicast filter
	write32(0xc, 0); // multicast filter

	// setup transmit config, can only be done when transmitter is enabled!	
	// append CRC, 1024 DMA burst
	write32(REG_TX_CONFIG, (read32(REG_TX_CONFIG) & ~(0x10000 | (1 << 8))) | (0x6 << 8));
	
  	// clear pending interrupt status
  	write16(REG_INT_STAT, 0xffff);
  	
  	// enable used interrupts
 	write16(REG_INT_MASK, INT_FOVW | INT_PUN | INT_TER | INT_TOK | INT_RER | INT_ROK);

	dump_phy_stat(device);		
	
	return B_OK;

err:
	delete_sem(device->rxReadySem);
	delete_sem(device->txFreeSem);
	delete_area(device->refArea);
	delete_area(device->txBufArea);
	delete_area(device->rxBufArea);
	delete_area(device->txDescArea);
	delete_area(device->rxDescArea);
	free(device);
	atomic_and(&gOpenMask, ~(1 << dev_id));
	return B_ERROR;
}


status_t
rtl8169_close(void* cookie)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	TRACE("rtl8169_close()\n");
	
	device->closed = true;
	release_sem(device->rxReadySem);
	release_sem(device->txFreeSem);

	return B_OK;
}


status_t
rtl8169_free(void* cookie)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	TRACE("rtl8169_free()\n");

	// disable receiver & transmitter
	write8(REG_CR, read8(REG_CR) & ~(CR_RE | CR_TE));

	// disable interrupts
  	write16(REG_INT_MASK, 0);
  	
  	// well...
  	remove_io_interrupt_handler (device->irq, rtl8169_int, device);

	delete_sem(device->rxReadySem);
	delete_sem(device->txFreeSem);
	delete_area(device->refArea);
	delete_area(device->txBufArea);
	delete_area(device->rxBufArea);
	delete_area(device->txDescArea);
	delete_area(device->rxDescArea);
	free(device);
	atomic_and(&gOpenMask, ~(1 << device->devId));
	return B_OK;
}


status_t
rtl8169_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	cpu_status cpu;
	status_t stat;
	int len;
	TRACE("rtl8169_read() enter\n");
	
	if (device->closed) {
		TRACE("rtl8169_read() interrupted 1\n");
		return B_INTERRUPTED;
	}
retry:
	stat = acquire_sem_etc(device->rxReadySem, 1, B_CAN_INTERRUPT | (device->nonblocking ? B_TIMEOUT : 0), 0);
	if (device->closed) {
		// TRACE("rtl8169_read() interrupted 2\n"); // net_server will crash if we print this (race condition in net_server?)
		return B_INTERRUPTED;
	}
	if (stat == B_WOULD_BLOCK) {
		TRACE("rtl8169_read() would block (OK 0 bytes)\n");
		*num_bytes = 0;
		return B_OK;
	}
	if (stat != B_OK) {
		TRACE("rtl8169_read() error\n");
		return B_ERROR;
	}

	if (device->rxDesc[device->rxNextIndex].stat_len & RX_DESC_OWN) {
		ERROR("rtl8169_read() buffer still in use\n");
		goto retry;
	}

	len = (device->rxDesc[device->rxNextIndex].stat_len & RX_DESC_LEN_MASK);
	len -= 4; // remove CRC that Realtek always appends
	if (len < 0)
		len = 0;
	if (len > *num_bytes)
		len = *num_bytes;

	memcpy(buf, device->rxBuf[device->rxNextIndex], len);
	*num_bytes = len;

	cpu = disable_interrupts();
	acquire_spinlock(&device->rxSpinlock);

	device->rxDesc[device->rxNextIndex].stat_len = RX_DESC_OWN | FRAME_SIZE | (device->rxDesc[device->rxNextIndex].stat_len & RX_DESC_EOR);
	device->rxFree++;

	release_spinlock(&device->rxSpinlock);
	restore_interrupts(cpu);
	
	device->rxNextIndex = (device->rxNextIndex + 1) % RX_DESC_COUNT;
	
	TRACE("rtl8169_read() leave\n");
	return B_OK;
}


status_t
rtl8169_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	cpu_status cpu;
	status_t stat;
	int len;
	
	TRACE("rtl8169_write() enter\n");

	len = *num_bytes;
	if (len > FRAME_SIZE) {
		TRACE("rtl8169_write() buffer too large\n");
		return B_ERROR;
	}

	if (device->closed) {
		TRACE("rtl8169_write() interrupted 1\n");
		return B_INTERRUPTED;
	}
retry:
	stat = acquire_sem_etc(device->txFreeSem, 1, B_CAN_INTERRUPT | B_TIMEOUT, device->nonblocking ? 0 : TX_TIMEOUT);
	if (device->closed) {
		TRACE("rtl8169_write() interrupted 2\n");
		return B_INTERRUPTED;
	}
	if (stat == B_WOULD_BLOCK) {
		TRACE("rtl8169_write() would block (OK 0 bytes)\n");
		*num_bytes = 0;
		return B_OK;
	}
	if (stat == B_TIMED_OUT) {
		TRACE("rtl8169_write() timeout\n");
		return B_BUSY;
	}
	if (stat != B_OK) {
		TRACE("rtl8169_write() error\n");
		return B_ERROR;
	}
	
	if (device->txDesc[device->txNextIndex].stat_len & TX_DESC_OWN) {
		ERROR("rtl8169_write() buffer still in use\n");
		goto retry;
	}

	memcpy(device->txBuf[device->txNextIndex], buffer, len);

	cpu = disable_interrupts();
	acquire_spinlock(&device->txSpinlock);

	device->txUsed++;
	device->txDesc[device->txNextIndex].stat_len = (device->txDesc[device->txNextIndex].stat_len & RX_DESC_EOR) | TX_DESC_OWN | TX_DESC_FS | TX_DESC_LS | len;

	release_spinlock(&device->txSpinlock);
	restore_interrupts(cpu);

	device->txNextIndex = (device->txNextIndex + 1) % TX_DESC_COUNT;
	
	write8(REG_TPPOLL, read8(REG_TPPOLL) | TPPOLL_NPQ); // set queue polling bit

	TRACE("rtl8169_write() leave\n");
	return B_OK;
}


status_t
rtl8169_control(void *cookie, uint32 op, void *arg, size_t len)
{
	rtl8169_device *device = (rtl8169_device *)cookie;

	switch (op) {
		case ETHER_INIT:
			TRACE("rtl8169_control() ETHER_INIT\n");
			return B_OK;
		
		case ETHER_GETADDR:
			TRACE("rtl8169_control() ETHER_GETADDR\n");
			memcpy(arg, &device->macaddr, sizeof(device->macaddr));
			return B_OK;
			
		case ETHER_NONBLOCK:
			if (*(int32 *)arg) {
				TRACE("non blocking mode on\n");
				device->nonblocking = true;
				/* could be used to unblock pending read and write calls,
				 * but this doesn't seem to be required
				release_sem_etc(device->txFreeSem, 1, B_DO_NOT_RESCHEDULE);
				release_sem_etc(device->rxReadySem, 1, B_DO_NOT_RESCHEDULE);
				*/
			} else {
				TRACE("non blocking mode off\n");
				device->nonblocking = false;
			}
			return B_OK;

		case ETHER_ADDMULTI:
			TRACE("rtl8169_control() ETHER_ADDMULTI\n");
			break;
		
		case ETHER_REMMULTI:
			TRACE("rtl8169_control() ETHER_REMMULTI\n");
			return B_OK;
		
		case ETHER_SETPROMISC:
			if (*(int32 *)arg) {
				TRACE("promiscuous mode on\n");
				write32(REG_RX_CONFIG, read32(REG_RX_CONFIG) | RX_CONFIG_AcceptAllPhys);
				write32(0x8, 0xffffffff); // multicast filter
				write32(0xc, 0xffffffff); // multicast filter
			} else {
				TRACE("promiscuous mode off\n");
				write32(REG_RX_CONFIG, read32(REG_RX_CONFIG) & ~RX_CONFIG_AcceptAllPhys);
				write32(0x8, 0); // multicast filter
				write32(0xc, 0); // multicast filter
			}
			return B_OK;

		case ETHER_GETFRAMESIZE:
			TRACE("rtl8169_control() ETHER_GETFRAMESIZE\n");
			*(uint32*)arg = FRAME_SIZE;
			return B_OK;
			
		default:
			TRACE("rtl8169_control() Invalid command\n");
			break;
	}
	
	return B_ERROR;
}

