/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	ATA command protocol
*/


#include "ide_internal.h"

#include "ide_sim.h"
#include "ide_cmds.h"

#define TRACE dprintf
#define FLOW dprintf


static void ata_exec_dma_transfer(ata_request *request);
static void	ata_exec_pio_transfer(ata_request *request);


void
ata_select_device(ide_bus_info *bus, int device)
{
	ide_task_file tf;

//	FLOW("ata_select_device device %d\n", device);

	tf.chs.head = 0;
	tf.chs.mode = ide_mode_lba;
	tf.chs.device = device ? 1 : 0;

	bus->controller->write_command_block_regs(bus->channel_cookie, &tf, ide_mask_device_head);
	bus->controller->get_altstatus(bus->channel_cookie); // flush posted writes
	spin(1); // wait 400 nsec

	// for debugging only
	bus->controller->read_command_block_regs(bus->channel_cookie, &tf, ide_mask_device_head);
	if (tf.chs.device != device)
		TRACE("ata_select_device result: device %d not selected! head 0x%x, mode 0x%x, device %d\n", device, tf.chs.head, tf.chs.mode, tf.chs.device);
}


void
ata_select(ide_device_info *device)
{
	ASSERT(device->is_device1 == device->tf.chs.device);
	ata_select_device(device->bus, device->is_device1);
}


/* Detect if the device is present
 * This is unrelyable for some controllers and
 * may report false posives.
 */
bool
ata_is_device_present(ide_bus_info *bus, int device)
{
	ide_task_file tf;

	ata_select_device(bus, device);

	tf.lba.sector_count = 0xaa;
	tf.lba.lba_0_7 = 0x55;
	bus->controller->write_command_block_regs(bus->channel_cookie, &tf,
		ide_mask_sector_count | ide_mask_LBA_low);
	bus->controller->get_altstatus(bus->channel_cookie); // flush posted writes
	spin(1);

	bus->controller->read_command_block_regs(bus->channel_cookie, &tf,
		ide_mask_sector_count | ide_mask_LBA_low);

	return tf.lba.sector_count == 0xaa && tf.lba.lba_0_7 == 0x55;
}


/**	busy-wait for device
 *	set       - bits of status register that must be set
 *	cleared   - bits of status register that must be cleared
 *	flags     - 0 or ATA_CHECK_ERROR_BIT abort if error bit is set
 *	timeout   - waiting timeout
 */
status_t
ata_wait(ide_bus_info *bus, uint8 set, uint8 cleared,
		 ata_flags flags, bigtime_t timeout)
{
	bigtime_t startTime = system_time();
	bigtime_t elapsedTime = 0;
	uint8 status;

	bus->controller->get_altstatus(bus->channel_cookie); // flush posted writes
	spin(1); // device needs 400ns to set status

	for (;;) {
		status = bus->controller->get_altstatus(bus->channel_cookie);

		if ((flags & ATA_CHECK_ERROR_BIT) && (status & ide_status_err) != 0)
			return B_ERROR;

		if ((status & set) == set && (status & cleared) == 0) {
			return B_OK;
		}

		elapsedTime = system_time() - startTime;

		if (elapsedTime > timeout)
			return B_TIMED_OUT;
		
		if (elapsedTime < 4000)
			spin(1);
		else
			snooze(4000);
	}
}


// busy-wait for data request going high
status_t
ata_wait_for_drq(ide_bus_info *bus)
{
	return ata_wait(bus, ide_status_drq, 0, ATA_CHECK_ERROR_BIT, 10000000);
}


// busy-wait for data request going low
status_t
ata_wait_for_drqdown(ide_bus_info *bus)
{
	return ata_wait(bus, 0, ide_status_drq, ATA_CHECK_ERROR_BIT, 1000000);
}


// busy-wait for device ready
status_t
ata_wait_for_drdy(ide_bus_info *bus)
{
	return ata_wait(bus, ide_status_drdy, ide_status_bsy, 0, 5000000);
}


// wait 20ms for device to report idle (busy and drq clear)
status_t
ata_wait_idle(ide_bus_info *bus)
{
	return ata_wait(bus, 0, ide_status_bsy | ide_status_drq, 0, 20000);
}

/*
// busy wait for device beeing ready,
// using the timeout set by the previous ata_send_command
status_t
ata_pio_wait_drdy(ide_device_info *device)
{
	ASSERT(device->bus->state == ata_state_pio);
	return ata_wait(device->bus, ide_status_drdy, ide_status_bsy, 0, device->pio_timeout);
}
*/

