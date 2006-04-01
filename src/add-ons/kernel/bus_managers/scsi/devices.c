/*
 * Copyright 2004-2006, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Device node layer. 

	When a SCSI bus is registered, this layer scans for SCSI devices
	and registers a node for each of them. Peripheral drivers are on
	top of these nodes.
*/

#include "scsi_internal.h"

#include <block_io.h>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifdef USE_FAST_LOG
static fast_log_event_type scsi_device_events[] = {
	{ ev_scsi_requeue_request, "ev_scsi_requeue_request" },
	{ ev_scsi_resubmit_request, "ev_scsi_resubmit_request" },
	{ ev_scsi_submit_autosense, "ev_scsi_submit_autosense" },
	{ ev_scsi_finish_autosense, "ev_scsi_finish_autosense" },
	{ ev_scsi_device_queue_overflow, "ev_scsi_device_queue_overflow" },
	{ ev_scsi_request_finished, "ev_scsi_request_finished" },
	{ ev_scsi_async_io, "ev_scsi_async_io" },
	{ ev_scsi_do_resend_request, "ev_scsi_do_resend_request" },
	{ ev_copy_sg_data, "ev_copy_sg_data" },
	{}
};
#endif


/** free autosense request of device */

static void 
scsi_free_autosense_request(scsi_device_info *device)
{
	SHOW_FLOW0( 3, "" );

	if (device->auto_sense_request != NULL) {
		scsi_free_ccb(device->auto_sense_request);
		device->auto_sense_request = NULL;
	}

	if (device->auto_sense_area > 0) {
		delete_area(device->auto_sense_area);
		device->auto_sense_area = 0;
	}
}


/** free all data of device */

static void 
scsi_free_device(scsi_device_info *device)
{
	SHOW_FLOW0( 3, "" );

	scsi_free_emulation_buffer(device);
	scsi_free_autosense_request(device);

	unregister_kernel_daemon(scsi_dma_buffer_daemon, device);

	scsi_dma_buffer_free(&device->dma_buffer);

	DELETE_BEN(&device->dma_buffer_lock);
	delete_sem(device->dma_buffer_owner);

#ifdef USE_FAST_LOG
	if (device->log != NULL)
		fast_log->stop_log(device->log);
#endif

	free(device);
}


/**	copy string src without trailing zero to dst and remove trailing 
 *	spaces size of dst is dst_size, size of src is dst_size-1
 */

static void 
beautify_string(char *dst, char *src, int dst_size)
{
	int i;

	memcpy(dst, src, dst_size - 1);

	for (i = dst_size - 2; i >= 0; --i) {
		if (dst[i] != ' ')
			break;
	}

	dst[i + 1] = 0;
}


/** register new device */

