/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	Command queuing functions
*/

#include "ide_internal.h"
#include "ide_sim.h"
#include "ide_cmds.h"

#include <string.h>
#include <malloc.h>


// maximum number of errors until command queuing is disabled
#define MAX_CQ_FAILURES 3


/** convert tag to request */

static inline ide_qrequest *
tag2request(ide_device_info *device, int tag)
{
	ide_qrequest *qrequest = &device->qreq_array[tag];

	if (qrequest->running)
		return qrequest;

	return NULL;
}


/**	service device
 *
 *	(expects locked bus and bus in "accessing" state)
 *	returns true if servicing a command (implies having bus unlocked)
 *	returns false on error
 */

static bool
service_device(ide_device_info *device)
{
	ide_qrequest *qrequest;
	int tag;

	SHOW_FLOW0( 3, "Start servicing" );

	// delete timeout first

	// we must unlock bus before cancelling timer: if the timeout has
	// just been fired we have to wait for it, but in turn it waits 
	// for the ide bus -> deadlock
	IDE_UNLOCK(device->bus);

	cancel_timer(&device->reconnect_timer.te);

	// between IDE_UNLOCK and cancel_timer the request may got
	// discarded due to timeout, so it's not a hardware problem
	// if servicing fails

	// further, the device discards the entire queue if anything goes
	// wrong, thus we call send_abort_queue on each error
	// (we could also discard the queue without telling the device,
	//  but we prefer setting the device into a safe state)

	// ask device to continue		
	if (!device_start_service(device, &tag)) {
		send_abort_queue(device);
		goto err;
	}

	SHOW_FLOW0( 3, "device starts service" );

	// get tag of request
	qrequest = tag2request(device, tag);
	if (qrequest == NULL) {
		send_abort_queue(device);
		goto err;
	}

	SHOW_FLOW( 3, "continue request %p with tag %d", qrequest, tag );

	device->bus->active_qrequest = qrequest;

	// from here on, queuing is ATA read/write specific, so you have to
	// modify that if you want to support ATAPI queuing!
	if (check_rw_error(device, qrequest)) {
		// if a read/write error occured, the request really failed
		finish_reset_queue(qrequest);
		goto err;
	}

	// all ATA commands continue with a DMA request	
	if (!prepare_dma(device, qrequest)) {
		// this is effectively impossible: before the command was initially
		// sent, prepare_dma had been called and obviously didn't fail,
		// so why should it fail now?
		device->subsys_status = SCSI_HBA_ERR;
		finish_reset_queue(qrequest);
		goto err;	
	}

	SHOW_FLOW0( 3, "launch DMA" );
	
	start_dma_wait_no_lock(device, qrequest);

	return true;

err:
	// don't start timeout - all requests have been discarded at this point
	IDE_LOCK(device->bus);
	return false;
}


/**	check if some device on bus wants to continue queued requests;
 *
 *	(expects locked bus and bus in "accessing" state)
 *	returns true if servicing a command (implies having bus unlocked)
 *	returns false if nothing to service
 */

bool
try_service(ide_device_info *device)
{
	bool this_device_needs_service;
	ide_device_info *other_device;

	other_device = device->other_device;

	// first check whether current device requests service
	// (the current device is selected anyway, so asking it is fast)
	this_device_needs_service = check_service_req(device);

	// service other device first as it was certainly waiting
	// longer then the current device
	if (other_device != device && check_service_req(other_device)) {
		if (service_device(other_device)) {
			// we handed over control; start timeout for device
			// (see below about fairness)
			if (device->num_running_reqs > 0) {
				if (!device->reconnect_timer_installed) {
					device->reconnect_timer_installed = true;
					add_timer(&device->reconnect_timer.te, reconnect_timeout,
						IDE_RELEASE_TIMEOUT, B_ONE_SHOT_RELATIVE_TIMER);
				}
			}
			return true;
		}
	}

	// service our device second
	if (this_device_needs_service) {
		if (service_device(device))
			return true;
	}

	// if device has pending reqs, start timeout.
	// this may sound strange as we cannot be blamed if the
	// other device blocks us. But: the timeout is delayed until
	// the bus is idle, so once the other device finishes its
	// access, we have a chance of servicing all the pending 
	// commands before the timeout handler is executed
	if (device->num_running_reqs > 0) {
		if (!device->reconnect_timer_installed) {
			device->reconnect_timer_installed = true;
			add_timer(&device->reconnect_timer.te, reconnect_timeout, 
				IDE_RELEASE_TIMEOUT, B_ONE_SHOT_RELATIVE_TIMER);
		}
	}

	return false;
}


bool
initialize_qreq_array(ide_device_info *device, int queue_depth)
{
	int i;

	device->queue_depth = queue_depth;

	SHOW_FLOW( 3, "queue depth=%d", device->queue_depth );

	device->qreq_array = (ide_qrequest *)malloc(queue_depth * sizeof(ide_qrequest));
	if (device->qreq_array == NULL)
		return false;

	memset(device->qreq_array, 0, queue_depth * sizeof(ide_qrequest));

	device->free_qrequests = NULL;

	for (i = queue_depth - 1; i >= 0 ; --i) {
		ide_qrequest *qrequest = &device->qreq_array[i];

		qrequest->next = device->free_qrequests;
		device->free_qrequests = qrequest;

		qrequest->running = false;
		qrequest->device = device;
		qrequest->tag = i;
		qrequest->request = NULL;
	}

	return true;
}