status_t
ata_send_command(ide_device_info *device, ata_request *request, ata_flags flags, bigtime_t timeout)
{
	ide_bus_info *bus = device->bus;

	ASSERT((device->tf_param_mask & ide_mask_command) == 0);
//	ASSERT(bus->state == ata_state_busy); // XXX fix this

	ASSERT((flags & ATA_DMA_TRANSFER) == 0); // XXX only pio for now

	FLOW("ata_send_command: %d:%d, request %p, ccb %p, tf %02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x\n",
		device->target_id, device->is_device1, 
		request, request ? request->ccb : NULL,
		device->tf.raw.r[0], device->tf.raw.r[1], device->tf.raw.r[2], 
		device->tf.raw.r[3], device->tf.raw.r[4], device->tf.raw.r[5], 
		device->tf.raw.r[6], device->tf.raw.r[7], device->tf.raw.r[8],
		device->tf.raw.r[9], device->tf.raw.r[10], device->tf.raw.r[11]);

	// disable Interrupts for PIO transfers
	if ((flags & ATA_DMA_TRANSFER) == 0) {
		if (bus->controller->write_device_control(bus->channel_cookie, ide_devctrl_bit3 | ide_devctrl_nien) != B_OK)
			goto err;
	}

	ata_select(device);

	if (ata_wait_idle(bus) != B_OK) {
		// resetting the device here will discard current configuration,
		// it's better when the SCSI bus manager requests an external reset.
		TRACE("ata_send_command: device selection timeout\n");
		ata_request_set_status(request, SCSI_SEL_TIMEOUT);
		return B_ERROR;
	}

	if ((flags & ATA_DRDY_REQUIRED) && (bus->controller->get_altstatus(bus->channel_cookie) & ide_status_drdy) == 0) {
		TRACE("ata_send_command: drdy not set\n");
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
		return B_ERROR;
	}

	// write parameters	
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf,	device->tf_param_mask) != B_OK)
		goto err;

//	FLOW("Writing command 0x%02x\n", (int)device->tf.write.command);

	IDE_LOCK(bus);

	if (flags & ATA_DMA_TRANSFER) {
		// enable interrupt
		if (bus->controller->write_device_control(bus->channel_cookie,	ide_devctrl_bit3) != B_OK)
			goto err_clearint;
	}

	// start the command
	if (bus->controller->write_command_block_regs(bus->channel_cookie, &device->tf, ide_mask_command) != B_OK) 
		goto err_clearint;

//	ASSERT(bus->state == ata_state_busy); XXX fix this

	bus->state = (flags & ATA_DMA_TRANSFER) ? ata_state_dma : ata_state_pio;
	request->timeout = timeout;

	IDE_UNLOCK(bus);

	return B_OK;


err_clearint:
	bus->controller->write_device_control(bus->channel_cookie, ide_devctrl_bit3 | ide_devctrl_nien);

err:
	ata_request_set_status(request, SCSI_HBA_ERR);
	IDE_UNLOCK(bus);
	return B_ERROR;
}


/** check result of ATA command
 *	drdy_required - true if drdy must be set by device
 *	error_mask - bits to be checked in error register
 *	is_write - true, if command was a write command
 */

