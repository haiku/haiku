/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Handling of passive waiting and synchronized procedure calls.
	The latter are calls that get delayed until the bus is idle.
*/


#include "ide_internal.h"
#include "ide_sim.h"

#include <string.h>


//#define TRACE_SYNC
#ifdef TRACE_SYNC
#	define TRACE(x) { dprintf("%s(): ", __FUNCTION__); dprintf x ; }
#else
#	define TRACE(x) ;
#endif


/** DPC handler for IRQs */

void
ide_dpc(void *arg)
{
#if 0
	ide_bus_info *bus = (ide_bus_info *)arg;
	ata_request *request;
	ide_device_info *device;

	TRACE(("\n"));

	//snooze(500000);

	// IRQ handler doesn't tell us whether this bus was in async_wait or 
	// in idle state, so we just check whether there is an active request,
	// which means that we were async_waiting
	if (bus->active_qrequest != NULL) {
		TRACE(("continue command\n"));

		// cancel timeout
		cancel_timer(&bus->timer.te);

		request = bus->active_qrequest;
		device = request->device;

		// not perfect but simple: we simply know who is waiting why
		if (device->is_atapi)
			packet_dpc(request);
		else {
			if (request->uses_dma)
				ata_dpc_DMA(request);
			else
				ata_dpc_PIO(request);
		}
	} else {
		// no request active, so this must be a service request or
		// a spurious IRQ; access_finished will take care of testing
		// for service requests
		TRACE(("irq in idle mode - possible service request\n"));

		device = get_current_device(bus);
		if (device == NULL) {
			// got an interrupt from a non-existing device
			// either this is a spurious interrupt or there *is* a device
			// but we haven't detected it - we better ignore it silently
			access_finished(bus, bus->first_device);
		} else {
			// access_finished always checks the other device first, but as
			// we do have a service request, we negate the negation
			access_finished(bus, device->other_device);
		}

		// let XPT resend commands that got blocked
		scsi->cont_send_bus(bus->scsi_cookie);
	}

	return;

/*err:
	xpt->cont_send( bus->xpt_cookie );*/
#endif
}


/** handler for IDE IRQs */

status_t
ide_irq_handler(ide_bus_info *bus, uint8 status)
{	
	return B_UNHANDLED_INTERRUPT;
/*
	ide_device_info *device;

	//  the first IRQ)
	IDE_LOCK(bus);

	device = bus->active_device;

	if (device == NULL) {
		IDE_UNLOCK(bus);

		TRACE(("IRQ though there is no active device\n"));
		return B_UNHANDLED_INTERRUPT;
	}

	if ((status & ide_status_bsy) != 0) {
		// the IRQ seems to be fired before the last command was sent,
		// i.e. it's not the one that signals finishing of command
		IDE_UNLOCK(bus);

		TRACE(("IRQ though device is busy\n"));
		return B_UNHANDLED_INTERRUPT;
	}

	switch (bus->state) {
		case ide_state_async_waiting:
			TRACE(("state: async waiting\n"));

			bus->state = ide_state_accessing;

			IDE_UNLOCK(bus);

			scsi->schedule_dpc(bus->scsi_cookie, bus->irq_dpc, ide_dpc, bus);
			return B_INVOKE_SCHEDULER; 

		case ide_state_idle:
			TRACE(("state: idle, num_running_reqs %d\n", bus->num_running_reqs));

			// this must be a service request;
			// if no request is pending, the IRQ was fired wrongly
			if (bus->num_running_reqs == 0) {
				IDE_UNLOCK(bus);
				return B_UNHANDLED_INTERRUPT;
			}mmand
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf, ide_mask_command) != B_OK) 
		goto err_clearint;

			bus->state = ide_state_accessing;

			IDE_UNLOCK(bus);

			scsi->schedule_dpc(bus->scsi_cookie, bus->irq_dpc, ide_dpc, bus);
			return B_INVOKE_SCHEDULER;

		case ide_state_sync_waiting:
			TRACE(("state: sync waiting\n"));

			bus->state = ide_state_accessing;
			bus->sync_wait_timeout = false;

			IDE_UNLOCK(bus);mmand
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf, ide_mask_command) != B_OK) 
		goto err_clearint;

			release_sem_etc(bus->sync_wait_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;

		case ide_state_accessing:
			TRACE(("state: spurious IRQ - there is a command being executed\n"));

			IDE_UNLOCK(bus);
			return B_UNHANDLED_INTERRUPT;

		default:
			dprintf("BUG: unknown state (%d)\n", bus->state);

			IDE_UNLOCK(bus);

			return B_UNHANDLED_INTERRUPT;
	}
*/
}