void
destroy_qreq_array(ide_device_info *device)
{
	if (device->qreq_array) {
		free(device->qreq_array);
		device->qreq_array = NULL;
	}

	device->num_running_reqs = 0;
	device->queue_depth = 0;
	device->free_qrequests = NULL;
}


/** change maximum number of queuable requests */

static bool
change_qreq_array(ide_device_info *device, int queue_depth)
{
	ide_qrequest *qreq_array = device->qreq_array;
	ide_qrequest *old_free_qrequests = device->free_qrequests;
	int old_queue_depth = queue_depth;

	// be very causious - even if no queuing supported, we still need
	// one queue entry; if this allocation fails, we have a device that
	// cannot accept any command, which would be odd
	if (initialize_qreq_array( device, queue_depth)) {
		free(qreq_array);
		return true;
	}

	device->qreq_array = qreq_array;
	device->num_running_reqs = 0;
	device->queue_depth = old_queue_depth;
	device->free_qrequests = old_free_qrequests;
	return false;
}


/**	reconnect timeout worker
 *	must be called as a synced procedure call, i.e.
 *	the bus is allocated for us
 */

void
reconnect_timeout_worker(ide_bus_info *bus, void *arg)
{
	ide_device_info *device = (ide_device_info *)arg;

	// perhaps all requests have been successfully finished
	// when the synced pc was waiting; in this case, everything's fine
	// (this is _very_ important if the other device blocks the bus
	// for a long time - if this leads to a reconnect timeout, the
	// device has a last chance by servicing all requests without
	// delay, in which case this function gets delayed until all
	// pending requests are finished and the following test would
	// make sure that this false alarm gets ignored)
	if (device->num_running_reqs > 0) {
		// if one queued command fails, all of them fail	
		send_abort_queue(device);

		// if too many timeouts occure, disable CQ
		if (++device->CQ_failures > MAX_CQ_FAILURES) {
			device->CQ_enabled = false;

			change_qreq_array(device, 1);
		}
	}

	// we've blocked the bus in dpc - undo that
	scsi->unblock_bus(device->bus->scsi_cookie);
}


/** dpc callback for reconnect timeout */

static void
reconnect_timeout_dpc(void *arg)
{
	ide_device_info *device = (ide_device_info *)arg;

	// even though we are in the service thread, 
	// the bus can be in use (e.g. by an ongoing PIO command),
	// so we have to issue a synced procedure call which
	// waits for the command to be finished

	// meanwhile, we don't want any command to be issued to this device
	// as we are going to discard the entire device queue;
	// sadly, we don't have a reliable XPT device handle, so we block
	// bus instead (as this is an error handler, so performance is
	// not crucial)
	scsi->block_bus(device->bus->scsi_cookie);
	schedule_synced_pc(device->bus, &device->reconnect_timeout_synced_pc, device);
}


/** timer function for reconnect timeout */

int32
reconnect_timeout(timer *arg)
{
	ide_device_info *device = ((ide_device_timer_info *)arg)->device;
	ide_bus_info *bus = device->bus;

	// we are polite and let the service thread do the job
	scsi->schedule_dpc(bus->scsi_cookie, device->reconnect_timeout_dpc, 
		reconnect_timeout_dpc, device);
		
	return B_INVOKE_SCHEDULER;
}


/**	tell device to abort all queued requests
 *	(tells XPT to resubmit these requests)
 *	return: true - abort successful
 *	        false - abort failed (in this case, nothing can be done)
 */

bool
send_abort_queue(ide_device_info *device)
{
	int status;
	ide_bus_info *bus = device->bus;

	SHOW_FLOW0( 3, "" );

	device->tf.write.command = IDE_CMD_NOP;
	// = discard outstanding commands
	device->tf.write.features = IDE_CMD_NOP_NOP;

	device->tf_param_mask = ide_mask_features;

	if (!send_command(device, NULL, true, 0, ide_state_accessing))
		goto err;

	if (!wait_for_drdy(device))
		goto err;

	// device must answer "command rejected" and discard outstanding commands
	status = bus->controller->get_altstatus(bus->channel_cookie);

	if ((status & ide_status_err) == 0)
		goto err;

	if (!bus->controller->read_command_block_regs(bus->channel_cookie,
			&device->tf, ide_mask_error)) {
		// don't bother trying bus_reset as controller disappeared
		device->subsys_status = SCSI_HBA_ERR;
		return false;
	}

	if ((device->tf.read.error & ide_error_abrt) == 0)
		goto err;

	finish_all_requests(device, NULL, 0, true);
	return true;

err:
	// ouch! device didn't react - we have to reset it
	return reset_device(device, NULL);
}
