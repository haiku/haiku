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


status_t
reset_bus(ide_bus_info *bus, bool *devicePresent0, uint32 *sigDev0, bool *devicePresent1, uint32 *sigDev1)
{
	ide_controller_interface *controller = bus->controller;
	ide_channel_cookie channel = bus->channel_cookie;
	ide_task_file tf;
	status_t status;

	dprintf("ATA: reset_bus %p\n", bus);

	*devicePresent0 = ata_is_device_present(bus, 0);
	*devicePresent1 = ata_is_device_present(bus, 1);

	dprintf("ATA: reset_bus: ata_is_device_present device 0, present %d\n", *devicePresent0);
	dprintf("ATA: reset_bus: ata_is_device_present device 1, present %d\n", *devicePresent1);

	// disable interrupts and assert SRST for at least 5 usec
	if (controller->write_device_control(channel, ide_devctrl_bit3 | ide_devctrl_nien | ide_devctrl_srst) != B_OK)
		goto error;
	spin(20);

	// clear SRST and wait for at least 2 ms but (we wait 150ms like everyone else does)
	if (controller->write_device_control(channel, ide_devctrl_bit3 | ide_devctrl_nien) != B_OK)
		goto error;
	snooze(150000);

	if (*devicePresent0) {

		ata_select_device(bus, 0);
		dprintf("altstatus device 0: %x\n", controller->get_altstatus(channel));

		// wait up to 31 seconds for busy to clear, abort when error is set
		status = ata_wait(bus, 0, ide_status_bsy, false, 31000000);
		if (status != B_OK) {
			dprintf("ATA: reset_bus: timeout\n");
			goto error;
		}

		if (controller->read_command_block_regs(channel, &tf, ide_mask_sector_count |
			ide_mask_LBA_low | ide_mask_LBA_mid | ide_mask_LBA_high | ide_mask_error) != B_OK)
			goto error;

		if (tf.read.error != 0x01 && tf.read.error != 0x81)
			dprintf("ATA: device 0 failed, error code is 0x%02\n", tf.read.error);

		if (tf.read.error >= 0x80)
			dprintf("ATA: device 0 indicates that device 1 failed, error code is 0x%02\n", tf.read.error);

		*sigDev0 = tf.lba.sector_count;
		*sigDev0 |= ((uint32)tf.lba.lba_0_7) << 8;
		*sigDev0 |= ((uint32)tf.lba.lba_8_15) << 16;
		*sigDev0 |= ((uint32)tf.lba.lba_16_23) << 24;
	} else {
		*sigDev0 = 0;
	}

	if (*devicePresent1) {	
		
		ata_select_device(bus, 1);
		dprintf("altstatus device 1: %x\n", controller->get_altstatus(channel));

		// wait up to 31 seconds for busy to clear, abort when error is set
		status = ata_wait(bus, 0, ide_status_bsy, false, 31000000);
		if (status != B_OK) {
			dprintf("ATA: reset_bus: timeout\n");
			goto error;
		}

		if (controller->read_command_block_regs(channel, &tf, ide_mask_sector_count |
			ide_mask_LBA_low | ide_mask_LBA_mid | ide_mask_LBA_high | ide_mask_error) != B_OK)
			goto error;

		if (tf.read.error != 0x01)
			dprintf("ATA: device 1 failed, error code is 0x%02\n", tf.read.error);

		*sigDev1 = tf.lba.sector_count;
		*sigDev1 |= ((uint32)tf.lba.lba_0_7) << 8;
		*sigDev1 |= ((uint32)tf.lba.lba_8_15) << 16;
		*sigDev1 |= ((uint32)tf.lba.lba_16_23) << 24;
	} else {
		*sigDev1 = 0;
	}

	dprintf("ATA: reset_bus success, device 0 signature: 0x%08lx, device 1 signature: 0x%08lx\n", *sigDev0, *sigDev1);

	return B_OK;

error:

	dprintf("ATA: reset_bus failed\n");
	return B_ERROR;
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

	FAST_LOGN(bus->log, ev_ide_send_command, 15, device->is_device1, (uint32)qrequest, 
		device->tf.raw.r[0], device->tf.raw.r[1], device->tf.raw.r[2], 
		device->tf.raw.r[3], device->tf.raw.r[4], device->tf.raw.r[5], 
		device->tf.raw.r[6], 
		device->tf.raw.r[7], device->tf.raw.r[8], device->tf.raw.r[9], 
		device->tf.raw.r[10], device->tf.raw.r[11]);

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

/*
		// reset device and retry
		if (reset_device(device, qrequest) && ++num_retries <= MAX_FAILED_SEND) {
			SHOW_FLOW0(1, "retrying");
			goto retry;
		}
*/
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


status_t
ata_wait(ide_bus_info *bus, uint8 mask, uint8 not_mask,
		 bool check_err, bigtime_t timeout)
{
	bigtime_t startTime = system_time();
	bigtime_t elapsedTime;
	uint8 status;

	spin(1); // device needs 400ns to set status

	for (;;) {

		status = bus->controller->get_altstatus(bus->channel_cookie);

		if (check_err && (status & ide_status_err) != 0)
			return B_ERROR;

		if ((status & mask) == mask && (status & not_mask) == 0)
			return B_OK;

		elapsedTime = system_time() - startTime;

		if (elapsedTime > timeout)
			return B_TIMED_OUT;
		
		if (elapsedTime < 5000)
			spin(1);
		else
			snooze(5000);
	}
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

	FAST_LOG1(bus->log, ev_ide_device_start_service, device->is_device1);

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

	FAST_LOG2(bus->log, ev_ide_device_start_service2, device->is_device1, *tag);
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

