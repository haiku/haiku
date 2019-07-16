/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI bus manager

	Bus node layer.

	Whenever a controller driver publishes a new controller, a new SCSI bus
	for public and internal use is registered in turn. After that, this
	bus is told to rescan for devices. For each device, there is a
	device registered for peripheral drivers. (see devices.c)
*/

#include "scsi_internal.h"

#include <string.h>
#include <malloc.h>


// bus service should hurry up a bit - good controllers don't take much time
// but are very happy to be busy; don't make it realtime though as we
// don't really need that but would risk to steel processing power of
// realtime-demanding threads
#define BUS_SERVICE_PRIORITY B_URGENT_DISPLAY_PRIORITY


/**	implementation of service thread:
 *	it handles DPC and pending requests
 */

static void
scsi_do_service(scsi_bus_info *bus)
{
	while (true) {
		SHOW_FLOW0( 3, "" );

		// handle DPCs first as they are more urgent
		if (scsi_check_exec_dpc(bus))
			continue;

		if (scsi_check_exec_service(bus))
			continue;

		break;
	}
}


/** main loop of service thread */

static int32
scsi_service_threadproc(void *arg)
{
	scsi_bus_info *bus = (scsi_bus_info *)arg;
	int32 processed_notifications = 0;

	SHOW_FLOW(3, "bus = %p", bus);

	while (true) {
		// we handle multiple requests in scsi_do_service at once;
		// to save time, we will acquire all notifications that are sent
		// up to now at once.
		// (Sadly, there is no "set semaphore to zero" function, so this
		//  is a poor-man emulation)
		acquire_sem_etc(bus->start_service, processed_notifications + 1, 0, 0);

		SHOW_FLOW0( 3, "1" );

		if (bus->shutting_down)
			break;

		// get number of notifications _before_ servicing to make sure no new
		// notifications are sent after do_service()
		get_sem_count(bus->start_service, &processed_notifications);

		scsi_do_service(bus);
	}

	return 0;
}


static scsi_bus_info *
scsi_create_bus(device_node *node, uint8 path_id)
{
	scsi_bus_info *bus;
	int res;

	SHOW_FLOW0(3, "");

	bus = (scsi_bus_info *)malloc(sizeof(*bus));
	if (bus == NULL)
		return NULL;

	memset(bus, 0, sizeof(*bus));

	bus->path_id = path_id;

	if (pnp->get_attr_uint32(node, SCSI_DEVICE_MAX_TARGET_COUNT, &bus->max_target_count, true) != B_OK)
		bus->max_target_count = MAX_TARGET_ID + 1;
	if (pnp->get_attr_uint32(node, SCSI_DEVICE_MAX_LUN_COUNT, &bus->max_lun_count, true) != B_OK)
		bus->max_lun_count = MAX_LUN_ID + 1;

	// our scsi_ccb only has a uchar for target_id
	if (bus->max_target_count > 256)
		bus->max_target_count = 256;
	// our scsi_ccb only has a uchar for target_lun
	if (bus->max_lun_count > 256)
		bus->max_lun_count = 256;

	bus->node = node;
	bus->lock_count = bus->blocked[0] = bus->blocked[1] = 0;
	bus->sim_overflow = 0;
	bus->shutting_down = false;

	bus->waiting_devices = NULL;
	//bus->resubmitted_req = NULL;

	bus->dpc_list = NULL;

	if ((bus->scan_lun_lock = create_sem(1, "scsi_scan_lun_lock")) < 0) {
		res = bus->scan_lun_lock;
		goto err6;
	}

	bus->start_service = create_sem(0, "scsi_start_service");
	if (bus->start_service < 0) {
		res = bus->start_service;
		goto err4;
	}

	res = INIT_BEN(&bus->mutex, "scsi_bus_mutex");

	if (res < B_OK)
		goto err3;

	spinlock_irq_init(&bus->dpc_lock);

	res = scsi_init_ccb_alloc(bus);
	if (res < B_OK)
		goto err2;

	bus->service_thread = spawn_kernel_thread(scsi_service_threadproc,
		"scsi_bus_service", BUS_SERVICE_PRIORITY, bus);

	if (bus->service_thread < 0) {
		res = bus->service_thread;
		goto err1;
	}

	resume_thread(bus->service_thread);

	return bus;

err1:
	scsi_uninit_ccb_alloc(bus);
err2:
	DELETE_BEN(&bus->mutex);
err3:
	delete_sem(bus->start_service);
err4:
	delete_sem(bus->scan_lun_lock);
err6:
	free(bus);
	return NULL;
}


static status_t
scsi_destroy_bus(scsi_bus_info *bus)
{
	int32 retcode;

	// noone is using this bus now, time to clean it up
	bus->shutting_down = true;
	release_sem(bus->start_service);

	wait_for_thread(bus->service_thread, &retcode);

	delete_sem(bus->start_service);
	DELETE_BEN(&bus->mutex);
	delete_sem(bus->scan_lun_lock);

	scsi_uninit_ccb_alloc(bus);

	return B_OK;
}


