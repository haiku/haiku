/*
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Basic ATA/ATAPI protocol functions
*/


#include "ide_internal.h"
#include <scsi_cmds.h>

#include "ide_sim.h"
#include "ide_cmds.h"


// time in µs an IDE interrupt may get delayed
// as this is used for waiting in normal code, this applies to hardware delays only
// it's used for a hardware bug fix as well, see send_command
#define MAX_IRQ_DELAY 50

// maximum number send tries before giving up
#define MAX_FAILED_SEND 1


/** busy-wait for data request going high */

bool
wait_for_drq(ide_device_info *device)
{
	return ide_wait(device, ide_status_drq, 0, true, 10000000);
}


/** busy-wait for data request going low */

bool
wait_for_drqdown(ide_device_info *device)
{
	return ide_wait(device, 0, ide_status_drq, true, 1000000);
}


/** busy-wait for device ready */

bool
wait_for_drdy(ide_device_info *device)
{
	return ide_wait(device, ide_status_drdy, ide_status_bsy, false, 5000000);
}


/** reset entire IDE bus
 *	all active request apart from <ignore> are resubmitted
 */

bool
reset_bus(ide_device_info *device, ide_qrequest *ignore)
{
	ide_bus_info *bus = device->bus;
	ide_controller_interface *controller = bus->controller;
	ide_channel_cookie channel = bus->channel_cookie;

	dprintf("ide: reset_bus() device %p, bus %p\n", device, bus);

	if (device->reconnect_timer_installed) {
		cancel_timer(&device->reconnect_timer.te);
		device->reconnect_timer_installed = false;
	}

	if (device->other_device->reconnect_timer_installed) {
		cancel_timer(&device->other_device->reconnect_timer.te);
		device->other_device->reconnect_timer_installed = false;
	}

	// activate srst signal for 5 µs
	// also, deactivate IRQ
	// (as usual, we will get an IRQ on disabling, but as we leave them
	// disabled for 2 ms, this false report is ignored)
	if (controller->write_device_control(channel,
			ide_devctrl_nien | ide_devctrl_srst | ide_devctrl_bit3) != B_OK)
		goto err0;

	spin(5);

	if (controller->write_device_control(channel, ide_devctrl_nien | ide_devctrl_bit3) != B_OK)
		goto err0;

	// let devices wake up
	snooze(2000);

	// ouch, we have to wait up to 31 seconds!
	if (!ide_wait(device, 0, ide_status_bsy, true, 31000000)) {
		// as we don't know which of the devices is broken
		// we leave them both alive
		if (controller->write_device_control(channel, ide_devctrl_bit3) != B_OK)
			goto err0;

		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_TIMEOUT);
		goto err1;
	}

	if (controller->write_device_control(channel, ide_devctrl_bit3) != B_OK)
		goto err0;

	finish_all_requests(bus->devices[0], ignore, SCSI_SCSI_BUS_RESET, true);
	finish_all_requests(bus->devices[1], ignore, SCSI_SCSI_BUS_RESET, true);

	dprintf("ide: reset_bus() device %p, bus %p success\n", device, bus);
	return true;

err0:
	set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);

err1:
	finish_all_requests(bus->devices[0], ignore, SCSI_SCSI_BUS_RESET, true);
	finish_all_requests(bus->devices[1], ignore, SCSI_SCSI_BUS_RESET, true);

	//xpt->call_async( bus->xpt_cookie, -1, -1, AC_BUS_RESET, NULL, 0 );

	dprintf("ide: reset_bus() device %p, bus %p failed\n", device, bus);
	return false;
}


/** execute packet device reset.
 *	resets entire bus on fail or if device is not atapi;
 *	all requests but <ignore> are resubmitted
 */

