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
	ide_bus_info *bus = (ide_bus_info *)arg;
	ide_qrequest *qrequest;
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

		qrequest = bus->active_qrequest;
		device = qrequest->device;

		// not perfect but simple: we simply know who is waiting why
		if (device->is_atapi)
			packet_dpc(qrequest);
		else {
			if (qrequest->uses_dma)
				ata_dpc_DMA(qrequest);
			else
				ata_dpc_PIO(qrequest);
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
}


/** handler for IDE IRQs */

status_t
ide_irq_handler(ide_bus_info *bus, uint8 status)
{
	ide_device_info *device;

	// we need to lock bus to have a solid bus state
	// (side effect: we lock out the timeout handler and get
	//  delayed if the IRQ happens at the same time as a command is
	//  issued; in the latter case, we have no official way to determine
	//  whether the command was issued before or afterwards; if it was
	//  afterwards, the device must not be busy; if it was before,
	//  the device is either busy because of the sent command, or it's
	//  not busy as the command has already been finished, i.e. there
	//  was a second IRQ which we've overlooked as we didn't acknowledge
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
			}

			bus->state = ide_state_accessing;

			IDE_UNLOCK(bus);

			scsi->schedule_dpc(bus->scsi_cookie, bus->irq_dpc, ide_dpc, bus);
			return B_INVOKE_SCHEDULER;

		case ide_state_sync_waiting:
			TRACE(("state: sync waiting\n"));

			bus->state = ide_state_accessing;
			bus->sync_wait_timeout = false;

			IDE_UNLOCK(bus);

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
}


/**	cancel IRQ timeout
 *	it doesn't matter whether there really was a timout running;
 *	on return, bus state is set to _accessing_
 */

void
cancel_irq_timeout(ide_bus_info *bus)
{
	IDE_LOCK(bus);
	bus->state = ide_state_accessing;
	IDE_UNLOCK(bus);

	cancel_timer(&bus->timer.te);
}


/** start waiting for IRQ with bus lock hold
 *	new_state must be either sync_wait or async_wait
 */

void
start_waiting(ide_bus_info *bus, uint32 timeout, int new_state)
{
	int res;

	TRACE(("timeout = %u\n", (uint)timeout));

	bus->state = new_state;

	res = add_timer(&bus->timer.te, ide_timeout,
		(bigtime_t)timeout * 1000000, B_ONE_SHOT_RELATIVE_TIMER);

	if (res != B_OK)
		panic("Error setting timeout (%s)", strerror(res));

	IDE_UNLOCK(bus);
}


/** start waiting for IRQ with bus lock not hold */

void
start_waiting_nolock(ide_bus_info *bus, uint32 timeout, int new_state)
{
	IDE_LOCK(bus);
	start_waiting(bus, timeout, new_state);
}


/** wait for sync IRQ */

void
wait_for_sync(ide_bus_info *bus)
{
	acquire_sem(bus->sync_wait_sem);
	cancel_timer(&bus->timer.te);
}


/** timeout dpc handler */

static void
ide_timeout_dpc(void *arg)
{
	ide_bus_info *bus = (ide_bus_info *)arg;
	ide_qrequest *qrequest;
	ide_device_info *device;

	qrequest = bus->active_qrequest;
	device = qrequest->device;

	dprintf("ide: ide_timeout_dpc() bus %p, device %p\n", bus, device);

	// this also resets overlapped commands
	reset_device(device, qrequest);

	device->subsys_status = SCSI_CMD_TIMEOUT;

	if (qrequest->uses_dma) {
		if (++device->DMA_failures >= MAX_DMA_FAILURES) {
			dprintf("Disabling DMA because of too many errors\n");

			device->DMA_enabled = false;
		}
	}

	// let upper layer do the retry
	finish_checksense(qrequest);
}


/** timeout handler, called by system timer */

status_t
ide_timeout(timer *arg)
{
	ide_bus_info *bus = ((ide_bus_timer_info *)arg)->bus;

	TRACE(("ide_timeout(): %p\n", bus));

	dprintf("ide: ide_timeout() bus %p\n", bus);

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
}


void
init_synced_pc(ide_synced_pc *pc, ide_synced_pc_func func)
{
	pc->func = func;
	pc->registered = false;
}


void
uninit_synced_pc(ide_synced_pc *pc)
{
	if (pc->registered)
		panic("Tried to clean up pending synced PC\n");
}


/**	schedule a synced pc
 *	a synced pc gets executed as soon as the bus becomes idle
 */

status_t
schedule_synced_pc(ide_bus_info *bus, ide_synced_pc *pc, void *arg)
{
	//TRACE(());

	IDE_LOCK(bus);

	if (pc->registered) {
		// spc cannot be registered twice
		TRACE(("already registered\n"));
		return B_ERROR;
	} else if( bus->state != ide_state_idle ) {
		// bus isn't idle - spc must be added to pending list
		TRACE(("adding to pending list\n"));

		pc->next = bus->synced_pc_list;
		bus->synced_pc_list = pc;
		pc->arg = arg;
		pc->registered = true;

		IDE_UNLOCK(bus);
		return B_OK;
	}

	// we have luck - bus is idle, so grab it before
	// releasing the lock

	TRACE(("exec immediately\n"));

	bus->state = ide_state_accessing;
	IDE_UNLOCK(bus);

	TRACE(("go\n"));
	pc->func(bus, arg);

	TRACE(("finished\n"));
	access_finished(bus, bus->first_device);

	// meanwhile, we may have rejected SCSI commands;
	// usually, the XPT resends them once a command
	// has finished, but in this case XPT doesn't know
	// about our "private" command, so we have to tell about
	// idle bus manually
	TRACE(("tell SCSI bus manager about idle bus\n"));
	scsi->cont_send_bus(bus->scsi_cookie);
	return B_OK;
}


/** execute list of synced pcs */

static void
exec_synced_pcs(ide_bus_info *bus, ide_synced_pc *pc_list)
{
	ide_synced_pc *pc;

	// noone removes items from pc_list, so we don't need lock
	// to access entries
	for (pc = pc_list; pc; pc = pc->next) {
		pc->func(bus, pc->arg);
	}

	// need lock now as items can be added to pc_list again as soon
	// as <registered> is reset
	IDE_LOCK(bus);

	for (pc = pc_list; pc; pc = pc->next) {
		pc->registered = false;
	}

	IDE_UNLOCK(bus);
}


/**	finish bus access;
 *	check if any device wants to service pending commands + execute synced_pc
 */

void
access_finished(ide_bus_info *bus, ide_device_info *device)
{
	TRACE(("bus = %p, device = %p\n", bus, device));

	while (true) {
		ide_synced_pc *synced_pc_list;

		IDE_LOCK(bus);

		// normally, there is always an device; only exception is a
		// bus without devices, not sure whether this can really happen though
		if (device) {
			if (try_service(device))
				return;
		}

		// noone wants it, so execute pending synced_pc
		if (bus->synced_pc_list == NULL) {
			bus->state = ide_state_idle;
			IDE_UNLOCK(bus);
			return;
		}

		synced_pc_list = bus->synced_pc_list;
		bus->synced_pc_list = NULL;

		IDE_UNLOCK(bus);

		exec_synced_pcs(bus, synced_pc_list);

		// executed synced_pc may have generated other sync_pc,
		// thus the loop
	}
}
