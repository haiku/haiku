/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//!	DMA helper functions


#include "ide_internal.h"

#define CHECK_DEV_DMA_MODE(infoblock, elem, mode, this_mode, num_modes ) \
	if( infoblock->elem ) { \
		mode = this_mode; \
		++num_modes; \
	}


int
get_device_dma_mode(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;

	int num_modes, mode;

	mode = 0;
	num_modes = 0;

	if (!infoblock->DMA_supported)
		return -1;

	CHECK_DEV_DMA_MODE(infoblock, MDMA0_selected, mode, 0, num_modes);
	CHECK_DEV_DMA_MODE(infoblock, MDMA1_selected, mode, 1, num_modes);
	CHECK_DEV_DMA_MODE(infoblock, MDMA2_selected, mode, 2, num_modes);

	if (infoblock->_88_valid) {
		CHECK_DEV_DMA_MODE(infoblock, UDMA0_selected, mode, 0x10, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA1_selected, mode, 0x11, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA2_selected, mode, 0x12, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA3_selected, mode, 0x13, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA4_selected, mode, 0x14, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA5_selected, mode, 0x15, num_modes);
		CHECK_DEV_DMA_MODE(infoblock, UDMA6_selected, mode, 0x16, num_modes);
	}

	if (num_modes != 1)
		return -1;

	SHOW_FLOW(3, "%x", mode);

	return mode;
}


bool
configure_dma(ide_device_info *device)
{
	if (get_device_dma_mode(device) != -1) {
		device->DMA_enabled = device->DMA_supported = device->bus->can_DMA;
		if (device->DMA_enabled) {
			dprintf("IDE: enabling DMA\n");
		} else {
			dprintf("IDE: disabling DMA (failsafe option selected)\n");
		}
	} else {
		device->DMA_enabled = false;
		dprintf("IDE: DMA not possible, disabling\n");
	}

	return true;
}


/*!	Abort DMA transmission
	must be called _before_ start_dma_wait
*/
void
abort_dma(ide_device_info *device, ide_qrequest *qrequest)
{
	ide_bus_info *bus = device->bus;

	SHOW_FLOW0(0, "");

	bus->controller->finish_dma(bus->channel_cookie);
}


/*!	Prepare DMA transmission
	on return, DMA engine waits for device to transmit data
	warning: doesn't set sense data on error
*/
bool
prepare_dma(ide_device_info *device, ide_qrequest *qrequest)
{
	ide_bus_info *bus = device->bus;
	scsi_ccb *request = qrequest->request;
	status_t res;

	res = bus->controller->prepare_dma(bus->channel_cookie, request->sg_list,
		request->sg_count, qrequest->is_write);
	if (res != B_OK)
		return false;

	return true;
}


/*! Start waiting for DMA to be finished */
void
start_dma_wait(ide_device_info *device, ide_qrequest *qrequest)
{
	ide_bus_info *bus = device->bus;

	bus->controller->start_dma(bus->channel_cookie);

	start_waiting(bus, qrequest->request->timeout > 0 ? 
		qrequest->request->timeout : IDE_STD_TIMEOUT, ide_state_async_waiting);
}


/*! Start waiting for DMA to be finished with bus lock not hold */
void
start_dma_wait_no_lock(ide_device_info *device, ide_qrequest *qrequest)
{
	ide_bus_info *bus = device->bus;

	IDE_LOCK(bus);
	start_dma_wait(device, qrequest);
}


/*! Finish dma transmission after device has fired IRQ */
bool
finish_dma(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;
	status_t dma_res;

	dma_res = bus->controller->finish_dma(bus->channel_cookie);

	return dma_res == B_OK || dma_res == B_DEV_DATA_OVERRUN;
}

