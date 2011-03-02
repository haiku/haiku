/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Interface between ide bus manager and scsi bus manager.

	The IDE bus manager has a bit unusual structure as it
	consists of a single level only. In fact it is no bus manager
	in terms of the PnP structure at all but a driver that maps
	one SCSI bus onto one IDE controller.

	This structure does not allow us to publish IDE devices
	as they can be accessed via the SCSI bus node only. Therefore
	we do a full bus scan every time the IDE bus node is loaded.
	The drawback is that a bus rescan must be done indirectly via a
	SCSI bus scan.
*/

#include "ide_internal.h"
#include "ide_sim.h"

#include <scsi_cmds.h>
#include <safemode.h>

#include <string.h>
#include <malloc.h>
#include <stdio.h>

scsi_for_sim_interface *scsi;


static void disconnect_worker(ide_bus_info *bus, void *arg);
static void set_check_condition(ide_qrequest *qrequest);


/** check whether this request can be within device */

static inline bool
is_queuable(ide_device_info *device, scsi_ccb *request)
{
	int opcode = request->cdb[0];

	// XXX disable queuing
	if (!device->CQ_enabled)
		return false;

	// make sure the caller allows queuing
	if ((request->flags & SCSI_ORDERED_QTAG) != 0)
		return false;

	// for atapi, all commands could be queued, but all
	// atapi devices I know don't support queuing anyway
	return opcode == SCSI_OP_READ_6 || opcode == SCSI_OP_WRITE_6
		|| opcode == SCSI_OP_READ_10 || opcode == SCSI_OP_WRITE_10;
}


static void
scan_device(ide_bus_info *bus, int device)
{
	SHOW_FLOW0(4, "");

	// currently, the SCSI bus manager doesn't block the
	// bus when a bus or device scan is issued, so we
	// have to use a SPC for that to be sure no one else
	// is accessing the device or bus concurrently
	schedule_synced_pc(bus, &bus->scan_bus_syncinfo, (void *)device);

	acquire_sem(bus->scan_device_sem);
}


static uchar
sim_scan_bus(ide_bus_info *bus)
{
	int i;

	SHOW_FLOW0(4, "");

	if (bus->disconnected)
		return SCSI_NO_HBA;

	for (i = 0; i < bus->max_devices; ++i) {
		scan_device(bus, i);
	}

	return SCSI_REQ_CMP;
}


static void
sim_set_scsi_bus(ide_bus_info *bus, scsi_bus scsi)
{
	bus->scsi_cookie = scsi;

	// detect devices
	sim_scan_bus(bus);
}


static void
sim_scsi_io(ide_bus_info *bus, scsi_ccb *request)
{
	ide_device_info *device;
	bool queuable;
	ide_qrequest *qrequest;

	SHOW_FLOW(3, "%d:%d", request->target_id, request->target_lun);

	if (bus->disconnected)
		goto err_disconnected;

	// make sure, device is valid
	// I've read that there are ATAPI devices with more then one LUN,
	// but it seems that most (all?) devices ignore LUN, so we have
	// to restrict to LUN 0 to avoid mirror devices
	if (request->target_id >= 2)
		goto err_inv_device;

	device = bus->devices[request->target_id];
	if (device == NULL)
		goto err_inv_device;

	if (request->target_lun > device->last_lun)
		goto err_inv_device;

	queuable = is_queuable(device, request);

	// grab the bus
	ACQUIRE_BEN(&bus->status_report_ben);
	IDE_LOCK(bus);

	if (bus->state != ide_state_idle)
		goto err_bus_busy;

	// bail out if device can't accept further requests
	if (device->free_qrequests == NULL
		|| (device->num_running_reqs > 0 && !queuable))
		goto err_device_busy;

	bus->state = ide_state_accessing;

	IDE_UNLOCK(bus);
	RELEASE_BEN(&bus->status_report_ben);

	// as we own the bus, noone can bother us
	qrequest = device->free_qrequests;
	device->free_qrequests = qrequest->next;

	qrequest->request = request;
	qrequest->queuable = queuable;
	qrequest->running = true;
	qrequest->uses_dma = false;

	++device->num_running_reqs;
	++bus->num_running_reqs;
	bus->active_qrequest = qrequest;

	device->exec_io(device, qrequest);

	return;

err_inv_device:
	SHOW_ERROR(3, "Invalid device %d:%d",
		request->target_id, request->target_lun);

	request->subsys_status = SCSI_SEL_TIMEOUT;
	scsi->finished(request, 1);
	return;

err_bus_busy:
	SHOW_FLOW0(3, "Bus busy");

	IDE_UNLOCK(bus);
	scsi->requeue(request, true);
	RELEASE_BEN(&bus->status_report_ben);
	return;

err_device_busy:
	SHOW_FLOW0(3, "Device busy");

	IDE_UNLOCK(bus);
	scsi->requeue(request, false);
	RELEASE_BEN(&bus->status_report_ben);
	return;

err_disconnected:
	SHOW_ERROR0(3, "No controller anymore");
	request->subsys_status = SCSI_NO_HBA;
	scsi->finished(request, 1);
	return;
}


