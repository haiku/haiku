/* Realtek RTL8169 Family Driver
 * Copyright (C) 2004 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 */
#ifndef __DEVICE_H
#define __DEVICE_H

#include <PCI.h>
#include "hardware.h"

#define TX_TIMEOUT		6000000		// 6 seconds
#define FRAME_SIZE		1536		// must be multiple of 8
#define TX_DESC_COUNT	256			// must be <= 1024
#define RX_DESC_COUNT	256			// must be <= 1024

extern pci_module_info *gPci;

status_t rtl8169_open(const char *name, uint32 flags, void** cookie);
status_t rtl8169_read(void* cookie, off_t position, void *buf, size_t* num_bytes);
status_t rtl8169_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes);
status_t rtl8169_control(void *cookie, uint32 op, void *arg, size_t len);
status_t rtl8169_close(void* cookie);
status_t rtl8169_free(void* cookie);

typedef struct {
	int 				devId;
	pci_info *			pciInfo;

	volatile bool		nonblocking;
	volatile bool		closed;
	
	spinlock			txSpinlock;
	sem_id				txFreeSem;
	volatile int32		txNextIndex;	// next descriptor that will be used for writing
	volatile int32		txIntIndex;		// current descriptor that needs be checked
	volatile int32		txUsed;
	
	spinlock			rxSpinlock;
	sem_id				rxReadySem;
	volatile int32		rxNextIndex;	// next descriptor that will be used for reading
	volatile int32		rxIntIndex;		// current descriptor that needs be checked
	volatile int32		rxFree;


	volatile buf_desc *	txDesc;
	volatile buf_desc *	rxDesc;
	
	area_id				txDescArea;
	area_id				rxDescArea;
	area_id				txBufArea;
	area_id				rxBufArea;

	void *				txBuf[TX_DESC_COUNT];
	void *				rxBuf[RX_DESC_COUNT];
	
	void *				regAddr;
	area_id				refArea;

	uint8				irq;
	uint8				macaddr[6];
	int					mac_version;
	int					phy_version;
} rtl8169_device;

#define read8(offset)			(*(volatile uint8 *) ((char *)(device->regAddr) + (offset)))
#define read16(offset)			(*(volatile uint16 *)((char *)(device->regAddr) + (offset)))
#define read32(offset)			(*(volatile uint32 *)((char *)(device->regAddr) + (offset)))
#define write8(offset, value)	(*(volatile uint8 *) ((char *)(device->regAddr) + (offset)) = value)
#define write16(offset, value)	(*(volatile uint16 *)((char *)(device->regAddr) + (offset)) = value)
#define write32(offset, value)	(*(volatile uint32 *)((char *)(device->regAddr) + (offset)) = value)

#endif
