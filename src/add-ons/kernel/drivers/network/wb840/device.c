/* Copyright (c) 2003-2011 
 * Stefano Ceccherini <stefano.ceccherini@gmail.com>. All rights reserved.
 * This file is released under the MIT license
 */
#include <KernelExport.h>
#include <Errors.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <net/if_media.h>

#include "debug.h"
#include "device.h"
#include "driver.h"
#include "interface.h"
#include "wb840.h"


#define MAX_CARDS 4

extern char* gDevNameList[];
extern pci_info* gDevList[];

static int32 sOpenMask = 0;

static status_t
wb840_open(const char* name, uint32 flags, void** cookie)
{
	char* deviceName = NULL;
	int32 i;
	int32 mask;
	struct wb_device* data;
	status_t status;
	
	LOG((DEVICE_NAME ": open()\n"));
	
	for (i = 0; (deviceName = gDevNameList[i]) != NULL; i++) {
		if (!strcmp(name, deviceName))
			break;
	}		
	
	if (deviceName == NULL) {
		LOG(("invalid device name"));
		return EINVAL;
	}
	
	// There can be only one access at time
	mask = 1L << i;
	if (atomic_or(&sOpenMask, mask) & mask)
		return B_BUSY;
	
	// Allocate a wb_device structure
	if (!(data = (wb_device*)calloc(1, sizeof(wb_device)))) {
		sOpenMask &= ~(1L << i);
		return B_NO_MEMORY;
	}
		
	*cookie = data;

#ifdef DEBUG
	load_driver_symbols("wb840");
#endif

	data->devId = i;
	data->pciInfo = gDevList[i];
	data->deviceName = gDevNameList[i];
	data->blockFlag = 0;
	data->reg_base = data->pciInfo->u.h0.base_registers[0];	
	data->wb_cachesize = gPci->read_pci_config(data->pciInfo->bus,
		data->pciInfo->device, data->pciInfo->function, PCI_line_size,
		sizeof(PCI_line_size)) & 0xff;
		
	wb_read_eeprom(data, &data->MAC_Address, 0, 3, false);
	
	status = wb_create_semaphores(data);
	if (status < B_OK) {
		LOG((DEVICE_NAME": couldn't create semaphores\n"));
		goto err;
	}
		
	status = wb_stop(data);
	if (status < B_OK) {
		LOG((DEVICE_NAME": can't stop device\n"));
		goto err1;
	}
			
	status = wb_initPHYs(data);
	if (status < B_OK) {
		LOG((DEVICE_NAME": can't init PHYs\n"));
		goto err1;
	}
		
	wb_init(data);
		
	/* Setup interrupts */
	data->irq = data->pciInfo->u.h0.interrupt_line;
	status = install_io_interrupt_handler(data->irq, wb_interrupt, data, 0);
	if (status < B_OK) {
		LOG((DEVICE_NAME
			": can't install interrupt handler: %s\n", strerror(status)));
		goto err1;		
	}
	
	LOG((DEVICE_NAME ": interrupts installed at irq line %x\n", data->irq));
		
	status = wb_create_rings(data);
	if (status < B_OK) {
		LOG((DEVICE_NAME": can't create ring buffers\n"));
		goto err2;
	}
	
	wb_enable_interrupts(data);	
	
	WB_SETBIT(data->reg_base + WB_NETCFG, WB_NETCFG_RX_ON);
	write32(data->reg_base + WB_RXSTART, 0xFFFFFFFF);
	WB_SETBIT(data->reg_base + WB_NETCFG, WB_NETCFG_TX_ON);
	
	add_timer(&data->timer, wb_tick, 1000000LL, B_PERIODIC_TIMER);
		
	return B_OK; // Everything after this line is an error
		
err2:
	remove_io_interrupt_handler(data->irq, wb_interrupt, data);
	
err1:
	wb_delete_semaphores(data);
	
err:			
	sOpenMask &= ~(1L << i);
	
	free(data);	
	LOG(("wb840: Open Failed\n"));
	
	return status;
}