status_t
ata_finish_command(ide_device_info *device, ata_request *request, ata_flags flags, uint8 errorMask)
{
	ide_bus_info *bus = device->bus;
	status_t result;
	uint8 error;

	if (flags & ATA_WAIT_FINISH) {
		// wait for the command to finish current command (device no longer busy)
		result = ata_wait(bus, 0, ide_status_bsy, 0, request->timeout);
		if (result != B_OK) {
			TRACE("ata_finish_command: timeout\n");
			ata_request_set_status(request, SCSI_CMD_TIMEOUT);
			return result;
		}
	}

	// read status, this also acknowledges pending interrupts
	result = device->bus->controller->read_command_block_regs(device->bus->channel_cookie, &device->tf, ide_mask_status | ide_mask_error);
	if (result != B_OK) {
		TRACE("ata_finish_command: status register read failed\n");
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
			return result;
	}
	
	if (device->tf.read.status & ide_status_bsy) {
		TRACE("ata_finish_command: failed! device still busy\n");
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
		return B_ERROR;
	}
	
	if ((flags & ATA_DRDY_REQUIRED) && (device->tf.read.status & ide_status_drdy) == 0) {
		TRACE("ata_finish_command: failed! drdy not set\n");
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
		return B_ERROR;
	}

	if ((device->tf.read.status & ide_status_err) == 0)
		return B_OK;

	result = device->bus->controller->read_command_block_regs(device->bus->channel_cookie, &device->tf, ide_mask_error);
	if (result != B_OK) {
		TRACE("ata_finish_command: error register read failed\n");
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
		return result;
	}

	TRACE("ata_finish_command: command failed ERR bit is set\n");

	// check only relevant error bits
	error = device->tf.read.error & errorMask;
	result = B_ERROR;

	if (error & ide_error_icrc) {
		ata_request_set_sense(request, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_CRC);
		return B_ERROR;
	}

	if (flags & ATA_IS_WRITE) {
		if (error & ide_error_wp) {
			ata_request_set_sense(request, SCSIS_KEY_DATA_PROTECT, SCSIS_ASC_WRITE_PROTECTED);
			return B_ERROR;
		}
	} else {
		if (error & ide_error_unc) {
			ata_request_set_sense(request, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_UNREC_READ_ERR);
			return B_ERROR;
		}
	}

	if (error & ide_error_mc) {
		// XXX proper sense key?
		ata_request_set_sense(request, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_MEDIUM_CHANGED);
		return B_ERROR;
	}

	if (error & ide_error_idnf) {
		// XXX strange error code, don't really know what it means
		ata_request_set_sense(request, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_RANDOM_POS_ERROR);
		return B_ERROR;
	}

	if (error & ide_error_mcr) {
		// XXX proper sense key?
		ata_request_set_sense(request, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_REMOVAL_REQUESTED);
		return B_ERROR;
	}

	if (error & ide_error_nm) {
		ata_request_set_sense(request, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_NO_MEDIUM);
		return B_ERROR;
	}

	if (error & ide_error_abrt) {
		ata_request_set_sense(request, SCSIS_KEY_ABORTED_COMMAND, SCSIS_ASC_NO_SENSE);
		return B_ERROR;
	}

	// either there was no error bit set or it was masked out
	ata_request_set_sense(request, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
	return B_ERROR;
}


status_t
ata_read_status(ide_device_info *device, uint8 *status)
{
	status_t result = device->bus->controller->read_command_block_regs(device->bus->channel_cookie, &device->tf, ide_mask_status);
	if (status)
		*status = device->tf.read.status;
	return result;
}


status_t
ata_reset_bus(ide_bus_info *bus, bool *_devicePresent0, uint32 *_sigDev0, bool *_devicePresent1, uint32 *_sigDev1)
{
	ide_controller_interface *controller = bus->controller;
	ide_channel_cookie channel = bus->channel_cookie;
	bool devicePresent0;
	bool devicePresent1;
	ide_task_file tf;
	status_t status;

	dprintf("ATA: reset_bus %p\n", bus);

	devicePresent0 = ata_is_device_present(bus, 0);
	devicePresent1 = ata_is_device_present(bus, 1);

	dprintf("ATA: reset_bus: device 0: %s present\n", devicePresent0 ? "might be" : "is not");
	dprintf("ATA: reset_bus: device 1: %s present\n", devicePresent1 ? "might be" : "is not");

	// select device 0
	ata_select_device(bus, 0);

	// disable interrupts and assert SRST for at least 5 usec
	if (controller->write_device_control(channel, ide_devctrl_bit3 | ide_devctrl_nien | ide_devctrl_srst) != B_OK)
		goto error;
	controller->get_altstatus(channel); // flush posted writes
	spin(20);

	// clear SRST and wait for at least 2 ms but (we wait 150ms like everyone else does)
	if (controller->write_device_control(channel, ide_devctrl_bit3 | ide_devctrl_nien) != B_OK)
		goto error;
	controller->get_altstatus(channel); // flush posted writes
	snooze(150000);

	if (devicePresent0) {

		ata_select_device(bus, 0);
//		dprintf("altstatus device 0: %x\n", controller->get_altstatus(channel));

		// wait up to 31 seconds for busy to clear
		status = ata_wait(bus, 0, ide_status_bsy, 0, 31000000);
		if (status != B_OK) {
			dprintf("ATA: reset_bus: timeout\n");
			goto error;
		}

		if (controller->read_command_block_regs(channel, &tf, ide_mask_sector_count |
			ide_mask_LBA_low | ide_mask_LBA_mid | ide_mask_LBA_high | ide_mask_error) != B_OK)
			goto error;

		if (tf.read.error != 0x01 && tf.read.error != 0x81)
			dprintf("ATA: device 0 failed, error code is 0x%02x\n", tf.read.error);

		if (tf.read.error >= 0x80)
			dprintf("ATA: device 0 indicates that device 1 failed, error code is 0x%02x\n", tf.read.error);

		if (_sigDev0) {
			*_sigDev0 = tf.lba.sector_count;
			*_sigDev0 |= ((uint32)tf.lba.lba_0_7) << 8;
			*_sigDev0 |= ((uint32)tf.lba.lba_8_15) << 16;
			*_sigDev0 |= ((uint32)tf.lba.lba_16_23) << 24;
		}
	}

	if (devicePresent1) {	
		
		ata_select_device(bus, 1);
//		dprintf("altstatus device 1: %x\n", controller->get_altstatus(channel));

		// wait up to 31 seconds for busy to clear
		status = ata_wait(bus, 0, ide_status_bsy, 0, 31000000);
		if (status != B_OK) {
			dprintf("ATA: reset_bus: timeout\n");
			goto error;
		}

		if (controller->read_command_block_regs(channel, &tf, ide_mask_sector_count |
			ide_mask_LBA_low | ide_mask_LBA_mid | ide_mask_LBA_high | ide_mask_error) != B_OK)
			goto error;

		if (tf.read.error != 0x01)
			dprintf("ATA: device 1 failed, error code is 0x%02x\n", tf.read.error);

		if (_sigDev1) {
			*_sigDev1 = tf.lba.sector_count;
			*_sigDev1 |= ((uint32)tf.lba.lba_0_7) << 8;
			*_sigDev1 |= ((uint32)tf.lba.lba_8_15) << 16;
			*_sigDev1 |= ((uint32)tf.lba.lba_16_23) << 24;
		}
	}

	if (_devicePresent0)
		*_devicePresent0 = devicePresent0;
	if (_devicePresent1)
		*_devicePresent1 = devicePresent1;

	dprintf("ATA: reset_bus done\n");
	return B_OK;

error:
	dprintf("ATA: reset_bus failed\n");
	return B_ERROR;
}


status_t
ata_reset_device(ide_device_info *device, bool *_devicePresent)
{
	// XXX first try to reset the single device here

	dprintf("ATA: ata_reset_device %p calling ata_reset_bus\n", device);
	return ata_reset_bus(device->bus, 
				device->is_device1 ? NULL : _devicePresent, NULL,
				device->is_device1 ? _devicePresent : NULL, NULL);
}


/** verify that device is ready for further PIO transmission */

static bool
check_rw_status(ide_device_info *device, bool drqStatus)
{
/*
	ide_bus_info *bus = device->bus;
	int status;

	status = bus->controller->get_altstatus(bus->channel_cookie);

	if ((status & ide_status_bsy) != 0) {
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		return false;
	}

	if (drqStatus != ((status & ide_status_drq) != 0)) {
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		return false;
	}
*/
	return true;
}


#if 0
/**	DPC called at
 *	 - begin of each PIO read/write block
 *	 - end of PUI write transmission
 */

void
ata_dpc_PIO(ata_request *request)
{
	ide_device_info *device = request->device;
	uint32 timeout = request->ccb->timeout > 0 ? 
		request->ccb->timeout * 1000 : IDE_STD_TIMEOUT;

	SHOW_FLOW0(3, "");

	if (check_rw_error(device, request)
		|| !check_rw_status(device, request->is_write ? device->left_blocks > 0 : true)) 
	{
		// failure reported by device
		SHOW_FLOW0( 3, "command finished unsuccessfully" );

		finish_checksense(request);
		return;
	}

	if (request->is_write) {
		if (device->left_blocks == 0) {
			// this was the end-of-transmission IRQ
			SHOW_FLOW0(3, "write access finished");
			if (!wait_for_drqdown(device)) {
				SHOW_ERROR0(3, "device wants to transmit data though command is finished");
				goto finish;
			}
			goto finish;
		}

		// wait until device requests data
		SHOW_FLOW0(3, "Waiting for device ready to transmit");
		if (!wait_for_drq(device)) {
			SHOW_FLOW0(3, "device not ready for data transmission - abort");
			goto finish;
		}

		// start async waiting for next block/end of command
		// we should start that when block is transmitted, but with bad
		// luck the IRQ fires exactly between transmission and start of waiting,
		// so we better start waiting too early; as we are in service thread,
		// a DPC initiated by IRQ cannot overtake us, so there is no need to block
		// IRQs during sent
		start_waiting_nolock(device->bus, timeout, ata_state_async_waiting);

		// having a too short data buffer shouldn't happen here
		// anyway - we are prepared
		SHOW_FLOW0(3, "Writing one block");
		if (write_PIO_block(request, 512) == B_ERROR)
			goto finish_cancel_timeout;

		--device->left_blocks;
	} else {
		if (device->left_blocks > 1) {
			// start async waiting for next command (see above)
			start_waiting_nolock(device->bus, timeout, ata_state_async_waiting);
		}

		// see write
		SHOW_FLOW0( 3, "Reading one block" );
		if (read_PIO_block(request, 512) == B_ERROR)
			goto finish_cancel_timeout;

		--device->left_blocks;

		if (device->left_blocks == 0) {
			// at end of transmission, wait for data ccb going low
			SHOW_FLOW0( 3, "Waiting for device to finish transmission" );

			if (!wait_for_drqdown(device))
				SHOW_FLOW0( 3, "Device continues data transmission - abort command" );

			// we don't cancel timeout as no timeout is started during last block
			goto finish;
		}
	}

	return;

finish_cancel_timeout:
	cancel_irq_timeout(device->bus);

finish:
	finish_checksense(request);
}

#endif
/** DPC called when IRQ was fired at end of DMA transmission */

void
ata_dpc_DMA(ata_request *request)
{
/*
	ide_device_info *device = request->device;
	bool dma_success, dev_err;

	dma_success = finish_dma(device);
	dev_err = check_rw_error(device, request);

	if (dma_success && !dev_err) {
		// reset error count if DMA worked
		device->DMA_failures = 0;
		request->ccb->data_resid = 0;
		finish_checksense(request);
	} else {
		SHOW_ERROR0( 2, "Error in DMA transmission" );

		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_FAILURE);

		if (++device->DMA_failures >= MAX_DMA_FAILURES) {
			SHOW_ERROR0( 2, "Disabled DMA because of too many errors" );
			device->DMA_enabled = false;
		}

		// reset queue in case queuing is active
		finish_reset_queue(request);
	}
*/
}


// list of LBA48 opcodes
static const uint8 cmd_48[2][2] = {
	{ IDE_CMD_READ_SECTORS_EXT, IDE_CMD_WRITE_SECTORS_EXT },
	{ IDE_CMD_READ_DMA_EXT, IDE_CMD_WRITE_DMA_EXT }
};


// list of normal LBA opcodes
static const uint8 cmd_28[2][2] = {
	{ IDE_CMD_READ_SECTORS, IDE_CMD_WRITE_SECTORS },
	{ IDE_CMD_READ_DMA, IDE_CMD_WRITE_DMA }
};


/** create IDE read/write command */

static bool
create_rw_taskfile(ide_device_info *device, ata_request *request, 
	uint64 pos, size_t length, bool write)
{
	SHOW_FLOW0( 3, "" );

	// XXX disable any writes
/*	if( write )
		goto err;*/

	if (device->use_LBA) {
		if (device->use_48bits && (pos + length > 0xfffffff || length > 0x100)) {
			// use LBA48 only if necessary
			SHOW_FLOW0( 3, "using LBA48" );

			if (length > 0xffff)
				goto err;

				// non-queued LBA48
				device->tf_param_mask = ide_mask_sector_count_48
					| ide_mask_LBA_low_48
					| ide_mask_LBA_mid_48
					| ide_mask_LBA_high_48;

				device->tf.lba48.sector_count_0_7 = length & 0xff;
				device->tf.lba48.sector_count_8_15 = (length >> 8) & 0xff;
				device->tf.lba48.lba_0_7 = pos & 0xff;
				device->tf.lba48.lba_8_15 = (pos >> 8) & 0xff;
				device->tf.lba48.lba_16_23 = (pos >> 16) & 0xff;
				device->tf.lba48.lba_24_31 = (pos >> 24) & 0xff;
				device->tf.lba48.lba_32_39 = (pos >> 32) & 0xff;
				device->tf.lba48.lba_40_47 = (pos >> 40) & 0xff;
				device->tf.lba48.command = cmd_48[request->uses_dma][write];
				return true;
		} else {
			// normal LBA
			SHOW_FLOW0(3, "using LBA");

			if (length > 0x100)
				goto err;

				// non-queued LBA
				SHOW_FLOW0( 3, "creating normal DMA/PIO command" );
				device->tf_param_mask = ide_mask_sector_count
					| ide_mask_LBA_low
					| ide_mask_LBA_mid
					| ide_mask_LBA_high
					| ide_mask_device_head;

				device->tf.lba.sector_count = length & 0xff;
				device->tf.lba.lba_0_7 = pos & 0xff;
				device->tf.lba.lba_8_15 = (pos >> 8) & 0xff;
				device->tf.lba.lba_16_23 = (pos >> 16) & 0xff;
				device->tf.lba.lba_24_27 = (pos >> 24) & 0xf;
				device->tf.lba.command = cmd_28[request->uses_dma][write];
				return true;
		}
	} else {
		// CHS mode
		// (probably, noone would notice if we'd dropped support)
		uint32 track_size, cylinder_offset, cylinder;
		ide_device_infoblock *infoblock = &device->infoblock;

		if (length > 0x100)
			goto err;

		device->tf.chs.mode = ide_mode_chs;

		device->tf_param_mask = ide_mask_sector_count
			| ide_mask_sector_number
			| ide_mask_cylinder_low
			| ide_mask_cylinder_high
			| ide_mask_device_head;

		device->tf.chs.sector_count = length & 0xff;

		track_size = infoblock->current_heads * infoblock->current_sectors;

		if (track_size == 0) {
			ata_request_set_sense(request, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_MEDIUM_FORMAT_CORRUPTED);
			return false;
		}

		cylinder = pos / track_size;

		device->tf.chs.cylinder_0_7 = cylinder & 0xff;
		device->tf.chs.cylinder_8_15 = (cylinder >> 8) & 0xff;

		cylinder_offset = pos - cylinder * track_size;

		device->tf.chs.sector_number = (cylinder_offset % infoblock->current_sectors + 1) & 0xff;
		device->tf.chs.head = cylinder_offset / infoblock->current_sectors;

		device->tf.chs.command = cmd_28[request->uses_dma][write];
		return true;
	}

	return true;

err:
	ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
	return false;
}


/**	execute read/write command
 *	pos - first block
 *	length - number of blocks
 */

void
ata_exec_read_write(ide_device_info *device, ata_request *request,
	uint64 address, size_t sectorCount, bool write)
{
	ide_bus_info *bus = device->bus;
	ata_flags flags = 0;
	bigtime_t timeout;

	// make a copy first as settings may get changed by user during execution	
	request->is_write = write;
	request->uses_dma = device->DMA_enabled;

	if (request->uses_dma) {
		if (!prepare_dma(device, request)) {
			// fall back to PIO on error
			request->uses_dma = false;
		}
	}

	if (!request->uses_dma) {
		prep_PIO_transfer(device, request);
		device->left_blocks = sectorCount;
	}

	// compose command
	if (!create_rw_taskfile(device, request, address, sectorCount, write))
		goto error;

	// if no timeout is specified, use standard
	timeout = request->ccb->timeout > 0 ? request->ccb->timeout * 1000 : IDE_STD_TIMEOUT;

	if (!device->is_atapi)
		flags |= ATA_DRDY_REQUIRED;
	if (request->uses_dma)
		flags |= ATA_DMA_TRANSFER;
	if (ata_send_command(device, request, flags, timeout) != B_OK) 
		goto error;

	// this could be executed in a DPC to improve performance
	if (request->uses_dma) {
		ata_exec_dma_transfer(request);
	} else {
		ata_exec_pio_transfer(request);
	}
	return;

error:
	if (request->uses_dma)
		abort_dma(device, request);
	ata_request_finish(request, false);
}


static void
ata_exec_dma_transfer(ata_request *request)
{
	ASSERT(0);
}

static void
ata_exec_pio_transfer(ata_request *request)
{
	ide_device_info *device = request->device;
	ide_bus_info *bus = device->bus;
	uint32 timeout = request->ccb->timeout > 0 ? 
		request->ccb->timeout * 1000 : IDE_STD_TIMEOUT;

//	FLOW("ata_exec_pio_transfer: length %d, left_blocks %d, left_sg_elem %d, cur_sg_ofs %d\n",
//		request->ccb->data_length, device->left_blocks, device->left_sg_elem, device->cur_sg_ofs);


	while (device->left_blocks > 0) {

		if (ata_wait(bus, ide_status_drq, ide_status_bsy, 0, 4000000) != B_OK) {
			TRACE("ata_exec_pio_transfer: wait failed\n");
			goto error;
		}

		if (request->is_write) {
//			FLOW("writing 1 block\n");
			if (write_PIO_block(request, 512) != B_OK)
				goto transfer_error;
		} else {
//			FLOW("reading 1 block\n");
			if (read_PIO_block(request, 512) != B_OK)
				goto transfer_error;
		}
		device->left_blocks--;

		// wait 1 pio cycle
		if (device->left_blocks)
			bus->controller->get_altstatus(bus->channel_cookie);
			
//		FLOW("%d blocks left\n", device->left_blocks);
	}

	if (ata_wait_for_drqdown(bus) != B_OK) {
		TRACE("ata_exec_pio_transfer: ata_wait_for_drqdown failed\n");
		goto error;
	}

	ata_finish_command(device, request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, ide_error_abrt);
	ata_request_finish(request, false /* resubmit */);
	return;

error:
	TRACE("ata_exec_pio_transfer: error\n");
	ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
	ata_request_finish(request, false /* resubmit */);
	return;

transfer_error:
	TRACE("ata_exec_pio_transfer: transfer error\n");
	ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
	ata_request_finish(request, false /* resubmit */);
}


/** check for errors reported by read/write command
 *	return: true, if an error occured
 */

bool
check_rw_error(ide_device_info *device, ata_request *request)
{
#if 0
	ide_bus_info *bus = device->bus;
	uint8 status;

	status = bus->controller->get_altstatus(bus->channel_cookie);

	if ((status & ide_status_err) != 0) {
		uint8 error;

		if (bus->controller->read_command_block_regs(bus->channel_cookie,
				&device->tf, ide_mask_error) != B_OK) {
			device->subsys_status = SCSI_HBA_ERR;
			return true;
		}

		error = device->tf.read.error;

		if ((error & ide_error_icrc) != 0) {
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_CRC);
			return true;
		}

		if (request->is_write) {
			if ((error & ide_error_wp) != 0) {
				set_sense(device, SCSIS_KEY_DATA_PROTECT, SCSIS_ASC_WRITE_PROTECTED);
				return true;
			}
		} else {
			if ((error & ide_error_unc) != 0) {
				set_sense(device, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_UNREC_READ_ERR);
				return true;
			}
		}

		if ((error & ide_error_mc) != 0) {
			set_sense(device, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_MEDIUM_CHANGED);
			return true;
		}

		if ((error & ide_error_idnf) != 0) {
			// ID not found - invalid CHS mapping (was: seek error?)
			set_sense(device, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_RANDOM_POS_ERROR);
			return true;
		}
		
		if ((error & ide_error_mcr) != 0) {
			// XXX proper sense key?
			// for TUR this case is not defined !?
			set_sense(device, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_REMOVAL_REQUESTED);
			return true;
		}

		if ((error & ide_error_nm) != 0) {
			set_sense(device, SCSIS_KEY_NOT_READY, SCSIS_ASC_NO_MEDIUM);
			return true;
		}

		if ((error & ide_error_abrt) != 0) {
			set_sense(device, SCSIS_KEY_ABORTED_COMMAND, SCSIS_ASC_NO_SENSE);
			return true;
		}

		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
		return true;
	}
#endif
	return false;
}


