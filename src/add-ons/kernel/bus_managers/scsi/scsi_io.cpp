/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2004, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//!	Part of Open SCSI bus manager


#include "scsi_internal.h"
#include "queuing.h"

#include <string.h>

#include <algorithm>


/** put request back in queue because of device/bus overflow */

void
scsi_requeue_request(scsi_ccb *request, bool bus_overflow)
{
	scsi_bus_info *bus = request->bus;
	scsi_device_info *device = request->device;
	bool was_servicable, start_retry;

	SHOW_FLOW0(3, "");

	if (request->state != SCSI_STATE_SENT) {
		panic("Unsent ccb was request to requeue\n");
		return;
	}

	request->state = SCSI_STATE_QUEUED;

	ACQUIRE_BEN(&bus->mutex);

	was_servicable = scsi_can_service_bus(bus);

	if (bus->left_slots++ == 0)
		scsi_unblock_bus_noresume(bus, false);

	if (device->left_slots++ == 0 || request->ordered)
		scsi_unblock_device_noresume(device, false);

	// make sure it's the next request for this device
	scsi_add_req_queue_first(request);

	if (bus_overflow) {
		// bus has overflown
		scsi_set_bus_overflow(bus);
		// add device to queue as last - other devices may be waiting already
		scsi_add_device_queue_last(device);
		// don't change device overflow condition as the device has never seen
		// this request
	} else {
		// device has overflown
		scsi_set_device_overflow(device);
		scsi_remove_device_queue(device);
		// either, the device has refused the request, i.e. it was transmitted
		// over the bus - in this case, the bus cannot be overloaded anymore;
		// or, the driver detected that the device can not be able to process
		// further requests, because the driver knows its maximum queue depth
		// or something - in this case, the bus state hasn't changed, but the
		// driver will tell us about any overflow when we submit the next
		// request, so the overflow state will be fixed automatically
		scsi_clear_bus_overflow(bus);
	}

	start_retry = !was_servicable && scsi_can_service_bus(bus);

	RELEASE_BEN(&bus->mutex);

	// submit requests to other devices in case bus was overloaded
	if (start_retry)
		release_sem_etc(bus->start_service, 1, 0/*B_DO_NOT_RESCHEDULE*/);
}


/** restart request ASAP because something went wrong */

void
scsi_resubmit_request(scsi_ccb *request)
{
	scsi_bus_info *bus = request->bus;
	scsi_device_info *device = request->device;
	bool was_servicable, start_retry;

	SHOW_FLOW0(3, "");

	if (request->state != SCSI_STATE_SENT) {
		panic("Unsent ccb was asked to get resubmitted\n");
		return;
	}

	request->state = SCSI_STATE_QUEUED;

	ACQUIRE_BEN(&bus->mutex);

	was_servicable = scsi_can_service_bus(bus);

	if (bus->left_slots++ == 0)
		scsi_unblock_bus_noresume(bus, false);

	if (device->left_slots++ == 0 || request->ordered)
		scsi_unblock_device_noresume(device, false);

	// if SIM reported overflow of device/bus, this should (hopefully) be over now
	scsi_clear_device_overflow(device);
	scsi_clear_bus_overflow(bus);

	// we don't want to let anyone overtake this request
	request->ordered = true;

	// make it the next request submitted to SIM for this device
	scsi_add_req_queue_first(request);

	// if device is not blocked (anymore) add it to waiting list of bus
	if (device->lock_count == 0) {
		scsi_add_device_queue_first(device);
		// as previous line does nothing if already queued, we force device
		// to be the next one to get handled
		bus->waiting_devices = device;
	}

	start_retry = !was_servicable && scsi_can_service_bus(bus);

	RELEASE_BEN(&bus->mutex);

	// let the service thread do the resubmit
	if (start_retry)
		release_sem_etc(bus->start_service, 1, 0/*B_DO_NOT_RESCHEDULE*/);
}


/** submit autosense for request */

static void
submit_autosense(scsi_ccb *request)
{
	scsi_device_info *device = request->device;

	//snooze(1000000);

	SHOW_FLOW0(3, "sending autosense");
	// we cannot use scsi_scsi_io but must insert it brute-force

	// give SIM a well-defined first state
	// WARNING: this is a short version of scsi_async_io, so if
	// you change something there, do it here as well!

	// no DMA buffer (we made sure that the data buffer fulfills all
	// limitations)
	request->buffered = false;
	// don't let any request bypass us
	request->ordered = true;
	// initial SIM state for this request
	request->sim_state = 0;

	device->auto_sense_originator = request;

	// make it next request to process
	scsi_add_queued_request_first(device->auto_sense_request);
}


/** finish special auto-sense request */