static status_t
wb840_read(void* cookie, off_t position, void* buf, size_t* num_bytes)
{
	wb_device* device = (wb_device*)cookie;
	int16 current;
	status_t status;
	size_t size;
	int32 blockFlag;
	uint32 check;
	
	LOG((DEVICE_NAME ": read()\n"));
	
	blockFlag = device->blockFlag;
	
	if (atomic_or(&device->rxLock, 1)) {
		*num_bytes = 0;
		return B_ERROR;
	}
	
	status = acquire_sem_etc(device->rxSem, 1, B_CAN_INTERRUPT | blockFlag, 0);
	if (status < B_OK) {
		atomic_and(&device->rxLock, 0);
		*num_bytes = 0;
		return status;
	}
	
	current = device->rxCurrent;
	check = device->rxDescriptor[current].wb_status;	 
	if (check & WB_RXSTAT_OWN) {
		LOG((DEVICE_NAME ":ERROR: read: buffer %d still in use: %x\n",
			(int)current, (int)status));
		atomic_and(&device->rxLock, 0);
		*num_bytes = 0;
		return B_BUSY;
	}
	
	if (check & (WB_RXSTAT_RXERR | WB_RXSTAT_CRCERR | WB_RXSTAT_RUNT)) {
		LOG(("Error read: packet with errors."));
		*num_bytes = 0;
	} else {
		size = WB_RXBYTES(check);
		size -= CRC_SIZE;
		LOG((DEVICE_NAME": received %ld bytes\n", size));
		if (size > WB_MAX_FRAMELEN || size > *num_bytes) {
			LOG(("ERROR: Bad frame size: %ld", size));
			size = *num_bytes;
		}
		*num_bytes = size;
		memcpy(buf, (void*)device->rxBuffer[current], size);
	}
	
	device->rxCurrent = (current + 1) & WB_RX_CNT_MASK;
	{
		cpu_status former;
		former = disable_interrupts();
		acquire_spinlock(&device->rxSpinlock);

		// release buffer to ring
		wb_put_rx_descriptor(&device->rxDescriptor[current]);
		device->rxFree++;

		release_spinlock(&device->rxSpinlock);
   		restore_interrupts(former);
	}
	
	atomic_and(&device->rxLock, 0);
	
	return B_OK;
}


static status_t
wb840_write(void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	wb_device* device = (wb_device*)cookie;
	status_t status = B_OK;
	uint16 frameSize;
	int16 current;
	uint32 check;
	
	LOG((DEVICE_NAME ": write()\n"));
	
	atomic_add(&device->txLock, 1);

	if (*num_bytes > WB_MAX_FRAMELEN)
		*num_bytes = WB_MAX_FRAMELEN;

	frameSize = *num_bytes;
	current = device->txCurrent;

	// block until a free tx descriptor is available
	status = acquire_sem_etc(device->txSem, 1, B_TIMEOUT, ETHER_TRANSMIT_TIMEOUT);
	if (status < B_OK) {
		write32(device->reg_base + WB_TXSTART, 0xFFFFFFFF);
		LOG((DEVICE_NAME": write: acquiring sem failed: %ld, %s\n",
			status, strerror(status)));
		atomic_add(&device->txLock, -1);
		*num_bytes = 0;
		return status;
	}
	
	check = device->txDescriptor[current].wb_status;
	if (check & WB_TXSTAT_OWN) {
		// descriptor is still in use
		dprintf(DEVICE_NAME ": card owns buffer %d\n", (int)current);
		atomic_add(&device->txLock, -1);
		*num_bytes = 0;
		return B_ERROR;
	}

	/* Copy data to tx buffer */
	memcpy((void*)device->txBuffer[current], buffer, frameSize);
	device->txCurrent = (current + 1) & WB_TX_CNT_MASK;
	LOG((DEVICE_NAME ": %d bytes written\n", frameSize));
	
	{
		cpu_status former = disable_interrupts();
		acquire_spinlock(&device->txSpinlock);
		
		device->txDescriptor[current].wb_ctl = WB_TXCTL_TLINK | frameSize;
		device->txDescriptor[current].wb_ctl |= WB_TXCTL_FIRSTFRAG
			| WB_TXCTL_LASTFRAG;
		device->txDescriptor[current].wb_status = WB_TXSTAT_OWN;
		device->txSent++;

		release_spinlock(&device->txSpinlock);
   		restore_interrupts(former);
	}

	// re-enable transmit state machine
	write32(device->reg_base + WB_TXSTART, 0xFFFFFFFF);

	atomic_add(&device->txLock, -1);
	
	return B_OK;
}