static uchar
sim_path_inquiry(ide_bus_info *bus, scsi_path_inquiry *info)
{
	const char *controller_name;

	SHOW_FLOW0(4, "");

	if (bus->disconnected)
		return SCSI_NO_HBA;

	info->hba_inquiry = SCSI_PI_TAG_ABLE | SCSI_PI_WIDE_16;

	info->hba_misc = 0;

	memset(info->vuhba_flags, 0, sizeof(info->vuhba_flags));
	// we don't need any of the private data
	info->sim_priv = 0;

	// there is no initiator for IDE, but SCSI needs it for scanning
	info->initiator_id = 2;
	// there's no controller limit, so set it higher then the maximum
	// number of queued requests, which is 32 per device * 2 devices
	info->hba_queue_size = 65;

	strncpy(info->sim_vid, "Haiku", SCSI_SIM_ID);

	if (pnp->get_attr_string(bus->node, SCSI_DESCRIPTION_CONTROLLER_NAME,
			&controller_name, true) == B_OK) {
		strlcpy(info->hba_vid, controller_name, SCSI_HBA_ID);
	} else
		strlcpy(info->hba_vid, "", SCSI_HBA_ID);

	strlcpy(info->controller_family, "IDE", SCSI_FAM_ID);
	strlcpy(info->controller_type, "IDE", SCSI_TYPE_ID);

	SHOW_FLOW0(4, "done");

	return SCSI_REQ_CMP;
}


static uchar
sim_abort(ide_bus_info *bus, scsi_ccb *ccb_to_abort)
{
	// we cannot abort specific commands, so just ignore
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_CMP;
}


static uchar
sim_term_io(ide_bus_info *bus, scsi_ccb *ccb_to_abort)
{
	// we cannot terminate commands, so just ignore
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_CMP;
}


static uchar
sim_reset_bus(ide_bus_info *bus)
{
	// no, we don't do that
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_INVALID;
}


static uchar
sim_reset_device(ide_bus_info *bus, uchar target_id, uchar target_lun)
{
	// xxx to do
	if (bus->disconnected)
		return SCSI_NO_HBA;

	return SCSI_REQ_INVALID;
}


/** fill sense buffer according to device sense */

void
create_sense(ide_device_info *device, scsi_sense *sense)
{
	memset(sense, 0, sizeof(*sense));

	sense->error_code = SCSIS_CURR_ERROR;
	sense->sense_key = decode_sense_key(device->combined_sense);
	sense->add_sense_length = sizeof(*sense) - 7;
	sense->asc = decode_sense_asc(device->combined_sense);
	sense->ascq = decode_sense_ascq(device->combined_sense);
	sense->sense_key_spec.raw.SKSV = 0;	// no additional info
}