static void
finish_autosense(scsi_device_info *device)
{
	scsi_ccb *orig_request = device->auto_sense_originator;
	scsi_ccb *request = device->auto_sense_request;

	SHOW_FLOW0(3, "");

	if (request->subsys_status == SCSI_REQ_CMP) {
		int sense_len;

		// we got sense data -> copy it to sense buffer
		sense_len = std::min((uint32)SCSI_MAX_SENSE_SIZE,
			request->data_length - request->data_resid);

		SHOW_FLOW(3, "Got sense: %d bytes", sense_len);

		memcpy(orig_request->sense, request->data, sense_len);

		orig_request->sense_resid = SCSI_MAX_SENSE_SIZE - sense_len;
		orig_request->subsys_status |= SCSI_AUTOSNS_VALID;
	} else {
		// failed to get sense
		orig_request->subsys_status = SCSI_AUTOSENSE_FAIL;
	}

	// inform peripheral driver
	release_sem_etc(orig_request->completion_sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);
}


/** device refused request because command queue is full */

static void
scsi_device_queue_overflow(scsi_ccb *request, uint num_requests)
{
	scsi_bus_info *bus = request->bus;
	scsi_device_info *device = request->device;
	int diff_max_slots;

	// set maximum number of concurrent requests to number of
	// requests running when QUEUE FULL condition occurred - 1
	// (the "1" is the refused request)
	--num_requests;

	// at least one request at once must be possible
	if (num_requests < 1)
		num_requests = 1;

	SHOW_INFO(2, "Restricting device queue to %d requests", num_requests);

	// update slot count
	ACQUIRE_BEN(&bus->mutex);

	diff_max_slots = device->total_slots - num_requests;
	device->total_slots = num_requests;
	device->left_slots -= diff_max_slots;

	RELEASE_BEN(&bus->mutex);

	// requeue request, blocking further device requests
	scsi_requeue_request(request, false);
}


/** finish scsi request */

void
scsi_request_finished(scsi_ccb *request, uint num_requests)
{
	scsi_device_info *device = request->device;
	scsi_bus_info *bus = request->bus;
	bool was_servicable, start_service, do_autosense;

	SHOW_FLOW(3, "%p", request);

	if (request->state != SCSI_STATE_SENT) {
		panic("Unsent ccb %p was reported as done\n", request);
		return;
	}

	if (request->subsys_status == SCSI_REQ_INPROG) {
		panic("ccb %p with status \"Request in Progress\" was reported as done\n",
			request);
		return;
	}

	// check for queue overflow reported by device
	if (request->subsys_status == SCSI_REQ_CMP_ERR
		&& request->device_status == SCSI_STATUS_QUEUE_FULL) {
		scsi_device_queue_overflow(request, num_requests);
		return;
	}

	request->state = SCSI_STATE_FINISHED;

	ACQUIRE_BEN(&bus->mutex);

	was_servicable = scsi_can_service_bus(bus);

	// do pseudo-autosense if device doesn't support it and
	// device reported a check condition state and auto-sense haven't
	// been retrieved by SIM
	// (last test is implicit as SIM adds SCSI_AUTOSNS_VALID to subsys_status)
	do_autosense = device->manual_autosense
		&& (request->flags & SCSI_DIS_AUTOSENSE) == 0
		&& request->subsys_status == SCSI_REQ_CMP_ERR
		&& request->device_status == SCSI_STATUS_CHECK_CONDITION;

	if (request->subsys_status != SCSI_REQ_CMP) {
		SHOW_FLOW(3, "subsys=%x, device=%x, flags=%x, manual_auto_sense=%d",
			request->subsys_status, request->device_status, (int)request->flags,
			device->manual_autosense);
	}

	if (do_autosense) {
		// queue auto-sense request after checking was_servicable but before
		// releasing locks so no other request overtakes auto-sense
		submit_autosense(request);
	}

	if (bus->left_slots++ == 0)
		scsi_unblock_bus_noresume(bus, false);

	if (device->left_slots++ == 0 || request->ordered)
		scsi_unblock_device_noresume(device, false);

	// if SIM reported overflow of device/bus, this should (hopefully) be over now
	scsi_clear_device_overflow(device);
	scsi_clear_bus_overflow(bus);

	// if device is not blocked (anymore) and has pending requests,
	// add it to waiting list of bus
	if (device->lock_count == 0 && device->queued_reqs != NULL)
		scsi_add_device_queue_last(device);

	start_service = !was_servicable && scsi_can_service_bus(bus);

	RELEASE_BEN(&bus->mutex);

	// tell service thread to submit new requests to SIM
	// (do this ASAP to keep bus/device busy)
	if (start_service)
		release_sem_etc(bus->start_service, 1, 0/*B_DO_NOT_RESCHEDULE*/);

	if (request->emulated)
		scsi_finish_emulation(request);

	// copy data from buffer and release it
	if (request->buffered)
		scsi_release_dma_buffer(request);

	// special treatment for finished auto-sense
	if (request == device->auto_sense_request)
		finish_autosense(device);
	else {
		// tell peripheral driver about completion
		if (!do_autosense)
			release_sem_etc(request->completion_sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);
	}
}


