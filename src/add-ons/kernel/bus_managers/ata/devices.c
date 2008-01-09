/*
** Copyright 2007, Marcus Overhagen. All rights reserved.
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the Haiku License.
*/

/*
	Part of Open IDE bus manager

	Device manager

	As the IDE bus manager is an SCSI to IDE translater, it
	has to know a bit more about connected devices then a standard
	SIM. This file contains device detection and classification.
*/

#include "ide_internal.h"
#include "ide_sim.h"
#include "ide_cmds.h"

#include <string.h>
#include <malloc.h>
#include <ByteOrder.h>

#define TRACE(x...) dprintf("IDE: " x)

/** cleanup links devices on one bus when <device> is deleted */

static void
cleanup_device_links(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;

	TRACE("cleanup_device_links: device %p\n", device);

	bus->devices[device->is_device1] = NULL;

	if (device->other_device) {
		if (device->other_device != device) {
			device->other_device->other_device = device->other_device;
			bus->first_device = device->other_device;
		} else
			bus->first_device = NULL;
	}

	device->other_device = NULL;
}


/** destroy device info */

void
destroy_device(ide_device_info *device)
{
	TRACE("destroy_device: device %p\n", device);

	// paranoia
	device->exec_io = NULL;
	cancel_timer(&device->reconnect_timer.te);

	scsi->free_dpc(device->reconnect_timeout_dpc);

	cleanup_device_links(device);

	destroy_qreq_array(device);

	uninit_synced_pc(&device->reconnect_timeout_synced_pc);

	free(device);
}


/** setup links between the devices on one bus */

static void
setup_device_links(ide_bus_info *bus, ide_device_info *device)
{
	TRACE("setup_device_links: bus %p, device %p\n", bus, device);

	device->bus = bus;
	bus->devices[device->is_device1] = device;

	device->other_device = device;

	if (device->is_device1) {
		if (bus->devices[0]) {
			device->other_device = bus->devices[0];
			bus->devices[0]->other_device = device;
		}
	} else {
		if (bus->devices[1]) {
			device->other_device = bus->devices[1];
			bus->devices[1]->other_device = device;
		}
	}

	if (bus->first_device == NULL)
		bus->first_device = device;
}


/** create device info */

ide_device_info *
create_device(ide_bus_info *bus, bool is_device1)
{
	ide_device_info *device;

	TRACE("create_device: bus %p, device-number %d\n", bus, is_device1);

	device = (ide_device_info *)malloc(sizeof(*device));
	if (device == NULL) 
		return NULL;

	memset(device, 0, sizeof(*device));

	device->is_device1 = is_device1;
	device->target_id = is_device1;

	setup_device_links(bus, device);

	device->DMA_failures = 0;
	device->CQ_failures = 0;
	device->num_failed_send = 0;

	device->combined_sense = 0;

	device->num_running_reqs = 0;

	device->reconnect_timer.device = device;

	init_synced_pc(&device->reconnect_timeout_synced_pc, 
		reconnect_timeout_worker);
	
	if (scsi->alloc_dpc(&device->reconnect_timeout_dpc) != B_OK)
		goto err;

	device->total_sectors = 0;
	return device;

err:
	destroy_device(device);
	return NULL;
}

#if B_HOST_IS_LENDIAN

#define B_BENDIAN_TO_HOST_MULTI(v, n) do {		\
	size_t __swap16_multi_n = (n);				\
	uint16 *__swap16_multi_v = (v);				\
												\
	while( __swap16_multi_n ) {					\
		*__swap16_multi_v = B_SWAP_INT16(*__swap16_multi_v); \
		__swap16_multi_v++;						\
		__swap16_multi_n--;						\
	}											\
} while (0)

#else

#define B_BENDIAN_TO_HOST_MULTI(v, n) 

#endif


/** prepare infoblock for further use, i.e. fix endianess */

static void
prep_infoblock(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;

	B_BENDIAN_TO_HOST_MULTI((uint16 *)infoblock->serial_number, 
		sizeof(infoblock->serial_number) / 2);

	B_BENDIAN_TO_HOST_MULTI( (uint16 *)infoblock->firmware_version, 
		sizeof(infoblock->firmware_version) / 2);

	B_BENDIAN_TO_HOST_MULTI( (uint16 *)infoblock->model_number, 
		sizeof(infoblock->model_number) / 2);

	infoblock->LBA_total_sectors = B_LENDIAN_TO_HOST_INT32(infoblock->LBA_total_sectors);
	infoblock->LBA48_total_sectors = B_LENDIAN_TO_HOST_INT64(infoblock->LBA48_total_sectors);
}


/** scan one device */
status_t
scan_device(ide_device_info *device, bool isAtapi)
{
	dprintf("ATA: scan_device\n");

	if (ata_read_infoblock(device, isAtapi) != B_OK) {
		dprintf("ATA: couldn't read infoblock for device %p\n", device);
		return B_ERROR;
	}

	device->subsys_status = SCSI_REQ_CMP;

	prep_infoblock(device);
	return B_OK;
}
