#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "hardware.h"
#include "util.h"

static int32 gOpenMask = 0;

status_t
init_buf_desc(rtl8169_device *device)
{
	void *rx_buf_desc_virt, *rx_buf_desc_phy;
	void *tx_buf_desc_virt, *tx_buf_desc_phy;
	void *tx_prio_buf_desc_virt, *tx_prio_buf_desc_phy;
	void *tx_buf_virt, *tx_buf_phy;
	void *rx_buf_virt, *rx_buf_phy;
	int i;

	device->txBufArea = alloc_mem(&tx_buf_virt, &tx_buf_phy, TX_DESC_COUNT * FRAME_SIZE, 0, "rtl8169 tx buf");
	device->rxBufArea = alloc_mem(&rx_buf_virt, &rx_buf_phy, RX_DESC_COUNT * FRAME_SIZE, 0, "rtl8169 rx buf");
	device->txDescArea = alloc_mem(&tx_buf_desc_virt, &tx_buf_desc_phy, TX_DESC_COUNT * sizeof(tx_desc), 0, "rtl8169 tx desc");
	device->txPrioDescArea = alloc_mem(&tx_prio_buf_desc_virt, &tx_prio_buf_desc_phy, sizeof(tx_desc), 0, "rtl8169 prio tx desc");
	device->rxDescArea = alloc_mem(&rx_buf_desc_virt, &rx_buf_desc_phy, RX_DESC_COUNT * sizeof(rx_desc), 0, "rtl8169 rx desc");
	if (device->txBufArea < B_OK || device->rxBufArea < B_OK || device->txDescArea < B_OK || device->txPrioDescArea < B_OK || device->rxDescArea < B_OK)
		return B_NO_MEMORY;
	
	device->txDesc = (tx_desc *) tx_buf_desc_virt;
	device->txPrioDesc = (tx_desc *) tx_prio_buf_desc_virt;
	device->rxDesc = (rx_desc *) rx_buf_desc_virt;

	// setup unused priority transmit descriptor	
	device->txPrioDesc->stat_len = TX_DESC_FS | TX_DESC_LS | TX_DESC_EOR;
	device->txPrioDesc->vlan     = 0;
	device->txPrioDesc->buf_low  = 0;
	device->txPrioDesc->buf_high = 0;
	
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
		device->rxDesc[i].stat_len = RX_DESC_FS | RX_DESC_LS | RX_DESC_OWN;
		device->rxDesc[i].buf_low  = (uint32)((char *)rx_buf_phy + (i * FRAME_SIZE));
		device->rxDesc[i].buf_high = 0;
	}
	device->rxDesc[i - 1].stat_len |= RX_DESC_EOR;
	
	LOG("tx_buf_desc_phy = %p\n", tx_buf_desc_phy);
	*REG32(REG_TNPDS_LOW) = (uint32)tx_buf_desc_phy;
	LOG("REG_TNPDS_LOW = %p\n", (void *) *REG32(REG_TNPDS_LOW));

	*REG32(REG_TNPDS_HIGH) = 0;
	*REG32(REG_THPDS_LOW) = (uint32)tx_prio_buf_desc_phy;
	*REG32(REG_THPDS_HIGH) = 0;
	*REG32(REG_RDSAR_LOW) = (uint32)rx_buf_desc_phy;
	*REG32(REG_RDSAR_HIGH) = 0;

	return B_OK;
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
	
	LOG("rtl8169_open()\n");

	for (dev_id = 0; (deviceName = gDevNameList[dev_id]) != NULL; dev_id++) {
		if (!strcmp(name, deviceName))
			break;
	}		
	if (deviceName == NULL) {
		LOG("invalid device name");
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
	device->blockFlag = (flags & O_NONBLOCK) ? B_TIMEOUT : 0;
	device->closed = false;
	device->rxReadySem = create_sem(0, "rtl8169 rx ready");
	device->txFreeSem = create_sem(TX_DESC_COUNT, "rtl8169 tx free");
	device->txIndex = 0;
	device->rxIndex = 0;
	
	// enable busmaster and memory mapped access, disable io port access
	val = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_command, 2);
	val = PCI_PCICMD_BME | PCI_PCICMD_MSE | (val & ~PCI_PCICMD_IOS);
	gPci->write_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_command, 2, val);

	// map registers into memory
	val = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, 0x14, 4);
	val &= PCI_address_memory_32_mask;
	LOG("hardware register address %p\n", (void *) val);
	device->refArea = map_mem(&device->regAddr, (void *)val, 256, 0, "rtl8169 register");
	if (device->refArea < B_OK)
		goto err;
		
	LOG("mapped %p to %p\n", (void *)val, device->regAddr);
	
	// get IRQ
	device->irq = gPci->read_pci_config(device->pciInfo->bus, device->pciInfo->device, device->pciInfo->function, PCI_interrupt_line, 1);
	if (device->irq == 0 || device->irq == 0xff)
		goto err;

	LOG("IRQ %d\n", device->irq);
	
	// disable receiver & transmitter
	*REG8(REG_CR) &= ~REG_CR_RE | REG_CR_TE;

	// setup buffer descriptors and buffers
	if (init_buf_desc(device) != B_OK)
		goto err;

	// soft reset
	*REG8(REG_CR) |= REG_CR_RST;
	for (i = 0; ((*REG8(REG_CR)) & REG_CR_RST) && i < 100; i++)
		snooze(10000);
	if (i == 100) {
		LOG("reset failed\n");
		goto err;		
	}
	LOG("reset done, i %d\n", i);
	
	for (i = 0; i < 6; i++)
		device->macaddr[i] = *REG8(i);
		
	dprintf("MAC %02x:%02x:%02x:%02x:%02x:%02x\n",
		device->macaddr[0], device->macaddr[1], device->macaddr[2],
		device->macaddr[3], device->macaddr[4], device->macaddr[5]);

	LOG("tx base = %p\n", (void *) *REG32(REG_TNPDS_LOW));
    
	// enable receiver & transmitter