/** finish command, updating sense of device and request, and release bus */

void
finish_checksense(ide_qrequest *qrequest)
{
	SHOW_FLOW(3, "%p, subsys_status=%d, sense=%x",
		qrequest->request,
		qrequest->request->subsys_status,
		(int)qrequest->device->new_combined_sense);

	qrequest->request->subsys_status = qrequest->device->subsys_status;

	if (qrequest->request->subsys_status == SCSI_REQ_CMP) {
		// device or emulation code completed command
		qrequest->device->combined_sense = qrequest->device->new_combined_sense;

		// if emulation code detected error, set CHECK CONDITION
		if (qrequest->device->combined_sense)
			set_check_condition(qrequest);
	}

	finish_request(qrequest, false);
}


/**	finish request and release bus
 *	resubmit - true, if request should be resubmitted by XPT
 */

void
finish_request(ide_qrequest *qrequest, bool resubmit)
{
	ide_device_info *device = qrequest->device;
	ide_bus_info *bus = device->bus;
	scsi_ccb *request;
	uint num_running;

	SHOW_FLOW0(3, "");

	// save request first, as qrequest can be reused as soon as
	// access_finished is called!
	request = qrequest->request;

	qrequest->running = false;
	qrequest->next = device->free_qrequests;
	device->free_qrequests = qrequest;

	// num_running is not really correct as the XPT is interested
	// in the number of concurrent requests when it was *started* !
	num_running = device->num_running_reqs--;
	--bus->num_running_reqs;

	// paranoia
	bus->active_qrequest = NULL;

	// release bus, handling service requests;
	// TBD:
	// if we really handle a service request, the finished command
	// is delayed unnecessarily, but if we tell the XPT about the finished
	// command first, it will instantly try to pass us another
	// request to handle, which we will refuse as the bus is still
	// locked; this really has to be improved
	access_finished(bus, device);

	ACQUIRE_BEN(&bus->status_report_ben);

	if (resubmit)
		scsi->resubmit(request);
	else
		scsi->finished(request, num_running);

	RELEASE_BEN(&bus->status_report_ben);
}


/**	set CHECK CONDITION of device and perform auto-sense if requested.
 *	(don't move it before finish_request - we don't want to inline
 *	it as it's on the rarely used error path)
 */

static void
set_check_condition(ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	ide_device_info *device = qrequest->device;

	SHOW_FLOW0(3, "");

	request->subsys_status = SCSI_REQ_CMP_ERR;
	request->device_status = SCSI_STATUS_CHECK_CONDITION;

	// copy sense only if caller requested it
	if ((request->flags & SCSI_DIS_AUTOSENSE) == 0) {
		scsi_sense sense;
		int sense_len;

		SHOW_FLOW0(3, "autosense");

		// we cannot copy sense directly as sense buffer may be too small
		create_sense(device, &sense);

		sense_len = min(SCSI_MAX_SENSE_SIZE, sizeof(sense));

		memcpy(request->sense, &sense, sense_len);
		request->sense_resid = SCSI_MAX_SENSE_SIZE - sense_len;
		request->subsys_status |= SCSI_AUTOSNS_VALID;

		// device sense gets reset once it's read
		device->combined_sense = 0;
	}
}


void
finish_retry(ide_qrequest *qrequest)
{
	qrequest->device->combined_sense = 0;
	finish_request(qrequest, true);
}


/**	finish request and abort pending requests of the device
 *	(to be called when the request failed and thus messed up the queue)
 */

void
finish_reset_queue(ide_qrequest *qrequest)
{
	ide_bus_info *bus = qrequest->device->bus;

	// don't remove block_bus!!!
	// during finish_checksense, the bus is released, so
	// the SCSI bus manager could send us further commands
	scsi->block_bus(bus->scsi_cookie);

	finish_checksense(qrequest);
	send_abort_queue(qrequest->device);

	scsi->unblock_bus(bus->scsi_cookie);
}