status_t
scsi_register_device(scsi_bus_info *bus, uchar target_id,
	uchar target_lun, scsi_res_inquiry *inquiry_data)
{
	bool is_atapi, manual_autosense;
	uint32 orig_max_blocks, max_blocks;

	SHOW_FLOW0( 3, "" );

	// ask for restrictions	
	bus->interface->get_restrictions(bus->sim_cookie, 
		target_id, &is_atapi, &manual_autosense, &max_blocks);

	// find maximum transfer blocks
	// set default value to max (need something like ULONG_MAX here)
	orig_max_blocks = ~0;		
	pnp->get_attr_uint32(bus->node, B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, &orig_max_blocks, true);

	max_blocks = min(max_blocks, orig_max_blocks);

	{
		char vendor_ident[sizeof( inquiry_data->vendor_ident ) + 1];
		char product_ident[sizeof( inquiry_data->product_ident ) + 1];
		char product_rev[sizeof( inquiry_data->product_rev ) + 1];
		device_attr attrs[] = {
			// info about driver
			{ B_DRIVER_MODULE, B_STRING_TYPE, { string: SCSI_DEVICE_MODULE_NAME }},

			// connection
			{ SCSI_DEVICE_TARGET_ID_ITEM, B_UINT8_TYPE, { ui8: target_id }},
			{ SCSI_DEVICE_TARGET_LUN_ITEM, B_UINT8_TYPE, { ui8: target_lun }},
			{ PNP_DRIVER_CONNECTION, B_STRING_TYPE, { string:
				"target: %"SCSI_DEVICE_TARGET_ID_ITEM
				"%, lun: %"SCSI_DEVICE_TARGET_LUN_ITEM"%" }},

			// inquiry data (used for both identification and information)
			{ SCSI_DEVICE_INQUIRY_ITEM, B_RAW_TYPE, 
				{ raw: { inquiry_data, sizeof( *inquiry_data ) }}},

			// some more info for driver loading
			{ SCSI_DEVICE_TYPE_ITEM, B_UINT8_TYPE, { ui8: inquiry_data->device_type }},
			{ SCSI_DEVICE_VENDOR_ITEM, B_STRING_TYPE, { string: vendor_ident }},
			{ SCSI_DEVICE_PRODUCT_ITEM, B_STRING_TYPE, { string: product_ident }},
			{ SCSI_DEVICE_REVISION_ITEM, B_STRING_TYPE, { string: product_rev }},

			// description of peripheral drivers
			{ B_DRIVER_BUS, B_STRING_TYPE, { string: "scsi" }},
			// ToDo: mapping is missing

			// extra restriction of maximum number of blocks per transfer
			{ B_BLOCK_DEVICE_MAX_BLOCKS_ITEM, B_UINT32_TYPE, { ui32: max_blocks }},

			// atapi emulation
			{ SCSI_DEVICE_IS_ATAPI_ITEM, B_UINT8_TYPE, { ui8: is_atapi }},
			// manual autosense
			{ SCSI_DEVICE_MANUAL_AUTOSENSE_ITEM, B_UINT8_TYPE, { ui8: manual_autosense }},
			{ NULL }
		};
		device_node_handle node;
		status_t res;

		beautify_string(vendor_ident, inquiry_data->vendor_ident, sizeof(vendor_ident));
		beautify_string(product_ident, inquiry_data->product_ident, sizeof(product_ident));
		beautify_string(product_rev, inquiry_data->product_rev, sizeof(product_rev));

		res = pnp->register_device(bus->node, attrs, NULL, &node);
		if (res < 0)
			return res;
	}

	return B_OK;
}


// create data structure for a device
static scsi_device_info *
scsi_create_device(device_node_handle node, scsi_bus_info *bus,
	int target_id, int target_lun)
{
	scsi_device_info *device;

	SHOW_FLOW0( 3, "" );

	device = (scsi_device_info *)malloc(sizeof(*device));
	if (device == NULL)
		return NULL;

	memset(device, 0, sizeof(*device));

	device->lock_count = device->blocked[0] = device->blocked[1] = 0;
	device->sim_overflow = 0;
	device->queued_reqs = NULL;
	device->bus = bus;
	device->target_id = target_id;
	device->target_lun = target_lun;
	device->valid = true;
	device->node = node;

	scsi_dma_buffer_init(&device->dma_buffer);

	if (INIT_BEN(&device->dma_buffer_lock, "dma_buffer") < 0)
		goto err;

	device->dma_buffer_owner = create_sem(1, "dma_buffer");
	if (device->dma_buffer_owner < 0)
		goto err2;

	register_kernel_daemon(scsi_dma_buffer_daemon, device, 5 * 10);

	return device;

err2:
	DELETE_BEN(&device->dma_buffer_lock);
err:
	free(device);
	return NULL;
}


/**	prepare autosense request.
 *	this cannot be done on demand but during init as we may 
 *	have run out of ccbs when we need it
 */

