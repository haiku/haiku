#ifndef __DEVICE_H
#define __DEVICE_H

#include <PCI.h>
#include "hardware.h"

#define TX_TIMEOUT		250000
#define FRAME_SIZE		1536	// must be multiple of 8
#define TX_DESC_COUNT	42		// must be <= 1024
#define RX_DESC_COUNT	42		// must be <= 1024

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

	volatile uint32		blockFlag;
	volatile bool		closed;
	
	sem_id				rxReadySem;
	sem_id				txFreeSem;

	volatile int32		txIndex;
	volatile int32		rxIndex;

	volatile tx_desc *	txDesc;
	volatile tx_desc *	txPrioDesc;
	volatile rx_desc *	rxDesc;
	
	area_id				txDescArea;
	area_id				txPrioDescArea;
	area_id				rxDescArea;
	area_id				txBufArea;
	area_id				rxBufArea;

	void *				txBuf[TX_DESC_COUNT];
	void *				rxBuf[RX_DESC_COUNT];
	
	void *				regAddr;
	area_id				refArea;
	uint8				irq;

	uint8				macaddr[6];
} rtl8169_device;

#define REG8(offset)	((volatile uint8 *)((char *)(device->regAddr) + (offset)))
#define REG16(offset)	((volatile uint16 *)((char *)(device->regAddr) + (offset)))
#define REG32(offset)	((volatile uint32 *)((char *)(device->regAddr) + (offset)))

#endif