/**	finish request, but don't release bus
 *	if resubmit is true, the request will be resubmitted
 */

static void
finish_norelease(ide_qrequest *qrequest, bool resubmit)
{
	ide_device_info *device = qrequest->device;
	ide_bus_info *bus = device->bus;
	uint num_requests;

	qrequest->running = false;
	qrequest->next = device->free_qrequests;
	device->free_qrequests = qrequest;

	num_requests = device->num_running_reqs++;
	--bus->num_running_reqs;

	if (bus->active_qrequest == qrequest)
		bus->active_qrequest = NULL;

	ACQUIRE_BEN(&bus->status_report_ben);

	if (resubmit)
		scsi->resubmit(qrequest->request);
	else
		scsi->finished(qrequest->request, num_requests);

	RELEASE_BEN(&bus->status_report_ben);
}


/**	finish all queued requests but <ignore> of the device;
 *	set resubmit, if requests are to be resubmitted by xpt
 */

void
finish_all_requests(ide_device_info *device, ide_qrequest *ignore,
	int subsys_status, bool resubmit)
{
	int i;

	if (device == NULL)
		return;

	// we only have to block the device, but for CD changers we
	// have to block all LUNS of the device (and we neither know
	// their device handle nor which exist at all), so block
	// the entire bus instead (it won't take that long anyway)
	scsi->block_bus(device->bus->scsi_cookie);

	for (i = 0; i < device->queue_depth; ++i) {
		ide_qrequest *qrequest = &device->qreq_array[i];

		if (qrequest->running && qrequest != ignore) {
			qrequest->request->subsys_status = subsys_status;
			finish_norelease(qrequest, resubmit);
		}
	}

	scsi->unblock_bus(device->bus->scsi_cookie);
}