bool
reset_device(ide_device_info *device, ide_qrequest *ignore)
{
	ide_bus_info *bus = device->bus;
	status_t res;
	uint8 orig_command;

	dprintf("ide: reset_device() device %p\n", device);

	SHOW_FLOW0(3, "");

	if (!device->is_atapi)
		goto err;

	if (device->reconnect_timer_installed) {
		cancel_timer(&device->reconnect_timer.te);
		device->reconnect_timer_installed = false;
	}

	// select device
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf,
			ide_mask_device_head) != B_OK)
		goto err;

	// safe original command to let caller restart it
	orig_command = device->tf.write.command;

	// send device reset, independ of current device state
	// (that's the point of a reset)
	device->tf.write.command = IDE_CMD_DEVICE_RESET;
	res = bus->controller->write_command_block_regs(bus->channel_cookie,
		&device->tf, ide_mask_command);
	device->tf.write.command = orig_command;

	if (res != B_OK)
		goto err;

	// don't know how long to wait, but 31 seconds, like soft reset,
	// should be enough
	if (!ide_wait(device, 0, ide_status_bsy, true, 31000000))
		goto err;

	// alright, resubmit all requests
	finish_all_requests(device, ignore, SCSI_SCSI_BUS_RESET, true);

	SHOW_FLOW0(3, "done");
	dprintf("ide: reset_device() device %p success\n", device);
	return true;

err:
	// do the hard way
	dprintf("ide: reset_device() device %p failed, calling reset_bus\n", device);
	return reset_bus(device, ignore);
}

/** new_state must be either accessing, async_waiting or sync_waiting
 *	param_mask must not include command register
 */

bool
send_command(ide_device_info *device, ide_qrequest *qrequest,
	bool need_drdy, uint32 timeout, ide_bus_state new_state)
{
	ide_bus_info *bus = device->bus;
	bigtime_t irq_disabled_at = 0; // make compiler happy
	uint8 num_retries = 0;
	bool irq_guard;

retry:
	irq_guard = bus->num_running_reqs > 1;

	SHOW_FLOW(3, "qrequest=%p, request=%p", qrequest,
		qrequest ? qrequest->request : NULL);

	// if there are pending requests, IRQs must be disabled to
	// not mix up IRQ reasons
	// XXX can we avoid that with the IDE_LOCK trick? It would
	//     save some work and the bug workaround!
	if (irq_guard) {
		if (bus->controller->write_device_control(bus->channel_cookie,
				ide_devctrl_nien | ide_devctrl_bit3) != B_OK)
			goto err;

		irq_disabled_at = system_time();
	}

	// select device
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf,
			ide_mask_device_head) != B_OK)
		goto err;

	bus->active_device = device;

	if (!ide_wait(device, 0, ide_status_bsy | ide_status_drq, false, 50000)) {
		uint8 status;

		SHOW_FLOW0(1, "device is not ready");

		status = bus->controller->get_altstatus(bus->channel_cookie);
		if (status == 0xff) {
			// there is no device (should happen during detection only)
			SHOW_FLOW0(1, "there is no device");

			// device detection recognizes this code as "all hope lost", so
			// neither replace it nor use it anywhere else
			device->subsys_status = SCSI_TID_INVALID;
			return false;
		}

		// reset device and retry
		if (reset_device(device, qrequest) && ++num_retries <= MAX_FAILED_SEND) {
			SHOW_FLOW0(1, "retrying");
			goto retry;
		}

		SHOW_FLOW0(1, "giving up");

		// reset to often - abort request
		device->subsys_status = SCSI_SEL_TIMEOUT;
		return false;
	}

	if (need_drdy
		&& (bus->controller->get_altstatus(bus->channel_cookie) & ide_status_drdy) == 0) {
		SHOW_FLOW0(3, "drdy not set");
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		return false;
	}

	// write parameters
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf,
			device->tf_param_mask) != B_OK)
		goto err;

	if (irq_guard) {
		// IRQ may be fired by service requests and by the process of disabling(!)
		// them (I heard this is caused by edge triggered PCI IRQs)

		// wait at least 50 µs to catch all pending irq's
		// (at my system, up to 30 µs elapsed)

		// additionally, old drives (at least my IBM-DTTA-351010) loose
		// sync if they are pushed too hard - on heavy overlapped write
		// stress this drive tends to forget outstanding requests,
		// waiting at least 50 µs seems(!) to solve this
		while (system_time() - irq_disabled_at < MAX_IRQ_DELAY)
			spin(1);
	}

	// if we will start waiting once the command is sent, we have to
	// lock the bus before sending; this way, IRQs that are fired
	// shortly before/after sending of command are delayed until the
	// command is really sent (start_waiting unlocks the bus) and then
	// the IRQ handler can check savely whether the IRQ really signals
	// finishing of command or not by testing the busy-signal of the device
	if (new_state != ide_state_accessing) {
		IDE_LOCK(bus);
	}

	if (irq_guard) {
		// now it's clear why IRQs gets fired, so we can enable them again
		if (bus->controller->write_device_control(bus->channel_cookie,
				ide_devctrl_bit3) != B_OK)
			goto err1;
	}

	// write command code - this will start the actual command
	SHOW_FLOW(3, "Writing command 0x%02x", (int)device->tf.write.command);
	if (bus->controller->write_command_block_regs(bus->channel_cookie,
			&device->tf, ide_mask_command) != B_OK)
		goto err1;

	// start waiting now; also un-blocks IRQ handler (see above)
	if (new_state != ide_state_accessing)
		start_waiting(bus, timeout, new_state);

	return true;