/**	check whether request can be executed right now, enqueuing it if not,
 *	return: true if request can be executed
 *	side effect: updates device->last_sort
 */

static inline bool
scsi_check_enqueue_request(scsi_ccb *request)
{
	scsi_bus_info *bus = request->bus;
	scsi_device_info *device = request->device;
	bool execute;

	ACQUIRE_BEN(&bus->mutex);

	// if device/bus is locked, or there are waiting requests
	// or waiting devices (last condition makes sure we don't overtake
	// requests that got queued because bus was full)
	if (device->lock_count > 0 || device->queued_reqs != NULL
		|| bus->lock_count > 0 || bus->waiting_devices != NULL) {
		SHOW_FLOW0(3, "bus/device is currently locked");
		scsi_add_queued_request(request);
		execute = false;
	} else {
		// if bus is saturated, block it
		if (--bus->left_slots == 0) {
			SHOW_FLOW0(3, "bus is saturated, blocking further requests");
			scsi_block_bus_nolock(bus, false);
		}

		// if device saturated or blocking request, block device
		if (--device->left_slots == 0 || request->ordered) {
			SHOW_FLOW0( 3, "device is saturated/blocked by requests, blocking further requests" );
			scsi_block_device_nolock(device, false);
		}

		if (request->sort >= 0) {
			device->last_sort = request->sort;
			SHOW_FLOW(1, "%" B_PRId64, device->last_sort);
		}

		execute = true;
	}

	RELEASE_BEN(&bus->mutex);

	return execute;
}


// size of SCSI command according to function group
int func_group_len[8] = {
	6, 10, 10, 0, 16, 12, 0, 0
};


/** execute scsi command asynchronously */

void
scsi_async_io(scsi_ccb *request)
{
	scsi_bus_info *bus = request->bus;

	//SHOW_FLOW( 0, "path_id=%d", bus->path_id );

	//snooze( 1000000 );

	// do some sanity tests first
	if (request->state != SCSI_STATE_FINISHED)
		panic("Passed ccb to scsi_action that isn't ready (state = %d)\n", request->state);

	if (request->cdb_length < func_group_len[request->cdb[0] >> 5]) {
		SHOW_ERROR(3, "invalid command len (%d instead of %d)",
			request->cdb_length, func_group_len[request->cdb[0] >> 5]);

		request->subsys_status = SCSI_REQ_INVALID;
		goto err;
	}

	if (!request->device->valid) {
		SHOW_ERROR0( 3, "device got removed" );

		// device got removed meanwhile
		request->subsys_status = SCSI_DEV_NOT_THERE;
		goto err;
	}

	if ((request->flags & SCSI_DIR_MASK) != SCSI_DIR_NONE
		&& request->sg_list == NULL && request->data_length > 0) {
		SHOW_ERROR( 3, "Asynchronous SCSI I/O requires S/G list (data is %d bytes)",
			(int)request->data_length );
		request->subsys_status = SCSI_DATA_RUN_ERR;
		goto err;
	}

	request->buffered = request->emulated = 0;

	// make data DMA safe
	// (S/G list must be created first to be able to verify DMA restrictions)
	if ((request->flags & SCSI_DMA_SAFE) == 0 && request->data_length > 0) {
		request->buffered = true;
		if (!scsi_get_dma_buffer(request)) {
			SHOW_ERROR0( 3, "cannot create DMA buffer for request - reduce data volume" );

			request->subsys_status = SCSI_DATA_RUN_ERR;
			goto err;
		}
	}

	// emulate command if not supported
	if ((request->device->emulation_map[request->cdb[0] >> 3]
		& (1 << (request->cdb[0] & 7))) != 0) {
		request->emulated = true;

		if (!scsi_start_emulation(request)) {
			SHOW_ERROR(3, "cannot emulate SCSI command 0x%02x", request->cdb[0]);
			goto err2;
		}
	}

	// SCSI-1 uses 3 bits of command packet for LUN
	// SCSI-2 uses identify message, but still needs LUN in command packet
	//        (though it won't fit, as LUNs can be 4 bits wide)
	// SCSI-3 doesn't use command packet for LUN anymore
	// ATAPI uses 3 bits of command packet for LUN

	// currently, we always copy LUN into command packet as a safe bet
	{
		// abuse TUR to find proper spot in command packet for LUN
		scsi_cmd_tur *cmd = (scsi_cmd_tur *)request->cdb;

		cmd->lun = request->device->target_lun;
	}

	request->ordered = (request->flags & SCSI_ORDERED_QTAG) != 0;

	SHOW_FLOW(3, "ordered=%d", request->ordered);

	// give SIM a well-defined first state
	request->sim_state = 0;

	// make sure device/bus is not blocked
	if (!scsi_check_enqueue_request(request))
		return;

	bus = request->bus;

	request->state = SCSI_STATE_SENT;
	bus->interface->scsi_io(bus->sim_cookie, request);
	return;

err2:
	if (request->buffered)
		scsi_release_dma_buffer(request);
err:
	release_sem(request->completion_sem);
	return;
}