static status_t
ide_sim_init_bus(device_node *node, void **cookie)
{
	ide_bus_info *bus;
	bool dmaDisabled = false;
	status_t status;

	SHOW_FLOW0(3, "");

	// first prepare the info structure
	bus = (ide_bus_info *)malloc(sizeof(*bus));
	if (bus == NULL)
		return B_NO_MEMORY;

	memset(bus, 0, sizeof(*bus));
	bus->node = node;
	B_INITIALIZE_SPINLOCK(&bus->lock);
	bus->num_running_reqs = 0;
	bus->active_qrequest = NULL;
	bus->disconnected = false;

	{
		int32 channel_id = -1;

		pnp->get_attr_uint32(node, IDE_CHANNEL_ID_ITEM, (uint32 *)&channel_id, true);

		sprintf(bus->name, "ide_bus %d", (int)channel_id);
	}

	init_synced_pc(&bus->scan_bus_syncinfo, scan_device_worker);
	init_synced_pc(&bus->disconnect_syncinfo, disconnect_worker);

	bus->state = ide_state_idle;
	bus->timer.bus = bus;
	bus->synced_pc_list = NULL;

	if ((status = scsi->alloc_dpc(&bus->irq_dpc)) < B_OK)
		goto err1;

	bus->active_device = NULL;
	bus->sync_wait_sem = create_sem(0, "ide_sync_wait");
	if (bus->sync_wait_sem < 0) {
		status = bus->sync_wait_sem;
		goto err2;
	}

	bus->devices[0] = bus->devices[1] = NULL;

	bus->scan_device_sem = create_sem(0, "ide_scan_finished");
	if (bus->scan_device_sem < 0) {
		status = bus->scan_device_sem;
		goto err3;
	}

	status = INIT_BEN(&bus->status_report_ben, "ide_status_report");
	if (status < B_OK)
		goto err4;

	{
		// check if safemode settings disable DMA
		void *settings = load_driver_settings(B_SAFEMODE_DRIVER_SETTINGS);
		if (settings != NULL) {
			dmaDisabled = get_driver_boolean_parameter(settings, B_SAFEMODE_DISABLE_IDE_DMA,
				false, false);
			unload_driver_settings(settings);
		}
	}

	bus->first_device = NULL;

	// read restrictions of controller

	if (pnp->get_attr_uint8(node, IDE_CONTROLLER_MAX_DEVICES_ITEM,
			&bus->max_devices, true) != B_OK) {
		// per default, 2 devices are supported per node
		bus->max_devices = 2;
	}

	bus->max_devices = min(bus->max_devices, 2);

	if (dmaDisabled
		|| pnp->get_attr_uint8(node, IDE_CONTROLLER_CAN_DMA_ITEM, &bus->can_DMA, true) != B_OK) {
		// per default, no dma support
		bus->can_DMA = false;
	}

	SHOW_FLOW(2, "can_dma: %d", bus->can_DMA);

	if (bus->can_DMA) {
		if (pnp->get_attr_uint8(node, IDE_CONTROLLER_CAN_CQ_ITEM, &bus->can_CQ, true) != B_OK) {
			// per default, command queuing is supported unless the driver
			// reports problems (queuing should be transparent to
			// controller, but for sure there is some buggy, over-optimizing
			// controller out there)
			bus->can_CQ = true;
		}
	} else {
		// I am not sure if it's a problem of the driver or the drive (probably the
		// former), but we're generally disabling command queueing in case of PIO
		// transfers. Since those should be rare on a real system (as is CQ support
		// in the drive), it's not really worth investigating, though.
		bus->can_CQ = false;
	}

	{
		device_node *parent = pnp->get_parent_node(node);
		pnp->get_driver(parent, (driver_module_info **)&bus->controller,
			(void **)&bus->channel_cookie);

		bus->controller->set_channel(bus->channel_cookie, bus);
		pnp->put_node(parent);
	}

	*cookie = bus;
	return B_OK;

err4:
	delete_sem(bus->scan_device_sem);
err3:
	delete_sem(bus->sync_wait_sem);
err2:
	scsi->free_dpc(bus->irq_dpc);
err1:
	uninit_synced_pc(&bus->scan_bus_syncinfo);
	uninit_synced_pc(&bus->disconnect_syncinfo);
	free(bus);

	return status;
}


static status_t
ide_sim_uninit_bus(ide_bus_info *bus)
{
	DELETE_BEN(&bus->status_report_ben);
	delete_sem(bus->scan_device_sem);
	delete_sem(bus->sync_wait_sem);
	scsi->free_dpc(bus->irq_dpc);
	uninit_synced_pc(&bus->scan_bus_syncinfo);
	uninit_synced_pc(&bus->disconnect_syncinfo);

	free(bus);

	return B_OK;
}


// abort all running requests with SCSI_NO_HBA; finally, unblock bus
static void
disconnect_worker(ide_bus_info *bus, void *arg)
{
	int i;

	for (i = 0; i < bus->max_devices; ++i) {
		if (bus->devices[i])
			// is this the proper error code?
			finish_all_requests(bus->devices[i], NULL, SCSI_NO_HBA, false);
	}

	scsi->unblock_bus(bus->scsi_cookie);
}


static void
ide_sim_bus_removed(ide_bus_info *bus)
{
	// XPT must not issue further commands
	scsi->block_bus(bus->scsi_cookie);
	// make sure, we refuse all new commands
	bus->disconnected = true;
	// abort all running commands with SCSI_NO_HBA
	// (the scheduled function also unblocks the bus when finished)
	schedule_synced_pc(bus, &bus->disconnect_syncinfo, NULL);
}