static status_t
wb840_control (void* cookie, uint32 op, void* arg, size_t len)
{
	wb_device* data = (wb_device*)cookie;
	
	LOG((DEVICE_NAME ": control()\n"));
	switch (op) {
		case ETHER_INIT:
			LOG(("%s: ETHER_INIT\n", data->deviceName));
			return B_OK;
		
		case ETHER_GETADDR:
			LOG(("%s: ETHER_GETADDR\n", data->deviceName));
			memcpy(arg, &data->MAC_Address, sizeof(data->MAC_Address));
			print_address(arg);
			return B_OK;
			
		case ETHER_NONBLOCK:
			LOG(("ETHER_NON_BLOCK\n"));
			data->blockFlag = *(int32*)arg ? B_TIMEOUT : 0;
			return B_OK;
		
		case ETHER_GETFRAMESIZE:
			LOG(("ETHER_GETFRAMESIZE\n"));
			*(uint32 *)arg = WB_MAX_FRAMELEN;
			return B_OK;
			
		case ETHER_GET_LINK_STATE:
		{
			ether_link_state_t state;
			LOG(("ETHER_GET_LINK_STATE"));
						
			state.media = (data->link ? IFM_ACTIVE : 0) | IFM_ETHER
				| (data->full_duplex ? IFM_FULL_DUPLEX : IFM_HALF_DUPLEX)
				| (data->speed == LINK_SPEED_100_MBIT ? IFM_100_TX : IFM_10_T);
			state.speed = data->speed == LINK_SPEED_100_MBIT
				? 100000000 : 10000000;
			state.quality = 1000;

			return user_memcpy(arg, &state, sizeof(ether_link_state_t));
		}

		case ETHER_ADDMULTI:
			LOG(("ETHER_ADDMULTI\n"));
			break;
		
		case ETHER_REMMULTI:
			LOG(("ETHER_REMMULTI\n"));
			break;
		
		case ETHER_SETPROMISC:
			LOG(("ETHER_SETPROMISC\n"));
			break;
			
		default:
			LOG(("Invalid command\n"));
			break;
	}
	
	return B_ERROR;
}


static status_t
wb840_close(void* cookie)
{
	wb_device* device = (wb_device*)cookie;
	
	LOG((DEVICE_NAME ": close()\n"));
	
	cancel_timer(&device->timer);
		
	wb_stop(device);
	
	write32(device->reg_base + WB_TXADDR, 0x00000000);
	write32(device->reg_base + WB_RXADDR, 0x00000000);

	wb_disable_interrupts(device);	
	remove_io_interrupt_handler(device->irq, wb_interrupt, device);
	
	delete_sem(device->rxSem);
	delete_sem(device->txSem);
			
	return B_OK;
}


static status_t
wb840_free(void* cookie)
{
	wb_device* device = (wb_device*)cookie;
	
	LOG((DEVICE_NAME ": free()\n"));
	
	sOpenMask &= ~(1L << device->devId);
	
	wb_delete_rings(device);
	free(device->firstPHY);
	free(device);
		
	return B_OK;
}


device_hooks
gDeviceHooks = {
	wb840_open, 	/* -> open entry point */
	wb840_close, 	/* -> close entry point */
	wb840_free,		/* -> free cookie */
	wb840_control, 	/* -> control entry point */
	wb840_read,		/* -> read entry point */
	wb840_write,	/* -> write entry point */
	NULL,
	NULL,
	NULL,
	NULL
};