static status_t
scsi_init_bus(device_node *node, void **cookie)
{
	uint8 path_id;
	scsi_bus_info *bus;
	status_t res;

	SHOW_FLOW0( 3, "" );

	if (pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &path_id, false) != B_OK)
		return B_ERROR;

	bus = scsi_create_bus(node, path_id);
	if (bus == NULL)
		return B_NO_MEMORY;

	// extract controller/protocoll restrictions from node
	if (pnp->get_attr_uint32(node, B_DMA_ALIGNMENT, &bus->dma_params.alignment,
			true) != B_OK)
		bus->dma_params.alignment = 0;
	if (pnp->get_attr_uint32(node, B_DMA_MAX_TRANSFER_BLOCKS,
			&bus->dma_params.max_blocks, true) != B_OK)
		bus->dma_params.max_blocks = 0xffffffff;
	if (pnp->get_attr_uint32(node, B_DMA_BOUNDARY,
			&bus->dma_params.dma_boundary, true) != B_OK)
		bus->dma_params.dma_boundary = ~0;
	if (pnp->get_attr_uint32(node, B_DMA_MAX_SEGMENT_BLOCKS,
			&bus->dma_params.max_sg_block_size, true) != B_OK)
		bus->dma_params.max_sg_block_size = 0xffffffff;
	if (pnp->get_attr_uint32(node, B_DMA_MAX_SEGMENT_COUNT,
			&bus->dma_params.max_sg_blocks, true) != B_OK)
		bus->dma_params.max_sg_blocks = ~0;

	// do some sanity check:
	bus->dma_params.max_sg_block_size &= ~bus->dma_params.alignment;

	if (bus->dma_params.alignment > B_PAGE_SIZE) {
		SHOW_ERROR(0, "Alignment (0x%" B_PRIx32 ") must be less then "
			"B_PAGE_SIZE", bus->dma_params.alignment);
		res = B_ERROR;
		goto err;
	}

	if (bus->dma_params.max_sg_block_size < 1) {
		SHOW_ERROR(0, "Max s/g block size (0x%" B_PRIx32 ") is too small",
			bus->dma_params.max_sg_block_size);
		res = B_ERROR;
		goto err;
	}

	if (bus->dma_params.dma_boundary < B_PAGE_SIZE - 1) {
		SHOW_ERROR(0, "DMA boundary (0x%" B_PRIx32 ") must be at least "
			"B_PAGE_SIZE", bus->dma_params.dma_boundary);
		res = B_ERROR;
		goto err;
	}

	if (bus->dma_params.max_blocks < 1 || bus->dma_params.max_sg_blocks < 1) {
		SHOW_ERROR(0, "Max blocks (%" B_PRIu32 ") and max s/g blocks (%"
			B_PRIu32 ") must be at least 1", bus->dma_params.max_blocks,
			bus->dma_params.max_sg_blocks);
		res = B_ERROR;
		goto err;
	}

	{
		device_node *parent = pnp->get_parent_node(node);
		pnp->get_driver(parent, (driver_module_info **)&bus->interface,
			(void **)&bus->sim_cookie);
		pnp->put_node(parent);

		bus->interface->set_scsi_bus(bus->sim_cookie, bus);
	}

	// cache inquiry data
	scsi_inquiry_path(bus, &bus->inquiry_data);

	// get max. number of commands on bus
	bus->left_slots = bus->inquiry_data.hba_queue_size;
	SHOW_FLOW( 3, "Bus has %d slots", bus->left_slots );

	*cookie = bus;

	return B_OK;

err:
	scsi_destroy_bus(bus);
	return res;
}


static void
scsi_uninit_bus(scsi_bus_info *bus)
{
	scsi_destroy_bus(bus);
}


uchar
scsi_inquiry_path(scsi_bus bus, scsi_path_inquiry *inquiry_data)
{
	SHOW_FLOW(4, "path_id=%d", bus->path_id);
	return bus->interface->path_inquiry(bus->sim_cookie, inquiry_data);
}


static uchar
scsi_reset_bus(scsi_bus_info *bus)
{
	return bus->interface->reset_bus(bus->sim_cookie);
}


static status_t
scsi_bus_module_init(void)
{
	SHOW_FLOW0(4, "");
	return init_temp_sg();
}


static status_t
scsi_bus_module_uninit(void)
{
	SHOW_INFO0(4, "");

	uninit_temp_sg();
	return B_OK;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
			return scsi_bus_module_init();
		case B_MODULE_UNINIT:
			return scsi_bus_module_uninit();

		default:
			return B_ERROR;
	}
}


scsi_bus_interface scsi_bus_module = {
	{
		{
			SCSI_BUS_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		NULL,	// register node
		scsi_init_bus,
		(void (*)(void *))scsi_uninit_bus,
		(status_t (*)(void *))scsi_scan_bus,
		(status_t (*)(void *))scsi_scan_bus,
		NULL
	},

	scsi_inquiry_path,
	scsi_reset_bus,
};