static void
ide_sim_get_restrictions(ide_bus_info *bus, uchar target_id,
	bool *is_atapi, bool *no_autosense, uint32 *max_blocks)
{
	ide_device_info *device = bus->devices[target_id];

	// we declare even ATA devices as ATAPI so we have to emulate fewer
	// commands
	*is_atapi = true;

	// we emulate autosense for ATA devices
	*no_autosense = false;

	if (device != NULL && device->is_atapi) {
		// we don't support native autosense for ATAPI devices
		*no_autosense = true;
	}

	*max_blocks = 255;

	if (device->is_atapi) {
		if (strncmp(device->infoblock.model_number, "IOMEGA  ZIP 100       ATAPI",
		 		strlen("IOMEGA  ZIP 100       ATAPI")) == 0
		 	|| strncmp( device->infoblock.model_number, "IOMEGA  Clik!",
		 		strlen( "IOMEGA  Clik!")) == 0) {
			SHOW_ERROR0(2, "Found buggy ZIP/Clik! drive - restricting transmission size");
			*max_blocks = 64;
		}
	}
}


static status_t
ide_sim_ioctl(ide_bus_info *bus, uint8 targetID, uint32 op, void *buffer, size_t length)
{
	ide_device_info *device = bus->devices[targetID];

	// We currently only support IDE_GET_INFO_BLOCK
	switch (op) {
		case IDE_GET_INFO_BLOCK:
			// we already have the info block, just copy it
			memcpy(buffer, &device->infoblock,
				min(sizeof(device->infoblock), length));
			return B_OK;

		case IDE_GET_STATUS:
		{
			// TODO: have our own structure and fill it with some useful stuff
			ide_status status;
			if (device->DMA_enabled)
				status.dma_status = 1;
			else if (device->DMA_supported) {
				if (device->DMA_failures > 0)
					status.dma_status = 6;
				else if (device->bus->can_DMA)
					status.dma_status = 2;
				else
					status.dma_status = 4;
			} else
				status.dma_status = 2;

			status.pio_mode = 0;
			status.dma_mode = get_device_dma_mode(device);

			memcpy(buffer, &status, min(sizeof(status), length));
			return B_OK;
		}
	}

	return B_DEV_INVALID_IOCTL;
}


static status_t
std_ops(int32 op, ...)
{
	switch (op) {
		case B_MODULE_INIT:
		case B_MODULE_UNINIT:
			return B_OK;

		default:
			return B_ERROR;
	}
}


module_dependency module_dependencies[] = {
	{ SCSI_FOR_SIM_MODULE_NAME, (module_info **)&scsi },
	{ B_DEVICE_MANAGER_MODULE_NAME, (module_info **)&pnp },
	{}
};


scsi_sim_interface ide_sim_module = {
	{
		{
			IDE_SIM_MODULE_NAME,
			0,
			std_ops,
		},

		NULL,	// supported devices
		NULL,	// register node
		(status_t (*)(device_node *, void **))		ide_sim_init_bus,
		(void (*)(void *)) 							ide_sim_uninit_bus,
		NULL,	// register child devices
		NULL,	// rescan
		(void (*)(void *))							ide_sim_bus_removed,
		NULL,	// suspend
		NULL	// resume
	},

	(void (*)(scsi_sim_cookie, scsi_bus))			sim_set_scsi_bus,
	(void (*)(scsi_sim_cookie, scsi_ccb *))			sim_scsi_io,
	(uchar (*)(scsi_sim_cookie, scsi_ccb *))		sim_abort,
	(uchar (*)(scsi_sim_cookie, uchar, uchar)) 		sim_reset_device,
	(uchar (*)(scsi_sim_cookie, scsi_ccb *))		sim_term_io,

	(uchar (*)(scsi_sim_cookie, scsi_path_inquiry *))sim_path_inquiry,
	(uchar (*)(scsi_sim_cookie))					sim_scan_bus,
	(uchar (*)(scsi_sim_cookie))					sim_reset_bus,

	(void (*)(scsi_sim_cookie, uchar,
		bool*, bool *, uint32 *))					ide_sim_get_restrictions,

	(status_t (*)(scsi_sim_cookie, uint8, uint32, void *, size_t))ide_sim_ioctl,
};