/** execute SET FEATURE command
 *	set subcommand in task file before calling this
 */

static bool
device_set_feature(ide_device_info *device, int feature)
{
	ata_request request;
	ata_request_init(&request, device);

	TRACE("device_set_feature: feature %d\n", feature);

	device->tf_param_mask = ide_mask_features;

	device->tf.write.features = feature;
	device->tf.write.command = IDE_CMD_SET_FEATURES;

	if (ata_send_command(device, &request, ATA_DRDY_REQUIRED, 1000000) != B_OK)
		return false;

	if (ata_finish_command(device, &request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, ide_error_abrt) != B_OK)
		return false;

	return true;
}


static bool
configure_rmsn(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;
	ata_request request;
	int i;

	if (!device->infoblock.RMSN_supported
		|| device->infoblock._127_RMSN_support != 1)
		return true;

	if (!device_set_feature(device, IDE_CMD_SET_FEATURES_ENABLE_MSN))
		return false;

	bus->controller->read_command_block_regs(bus->channel_cookie, &device->tf,
		ide_mask_LBA_mid | ide_mask_LBA_high);

	ata_request_init(&request, device);

	for (i = 0; i < 5; ++i) {
		// don't use TUR as it checks not ide_error_mcr | ide_error_mc | ide_error_wp
		// but: we don't check wp as well

		device->tf_param_mask = 0;
		device->tf.write.command = IDE_CMD_GET_MEDIA_STATUS;

		if (!ata_send_command(device, &request, ATA_DRDY_REQUIRED, 15000000))
			continue;

		if (ata_finish_command(device, &request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, 
			ide_error_nm | ide_error_abrt | ide_error_mcr | ide_error_mc) == B_OK
			|| ((request.senseAsc << 8) | request.senseAscq) == SCSIS_ASC_NO_MEDIUM)
			return true;

		ata_request_clear_sense(&request);
	}

	return false;
}