static status_t 
scsi_create_autosense_request(scsi_device_info *device)
{
	scsi_ccb *request;
	char *buffer;
	scsi_cmd_request_sense *cmd;
	size_t total_size;

	SHOW_FLOW0( 3, "" );

	device->auto_sense_request = request = scsi_alloc_ccb(device);
	if (device->auto_sense_request == NULL)
		return B_NO_MEMORY;

	total_size = SCSI_MAX_SENSE_SIZE + sizeof(physical_entry);
	total_size = (total_size + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// allocate buffer for space sense data and S/G list
	device->auto_sense_area = create_area("auto_sense",
		(void **)&buffer, B_ANY_KERNEL_ADDRESS, B_PAGE_SIZE, B_FULL_LOCK, 0);
	if (device->auto_sense_area < 0)
		goto err;

	request->data = buffer;
	request->data_len = SCSI_MAX_SENSE_SIZE;
	request->sg_list = (physical_entry *)(buffer + SCSI_MAX_SENSE_SIZE);
	request->sg_cnt = 1;
	
	get_memory_map(buffer, SCSI_MAX_SENSE_SIZE, 
		(physical_entry *)request->sg_list, 1);

	// disable auto-autosense, just in case;
	// make sure no other request overtakes sense request;
	// buffer is/must be DMA safe as we cannot risk trouble with 
	// dynamically allocated DMA buffer
	request->flags = SCSI_DIR_IN | SCSI_DIS_AUTOSENSE | 
		SCSI_ORDERED_QTAG | SCSI_DMA_SAFE;

	cmd = (scsi_cmd_request_sense *)request->cdb;
	request->cdb_len = sizeof(*cmd);

	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_REQUEST_SENSE;	
	cmd->LUN = device->target_lun;
	cmd->alloc_length = SCSI_MAX_SENSE_SIZE;

	return B_OK;

err:
	scsi_free_ccb(request);
	return B_NO_MEMORY;
}


#define SET_BIT(field, bit) field[(bit) >> 3] |= 1 << ((bit) & 7)

static status_t
scsi_init_device(device_node_handle node, void *user_cookie, void **cookie)
{
	device_node_handle parent = pnp->get_parent(node);
	scsi_res_inquiry *inquiry_data = NULL;
	uint8 target_id, target_lun, path_id;
	scsi_bus_info *bus;
	scsi_device_info *device;
	status_t res;
	driver_module_info *bus_interface;
	size_t inquiry_data_len;
	uint8 is_atapi, manual_autosense;

	SHOW_FLOW0(3, "");

	if (pnp->get_attr_uint8( node, SCSI_DEVICE_TARGET_ID_ITEM, &target_id, false) != B_OK
		|| pnp->get_attr_uint8( node, SCSI_DEVICE_TARGET_LUN_ITEM, &target_lun, false) != B_OK
		|| pnp->get_attr_uint8( node, SCSI_DEVICE_IS_ATAPI_ITEM, &is_atapi, false) != B_OK
		|| pnp->get_attr_uint8( node, SCSI_DEVICE_MANUAL_AUTOSENSE_ITEM, &manual_autosense, false) != B_OK
		|| pnp->get_attr_raw( node, SCSI_DEVICE_INQUIRY_ITEM,
				(void **)&inquiry_data, &inquiry_data_len, false) != B_OK
		|| inquiry_data_len != sizeof(*inquiry_data)) {
		res = B_ERROR;
		goto err1;
	}

	res = pnp->init_driver(parent, NULL, &bus_interface, 
		(void **)&bus);
	if (res != B_OK)
		goto err1;

	device = scsi_create_device(node, bus, target_id, target_lun);
	if (device == NULL) {
		res = B_NO_MEMORY;
		goto err2;
	}

	// never mind if there is no path - it might be an emulated controller
	path_id = -1;

	pnp->get_attr_uint8(node, SCSI_BUS_PATH_ID_ITEM, &path_id, true);

	sprintf(device->name, "scsi_device %u:%u:%u", path_id, target_id, target_lun);

#ifdef USE_FAST_LOG
	device->log = fast_log->start_log(device->name, scsi_device_events);
	if (device->log == NULL) {
		res = B_NO_MEMORY;
		goto err3;
	}
#endif

	device->inquiry_data = *inquiry_data;

	// save restrictions	
	device->is_atapi = is_atapi;
	device->manual_autosense = manual_autosense;

	// size of device queue must be detected by trial and error, so
	// we start with a really high number and see when the device chokes
	device->total_slots = 4096;

	// disable queuing if bus doesn't support it
	if ((bus->inquiry_data.hba_inquiry & SCSI_PI_TAG_ABLE) == 0)
		device->total_slots = 1;

	// if there is no autosense, disable queuing to make sure autosense is
	// not overtaken by other requests
	if (device->manual_autosense)
		device->total_slots = 1;

	device->left_slots = device->total_slots;

	// get autosense request if required
	if (device->manual_autosense) {
		if (scsi_create_autosense_request(device) != B_OK) {
			res = B_NO_MEMORY;
			goto err3;
		}
	}

	// if this is an ATAPI device, we need an emulation buffer
	if (scsi_init_emulation_buffer(device, SCSI_ATAPI_BUFFER_SIZE) != B_OK) {
		res = B_NO_MEMORY;
		goto err3;
	}

	memset(device->emulation_map, 0, sizeof(device->emulation_map));

	if (device->is_atapi) {
		SET_BIT(device->emulation_map, SCSI_OP_READ_6);
		SET_BIT(device->emulation_map, SCSI_OP_WRITE_6);
		SET_BIT(device->emulation_map, SCSI_OP_MODE_SENSE_6);
		SET_BIT(device->emulation_map, SCSI_OP_MODE_SELECT_6);
		SET_BIT(device->emulation_map, SCSI_OP_INQUIRY);
	}

	free(inquiry_data);

	*cookie = device;
	return B_OK;

err3:
	scsi_free_device(device);
err2:
	pnp->uninit_driver(parent);
err1:
	pnp->put_device_node(parent);
	free(inquiry_data);
	return res;
}


static status_t
scsi_uninit_device(scsi_device_info *device)
{
	device_node_handle parent = pnp->get_parent(device->node);

	SHOW_FLOW0(3, "");

	scsi_free_device(device);

	// must unload parent at last as scsi_free_device access it
	pnp->uninit_driver(parent);
	pnp->put_device_node(parent);

	return B_OK;
}


static void
scsi_device_removed(device_node_handle node, scsi_device_info *device)
{	
	SHOW_FLOW0(3, "");

	if (device == NULL)
		return;

	// this must be atomic as no lock is used
	device->valid = false;
}


/**	get device info; create a temporary one if it's not registered
 *	(used during detection)
 *	on success, scan_lun_lock of bus is hold
 */

status_t
scsi_force_get_device(scsi_bus_info *bus, uchar target_id,
	uchar target_lun, scsi_device_info **res_device)
{
	device_attr attrs[] = {
		{ SCSI_DEVICE_TARGET_ID_ITEM, B_UINT8_TYPE, { ui8: target_id }},
		{ SCSI_DEVICE_TARGET_LUN_ITEM, B_UINT8_TYPE, { ui8: target_lun }},
		{ NULL }
	};
	device_node_handle node;
	status_t res;
	driver_module_info *driver_interface;	
	scsi_device device;

	SHOW_FLOW0(3, "");

	// very important: only one can use a forced device to avoid double detection
	acquire_sem(bus->scan_lun_lock);

	// check whether device registered already
	node = NULL;
	pnp->get_next_child_device(bus->node, &node, attrs);

	SHOW_FLOW(3, "%p", node);

	if (node != NULL) {
		// there is one - get it
		res = pnp->init_driver(node, NULL, &driver_interface, 
			(void **)&device);
		pnp->put_device_node(node);
	} else {
		// device doesn't exist yet - create a temporary one
		device = scsi_create_device(NULL, bus, target_id, target_lun);
		if (device == NULL)
			res = B_NO_MEMORY;
		else
			res = B_OK;
	}

	*res_device = device;

	if (res != B_OK)
		release_sem(bus->scan_lun_lock);

	return res;
}


/**	cleanup device received from scsi_force_get_device
 *	on return, scan_lun_lock of bus is released
 */

void
scsi_put_forced_device(scsi_device_info *device)
{
	scsi_bus_info *bus = device->bus;

	SHOW_FLOW0(3, "");

	if (device->node != NULL)
		// device is registered
		pnp->uninit_driver(device->node);
	else
		// device is temporary
		scsi_free_device(device);

	release_sem(bus->scan_lun_lock);
}


static uchar
scsi_reset_device(scsi_device_info *device)
{
	SHOW_FLOW0(3, "");

	if (device->node == NULL)
		return SCSI_DEV_NOT_THERE;

	return device->bus->interface->reset_device(device->bus->sim_cookie,
		device->target_id, device->target_lun);
}


static status_t
scsi_ioctl(scsi_device_info *device, uint32 op, void *buffer, size_t length)
{
	if (device->bus->interface->ioctl != NULL) {
		return device->bus->interface->ioctl(device->bus->sim_cookie,
			device->target_id, op, buffer, length);
	}

	return B_BAD_VALUE;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		{
			// Link to SCSI bus.
			// SCSI device driver must have SCSI bus loaded, but it calls its functions
			// directly instead via official interface, so this pointer is never read.
			module_info *dummy;
			return get_module(SCSI_BUS_MODULE_NAME, &dummy);
		}
		case B_MODULE_UNINIT:
			return put_module(SCSI_BUS_MODULE_NAME);

		default:
			return B_ERROR;
	}
}


scsi_device_interface scsi_device_module = {
	{
		{
			SCSI_DEVICE_MODULE_NAME,
			0,
			std_ops
		},

		NULL,	// supported devices
		NULL,	// register node
		scsi_init_device,
		(status_t (*)(void *)) scsi_uninit_device,
		(void (*)(device_node_handle, void *)) scsi_device_removed
	},

	scsi_alloc_ccb,
	scsi_free_ccb,

	scsi_async_io,
	scsi_sync_io,
	scsi_abort,
	scsi_reset_device,
	scsi_term_io,
	scsi_ioctl,
};