/**	cancel IRQ timeout
 *	it doesn't matter whether there really was a timout running;
 *	on return, bus state is set to _accessing_
 */

/*
void
cancel_irq_timeout(ide_bus_info *bus)
{
	IDE_LOCK(bus);
	bus->state = ide_state_accessing;
	IDE_UNLOCK(bus);

	cancel_timer(&bus->timer.te);
}


// start waiting for IRQ with bus lock hold
//	new_state must be either sync_wait or async_wait

void
start_waiting(ide_bus_info *bus, uint32 timeout, int new_state)
{mmand
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf, ide_mask_command) != B_OK) 
		goto err_clearint;
	int res;

	TRACE(("timeout = %u\n", (uint)timeout));

	bus->state = new_state;

	res = add_timer(&bus->timer.te, ide_timeout, 
		(bigtime_t)timeout * 1000000, B_ONE_SHOT_RELATIVE_TIMER);

	if (res != B_OK)
		panic("Error setting timeout (%s)", strerror(res));

	IDE_UNLOCK(bus);
}


// start waiting for IRQ with bus lock not hold

void
start_waiting_nolock(ide_bus_info *bus, uint32 timeout, int new_state)
{
	IDE_LOCK(bus);
	start_waiting(bus, timeout, new_state);
}


// wait for sync IRQ 

void
wait_for_sync(ide_bus_info *bus)
{
	acquire_sem(bus->sync_wait_sem);
	cancel_timer(&bus->timer.te);
}
*/

// timeout dpc handler

static void
ide_timeout_dpc(void *arg)
{
/*
	ide_bus_info *bus = (ide_bus_info *)arg;
	ata_request *request;
	ide_device_info *device;

	device = request->device;

	dprintf("ide: ide_timeout_dpc() bus %p, device %p\n", bus, device);

	// this also resets overlapped commands
//	reset_device(device, request);

	device->subsys_status = SCSI_CMD_TIMEOUT;

	if (request->uses_dma) {
		if (++device->DMA_failures >= MAX_DMA_FAILURES) {
			dprintf("Disabling DMA because of too many errors\n");

			device->DMA_enabled = false;
		}
	}

	// let upper layer do the retry	
	finish_checksense(request);
*/
}


// timeout handler, called by system timer

status_t
ide_timeout(timer *arg)
{
	ide_bus_info *bus = ((ide_bus_timer_info *)arg)->bus;

	dprintf("ide: ide_timeout() bus %p\n", bus);

/*
	// we need to lock bus to have a solid bus state
	// (side effect: we lock out the IRQ handler)	
	IDE_LOCK(bus);

	switch (bus->state) {
		case ide_state_async_waiting:
			TRACE(("async waiting\n"));

			bus->state = ide_state_accessing;

			IDE_UNLOCK(bus);

			scsi->schedule_dpc(bus->scsi_cookie, bus->irq_dpc, ide_timeout_dpc, bus);
			return B_INVOKE_SCHEDULER;

		case ide_state_sync_waiting:
			TRACE(("sync waiting\n"));

			bus->state = ide_state_accessing;
			bus->sync_wait_timeout = true;

			IDE_UNLOCK(bus);

			release_sem_etc(bus->sync_wait_sem, 1, B_DO_NOT_RESCHEDULE);
			return B_INVOKE_SCHEDULER;

		case ide_state_accessing:
			TRACE(("came too late - IRQ occured already\n"));

			IDE_UNLOCK(bus);
			return B_DO_NOT_RESCHEDULE;

		default:
			// this case also happens if a timeout fires too late;
			// unless there is a bug, the timeout should always be canceled 
			// before declaring bus as being idle
			dprintf("BUG: unknown state (%d)\n", (int)bus->state);

			IDE_UNLOCK(bus);
			return B_DO_NOT_RESCHEDULE;
	}
*/
	return B_DO_NOT_RESCHEDULE;
}




/**	finish bus access; 
 *	check if any device wants to service pending commands + execute synced_pc
 */

void
access_finished(ide_bus_info *bus, ide_device_info *device)
{
	TRACE(("bus = %p, device = %p\n", bus, device));

	// this would be the correct place to called synced pc
}