//	*REG8(REG_CR) |= REG_CR_RE | REG_CR_TE;

	return B_OK;
err:
	LOG("error!\n");
	delete_sem(device->rxReadySem);
	delete_sem(device->txFreeSem);
	delete_area(device->refArea);
	delete_area(device->txBufArea);
	delete_area(device->rxBufArea);
	delete_area(device->txDescArea);
	delete_area(device->txPrioDescArea);
	delete_area(device->rxDescArea);
	free(device);
	atomic_and(&gOpenMask, ~(1 << dev_id));
	return B_ERROR;
}


status_t
rtl8169_close(void* cookie)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	LOG("rtl8169_close()\n");
	
	device->closed = true;
	release_sem(device->rxReadySem);
	release_sem(device->txFreeSem);

	return B_OK;
}


status_t
rtl8169_free(void* cookie)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	LOG("rtl8169_free()\n");

	// disable receiver & transmitter
	*REG8(REG_CR) &= ~REG_CR_RE | REG_CR_TE;

	delete_sem(device->rxReadySem);
	delete_sem(device->txFreeSem);
	delete_area(device->refArea);
	delete_area(device->txBufArea);
	delete_area(device->rxBufArea);
	delete_area(device->txDescArea);
	delete_area(device->txPrioDescArea);
	delete_area(device->rxDescArea);
	free(device);
	atomic_and(&gOpenMask, ~(1 << device->devId));
	return B_OK;
}


status_t
rtl8169_read(void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	status_t stat;
	LOG("rtl8169_read()\n");
	
	if (device->closed)
		return B_INTERRUPTED;
retry:
	stat = acquire_sem_etc(device->rxReadySem, 1, B_CAN_INTERRUPT | device->blockFlag, 0);
	if (device->closed)
		return B_INTERRUPTED;
	if (stat == B_WOULD_BLOCK) {
		*num_bytes = 0;
		return B_OK;
	}
	if (stat != B_OK)
		return B_ERROR;


	
	return B_OK;
}


status_t
rtl8169_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	rtl8169_device *device = (rtl8169_device *)cookie;
	status_t stat;
	
	LOG("rtl8169_write()\n");

	if (device->closed)
		return B_INTERRUPTED;

	stat = acquire_sem_etc(device->txFreeSem, 1, B_CAN_INTERRUPT | B_TIMEOUT, TX_TIMEOUT);
	if (device->closed)
		return B_INTERRUPTED;
	if (device->blockFlag && stat == B_WOULD_BLOCK) {
		*num_bytes = 0;
		return B_OK;
	}
	if (stat != B_OK)
		return B_ERROR;
	


	return B_OK;
}



status_t
rtl8169_control(void *cookie, uint32 op, void *arg, size_t len)
{
	rtl8169_device *device = (rtl8169_device *)cookie;

	LOG("rtl8169_control()\n");

	
	switch (op) {
		case ETHER_INIT:
			LOG("ETHER_INIT\n");
			return B_OK;
		
		case ETHER_GETADDR:
			LOG("ETHER_GETADDR\n");
			memcpy(arg, &device->macaddr, sizeof(device->macaddr));
			return B_OK;
			
		case ETHER_NONBLOCK:
			LOG("ETHER_NON_BLOCK\n");
			device->blockFlag = *(int32 *)arg ? B_TIMEOUT : 0;
			return B_OK;

		case ETHER_ADDMULTI:
			LOG("ETHER_ADDMULTI\n");
			break;
		
		case ETHER_REMMULTI:
			LOG("ETHER_REMMULTI\n");
			return B_OK;
		
		case ETHER_SETPROMISC:
			LOG("ETHER_SETPROMISC\n");
			return B_OK;
		
		case ETHER_GETFRAMESIZE:
			LOG("ETHER_GETFRAMESIZE\n");
			*(uint32*)arg = FRAME_SIZE;
			return B_OK;
			
		default:
			LOG("Invalid command\n");
			break;
	}
	
	return B_ERROR;
}