/** execute SCSI command synchronously */

void
scsi_sync_io(scsi_ccb *request)
{
	bool tmp_sg = false;

	// create scatter-gather list if required
	if ((request->flags & SCSI_DIR_MASK) != SCSI_DIR_NONE
		&& request->sg_list == NULL && request->data_length > 0) {
		tmp_sg = true;
		if (!create_temp_sg(request)) {
			SHOW_ERROR0( 3, "data is too much fragmented - you should use s/g list" );

			// ToDo: this means too much (fragmented) data
			request->subsys_status = SCSI_DATA_RUN_ERR;
			return;
		}
	}

	scsi_async_io(request);
	acquire_sem(request->completion_sem);

	if (tmp_sg)
		cleanup_tmp_sg(request);
}


uchar
scsi_term_io(scsi_ccb *ccb_to_terminate)
{
	scsi_bus_info *bus = ccb_to_terminate->bus;

	return bus->interface->term_io(bus->sim_cookie, ccb_to_terminate);
}


uchar
scsi_abort(scsi_ccb *req_to_abort)
{
	scsi_bus_info *bus = req_to_abort->bus;

	if (bus == NULL) {
		// checking the validity of the request to abort is a nightmare
		// this is just a beginning
		return SCSI_REQ_INVALID;
	}

	ACQUIRE_BEN(&bus->mutex);

	switch (req_to_abort->state) {
		case SCSI_STATE_FINISHED:
		case SCSI_STATE_SENT:
			RELEASE_BEN(&bus->mutex);
			break;

		case SCSI_STATE_QUEUED: {
			bool was_servicable, start_retry;

			was_servicable = scsi_can_service_bus(bus);

			// remove request from device queue
			scsi_remove_queued_request(req_to_abort);

			start_retry = scsi_can_service_bus(bus) && !was_servicable;

			RELEASE_BEN(&bus->mutex);

			req_to_abort->subsys_status = SCSI_REQ_ABORTED;

			// finish emulation
			if (req_to_abort->emulated)
				scsi_finish_emulation(req_to_abort);

			// release DMA buffer
			if (req_to_abort->buffered)
				scsi_release_dma_buffer(req_to_abort);

			// tell peripheral driver about
			release_sem_etc(req_to_abort->completion_sem, 1, 0/*B_DO_NOT_RESCHEDULE*/);

			if (start_retry)
				release_sem(bus->start_service);

			break;
		}
	}

	return SCSI_REQ_CMP;
}


/** submit pending request (at most one!) */

bool
scsi_check_exec_service(scsi_bus_info *bus)
{
	SHOW_FLOW0(3, "");
	ACQUIRE_BEN(&bus->mutex);

	if (scsi_can_service_bus(bus)) {
		scsi_ccb *request;
		scsi_device_info *device;

		SHOW_FLOW0(3, "servicing bus");

		//snooze( 1000000 );

		// handle devices in round-robin-style
		device = bus->waiting_devices;
		bus->waiting_devices = bus->waiting_devices->waiting_next;

		request = device->queued_reqs;
		scsi_remove_queued_request(request);

		// if bus is saturated, block it
		if (--bus->left_slots == 0) {
			SHOW_FLOW0(3, "bus is saturated, blocking further requests");
			scsi_block_bus_nolock(bus, false);
		}

		// if device saturated or blocking request, block device
		if (--device->left_slots == 0 || request->ordered) {
			SHOW_FLOW0(3, "device is saturated/blocked by requests, blocking further requests");
			scsi_block_device_nolock(device, false);
		}

		if (request->sort >= 0) {
			device->last_sort = request->sort;
			SHOW_FLOW(1, "%" B_PRId64, device->last_sort);
		}

		RELEASE_BEN(&bus->mutex);

		request->state = SCSI_STATE_SENT;
		bus->interface->scsi_io(bus->sim_cookie, request);

		return true;
	}

	RELEASE_BEN(&bus->mutex);

	return false;
}