err1:
	if (timeout > 0) {
		bus->state = ide_state_accessing;
		IDE_UNLOCK(bus);
	}

err:
	device->subsys_status = SCSI_HBA_ERR;
	return false;
}


/**	busy-wait for device
 *	mask - bits of status register that must be set
 *	not_mask - bits of status register that must not be set
 *	check_err - abort if error bit is set
 *	timeout - waiting timeout
 *	return: true on success
 */

bool
ide_wait(ide_device_info *device, int mask, int not_mask,
	bool check_err, bigtime_t timeout)
{
	ide_bus_info *bus = device->bus;
	bigtime_t start_time = system_time();

	while (1) {
		bigtime_t elapsed_time;
		int status;

		// do spin before test as the device needs 400 ns
		// to update its status register
		spin(1);

		status = bus->controller->get_altstatus(bus->channel_cookie);

		if ((status & mask) == mask && (status & not_mask) == 0)
			return true;

		if (check_err && (status & ide_status_err) != 0) {
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
			return false;
		}

		elapsed_time = system_time() - start_time;

		if (elapsed_time > timeout) {
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_TIMEOUT);
			return false;
		}

		// if we've waited more then 5ms, we start passive waiting
		// to reduce system load
		if (elapsed_time > 5000)
			snooze(elapsed_time / 10);
	}
}


/**	tell device to continue queued command
 *	on return, no waiting is active!
 *	tag - will contain tag of command to be continued
 *	return: true - request continued
 *	       false - something went wrong; sense set
 */

bool
device_start_service(ide_device_info *device, int *tag)
{
	ide_bus_info *bus = device->bus;

	device->tf.write.command = IDE_CMD_SERVICE;
	device->tf.queued.mode = ide_mode_lba;

	if (bus->active_device != device) {
		// don't apply any precautions in terms of IRQ
		// -> the bus is in accessing state, so IRQs are ignored anyway
		if (bus->controller->write_command_block_regs(bus->channel_cookie,
				&device->tf, ide_mask_device_head) != B_OK)
			// on error, pretend that this device asks for service
			// -> the disappeared controller will be recognized soon ;)
			return true;

		bus->active_device = device;

		// give one clock (400 ns) to take notice
		spin(1);
	}

	// here we go...
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf,
			ide_mask_command) != B_OK)
		goto err;

	// we need to wait for the device as we want to read the tag
	if (!ide_wait(device, ide_status_drdy, ide_status_bsy, false, 1000000))
		return false;

	// read tag
	if (bus->controller->read_command_block_regs(bus->channel_cookie, &device->tf,
			ide_mask_sector_count) != B_OK)
		goto err;

	if (device->tf.queued.release) {
		// bus release is the wrong answer to a service request
		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
		return false;
	}

	*tag = device->tf.queued.tag;

	return true;

err:
	set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
	return false;
}


/** check device whether it wants to continue queued request */

bool
check_service_req(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;
	int status;

	// fast bailout if there is no request pending
	if (device->num_running_reqs == 0)
		return false;

	if (bus->active_device != device) {
		// don't apply any precautions in terms of IRQ
		// -> the bus is in accessing state, so IRQs are ignored anyway
		if (bus->controller->write_command_block_regs(bus->channel_cookie,
				&device->tf, ide_mask_device_head) != B_OK)
			// on error, pretend that this device asks for service
			// -> the disappeared controller will be recognized soon ;)
			return true;

		bus->active_device = device;

		// give one clock (400 ns) to take notice
		spin(1);
	}

	status = bus->controller->get_altstatus(bus->channel_cookie);

	return (status & ide_status_service) != 0;
}