static bool
disable_command_queueing(ide_device_info *device)
{
	if (!device->infoblock.DMA_QUEUED_supported)
		return true;

	if (device->infoblock.RELEASE_irq_supported
		&& !device_set_feature( device, IDE_CMD_SET_FEATURES_DISABLE_REL_INT))
		dprintf("Cannot disable release irq\n");

	if (device->infoblock.SERVICE_irq_supported
		&& !device_set_feature(device, IDE_CMD_SET_FEATURES_DISABLE_SERV_INT))
		dprintf("Cannot disable service irq\n");

	return true;
}



status_t
configure_ata_device(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;
	uint32 chs_capacity;

	TRACE("configure_ata_device\n");

	device->is_atapi = false;
	device->exec_io = ata_exec_io;
	device->last_lun = 0;

	// warning: ata == 0 means "this is ata"...
	if (infoblock->_0.ata.ATA != 0) {
		// CF has either magic header or CFA bit set
		// we merge it to "CFA bit set" for easier (later) testing
		if (*(uint16 *)infoblock == 0x848a)
			infoblock->CFA_supported = true;
		else 
			return B_ERROR;
	}

	if (!infoblock->_54_58_valid) {
		// normally, current_xxx contains active CHS mapping,
		// but if BIOS didn't call INITIALIZE DEVICE PARAMETERS
		// the default mapping is used
		infoblock->current_sectors = infoblock->sectors;
		infoblock->current_cylinders = infoblock->cylinders;
		infoblock->current_heads = infoblock->heads;
	}

	// just in case capacity_xxx isn't initialized - calculate it manually
	// (seems that this information is really redundant; hopefully)
	chs_capacity = infoblock->current_sectors * infoblock->current_cylinders *
		infoblock->current_heads;

	infoblock->capacity_low = chs_capacity & 0xff;
	infoblock->capacity_high = chs_capacity >> 8;

	// checking LBA_supported flag should be sufficient, but it seems
	// that checking LBA_total_sectors is a good idea
	device->use_LBA = infoblock->LBA_supported && infoblock->LBA_total_sectors != 0;

	if (device->use_LBA) {
		device->total_sectors = infoblock->LBA_total_sectors;
		device->tf.lba.mode = ide_mode_lba;
	} else {
		device->total_sectors = chs_capacity;
		device->tf.chs.mode = ide_mode_chs;
	}

	device->use_48bits = infoblock->_48_bit_addresses_supported;

	if (device->use_48bits)
		device->total_sectors = infoblock->LBA48_total_sectors;

	if (!configure_dma(device)
		|| !disable_command_queueing(device)
		|| !configure_rmsn(device))
		return B_ERROR;

	return B_OK;
}


status_t
ata_identify_device(ide_device_info *device, bool isAtapi)
{
	ata_request request;
	ide_bus_info *bus = device->bus;

	TRACE("ata_identify_device: bus %p, device %d, isAtapi %d\n", device->bus, device->is_device1, isAtapi);

	ata_request_init(&request, device);

	ata_select(device);

	device->tf_param_mask = 0;
	device->tf.write.command = isAtapi ? IDE_CMD_IDENTIFY_PACKET_DEVICE : IDE_CMD_IDENTIFY_DEVICE;

	if (ata_send_command(device, &request, isAtapi ? 0 : ATA_DRDY_REQUIRED, 20000000) != B_OK) {
		TRACE("ata_identify_device: send_command failed\n");
		return B_ERROR;
	}

	if (ata_wait(bus, ide_status_drq, ide_status_bsy, 0, 4000000) != B_OK) {
		TRACE("ata_identify_device: wait failed\n");
		return B_ERROR;
	}

	// get the infoblock		
	bus->controller->read_pio(bus->channel_cookie, (uint16 *)&device->infoblock, 
		sizeof(device->infoblock) / sizeof(uint16), false);

	if (ata_wait_for_drqdown(bus) != B_OK) {
		TRACE("ata_identify_device: ata_wait_for_drqdown failed\n");
		return B_ERROR;
	}

	if (ata_finish_command(device, &request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, ide_error_abrt) != B_OK)
		return B_ERROR;

	TRACE("ata_identify_device: success\n");
	return B_OK;
}
