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


/** verify that device is ready for further PIO transmission */

static bool
check_rw_status(ide_device_info *device, bool drqStatus)
{
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

	return true;
}


/**	DPC called at
 *	 - begin of each PIO read/write block
 *	 - end of PUI write transmission
 */

void
ata_dpc_PIO(ide_qrequest *qrequest)
{
	ide_device_info *device = qrequest->device;
	uint32 timeout = qrequest->request->timeout > 0 ?
		qrequest->request->timeout : IDE_STD_TIMEOUT;

	SHOW_FLOW0(3, "");

	if (check_rw_error(device, qrequest)
		|| !check_rw_status(device, qrequest->is_write ? device->left_blocks > 0 : true))
	{
		// failure reported by device
		SHOW_FLOW0( 3, "command finished unsuccessfully" );

		finish_checksense(qrequest);
		return;
	}

	if (qrequest->is_write) {
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
		start_waiting_nolock(device->bus, timeout, ide_state_async_waiting);

		// having a too short data buffer shouldn't happen here
		// anyway - we are prepared
		SHOW_FLOW0(3, "Writing one block");
		if (write_PIO_block(qrequest, 512) == B_ERROR)
			goto finish_cancel_timeout;

		--device->left_blocks;
	} else {
		if (device->left_blocks > 1) {
			// start async waiting for next command (see above)
			start_waiting_nolock(device->bus, timeout, ide_state_async_waiting);
		}

		// see write
		SHOW_FLOW0( 3, "Reading one block" );
		if (read_PIO_block(qrequest, 512) == B_ERROR)
			goto finish_cancel_timeout;

		--device->left_blocks;

		if (device->left_blocks == 0) {
			// at end of transmission, wait for data request going low
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
	finish_checksense(qrequest);
}


/** DPC called when IRQ was fired at end of DMA transmission */

void
ata_dpc_DMA(ide_qrequest *qrequest)
{
	ide_device_info *device = qrequest->device;
	bool dma_success, dev_err;

	dma_success = finish_dma(device);
	dev_err = check_rw_error(device, qrequest);

	if (dma_success && !dev_err) {
		// reset error count if DMA worked
		device->DMA_failures = 0;
		device->CQ_failures = 0;
		qrequest->request->data_resid = 0;
		finish_checksense(qrequest);
	} else {
		SHOW_ERROR0( 2, "Error in DMA transmission" );

		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_FAILURE);

		if (++device->DMA_failures >= MAX_DMA_FAILURES) {
			SHOW_ERROR0( 2, "Disabled DMA because of too many errors" );
			device->DMA_enabled = false;
		}

		// reset queue in case queuing is active
		finish_reset_queue(qrequest);
	}
}


// list of LBA48 opcodes
static uint8 cmd_48[2][2] = {
	{ IDE_CMD_READ_SECTORS_EXT, IDE_CMD_WRITE_SECTORS_EXT },
	{ IDE_CMD_READ_DMA_EXT, IDE_CMD_WRITE_DMA_EXT }
};


// list of normal LBA opcodes
static uint8 cmd_28[2][2] = {
	{ IDE_CMD_READ_SECTORS, IDE_CMD_WRITE_SECTORS },
	{ IDE_CMD_READ_DMA, IDE_CMD_WRITE_DMA }
};


/** create IDE read/write command */

static bool
create_rw_taskfile(ide_device_info *device, ide_qrequest *qrequest,
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

			if (qrequest->queuable) {
				// queued LBA48
				device->tf_param_mask = ide_mask_features_48
					| ide_mask_sector_count
					| ide_mask_LBA_low_48
					| ide_mask_LBA_mid_48
					| ide_mask_LBA_high_48;

				device->tf.queued48.sector_count_0_7 = length & 0xff;
				device->tf.queued48.sector_count_8_15 = (length >> 8) & 0xff;
				device->tf.queued48.tag = qrequest->tag;
				device->tf.queued48.lba_0_7 = pos & 0xff;
				device->tf.queued48.lba_8_15 = (pos >> 8) & 0xff;
				device->tf.queued48.lba_16_23 = (pos >> 16) & 0xff;
				device->tf.queued48.lba_24_31 = (pos >> 24) & 0xff;
				device->tf.queued48.lba_32_39 = (pos >> 32) & 0xff;
				device->tf.queued48.lba_40_47 = (pos >> 40) & 0xff;
				device->tf.queued48.command = write ? IDE_CMD_WRITE_DMA_QUEUED_EXT
					: IDE_CMD_READ_DMA_QUEUED_EXT;
				return true;
			} else {
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
				device->tf.lba48.command = cmd_48[qrequest->uses_dma][write];
				return true;
			}
		} else {
			// normal LBA
			SHOW_FLOW0(3, "using LBA");

			if (length > 0x100)
				goto err;

			if (qrequest->queuable) {
				// queued LBA
				SHOW_FLOW( 3, "creating DMA queued command, tag=%d", qrequest->tag );
				device->tf_param_mask = ide_mask_features
					| ide_mask_sector_count
					| ide_mask_LBA_low
					| ide_mask_LBA_mid
					| ide_mask_LBA_high
					| ide_mask_device_head;

				device->tf.queued.sector_count = length & 0xff;
				device->tf.queued.tag = qrequest->tag;
				device->tf.queued.lba_0_7 = pos & 0xff;
				device->tf.queued.lba_8_15 = (pos >> 8) & 0xff;
				device->tf.queued.lba_16_23 = (pos >> 16) & 0xff;
				device->tf.queued.lba_24_27 = (pos >> 24) & 0xf;
				device->tf.queued.command = write ? IDE_CMD_WRITE_DMA_QUEUED
					: IDE_CMD_READ_DMA_QUEUED;
				return true;
			} else {
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
				device->tf.lba.command = cmd_28[qrequest->uses_dma][write];
				return true;
			}
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
			set_sense(device,
				SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_MEDIUM_FORMAT_CORRUPTED);
			return false;
		}

		cylinder = pos / track_size;

		device->tf.chs.cylinder_0_7 = cylinder & 0xff;
		device->tf.chs.cylinder_8_15 = (cylinder >> 8) & 0xff;

		cylinder_offset = pos - cylinder * track_size;

		device->tf.chs.sector_number = (cylinder_offset % infoblock->current_sectors + 1) & 0xff;
		device->tf.chs.head = cylinder_offset / infoblock->current_sectors;

		device->tf.chs.command = cmd_28[qrequest->uses_dma][write];
		return true;
	}

	return true;

err:
	set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
	return false;
}


/**	execute read/write command
 *	pos - first block
 *	length - number of blocks
 */

void
ata_send_rw(ide_device_info *device, ide_qrequest *qrequest,
	uint64 pos, size_t length, bool write)
{
	ide_bus_info *bus = device->bus;
	uint32 timeout;

	// make a copy first as settings may get changed by user during execution
	qrequest->is_write = write;
	qrequest->uses_dma = device->DMA_enabled;

	if (qrequest->uses_dma) {
		if (!prepare_dma(device, qrequest)) {
			// fall back to PIO on error

			// if command queueing is used and there is another command
			// already running, we cannot fallback to PIO immediately -> declare
			// command as not queuable and resubmit it, so the scsi bus manager
			// will block other requests on retry
			// (XXX this is not fine if the caller wants to recycle the CCB)
			if (device->num_running_reqs > 1) {
				qrequest->request->flags &= ~SCSI_ORDERED_QTAG;
				finish_retry(qrequest);
				return;
			}

			qrequest->uses_dma = false;
		}
	}

	if (!qrequest->uses_dma) {
		prep_PIO_transfer(device, qrequest);
		device->left_blocks = length;
	}

	// compose command
	if (!create_rw_taskfile(device, qrequest, pos, length, write))
		goto err_setup;

	// if no timeout is specified, use standard
	timeout = qrequest->request->timeout > 0 ?
		qrequest->request->timeout : IDE_STD_TIMEOUT;

	// in DMA mode, we continue with "accessing",
	// on PIO read, we continue with "async waiting"
	// on PIO write, we continue with "accessing"
	if (!send_command(device, qrequest, !device->is_atapi, timeout,
			(!qrequest->uses_dma && !qrequest->is_write) ?
				ide_state_async_waiting : ide_state_accessing))
		goto err_send;

	if (qrequest->uses_dma) {
		// if queuing used, we have to ask device first whether it wants
		// to postpone the command
		// XXX: using the bus release IRQ we don't have to busy wait for
		// a response, but I heard that IBM drives have problems with
		// that IRQ; to be evaluated
		if (qrequest->queuable) {
			if (!wait_for_drdy(device))
				goto err_send;

			if (check_rw_error(device, qrequest))
				goto err_send;

			if (device_released_bus(device)) {
				// device enqueued command, so we have to wait;
				// in access_finished, we'll ask device whether it wants to
				// continue some other command
				bus->active_qrequest = NULL;

				access_finished(bus, device);
				// we may have rejected commands meanwhile, so tell
				// the SIM that it can resend them now
				scsi->cont_send_bus(bus->scsi_cookie);
				return;
			}

			//SHOW_ERROR0( 2, "device executes command instantly" );
		}

		start_dma_wait_no_lock(device, qrequest);
	} else {
		// on PIO read, we start with waiting, on PIO write we can
		// transmit data immediately; we let the service thread do
		// the writing, so the caller can issue the next command
		// immediately (this optimisation really pays on SMP systems
		// only)
		SHOW_FLOW0(3, "Ready for PIO");
		if (qrequest->is_write) {
			SHOW_FLOW0(3, "Scheduling write DPC");
			scsi->schedule_dpc(bus->scsi_cookie, bus->irq_dpc, ide_dpc, bus);
		}
	}

	return;

err_setup:
	// error during setup
	if (qrequest->uses_dma)
		abort_dma(device, qrequest);

	finish_checksense(qrequest);
	return;

err_send:
	// error during/after send;
	// in this case, the device discards queued request automatically
	if (qrequest->uses_dma)
		abort_dma(device, qrequest);

	finish_reset_queue(qrequest);
}


/** check for errors reported by read/write command
 *	return: true, if an error occured
 */

bool
check_rw_error(ide_device_info *device, ide_qrequest *qrequest)
{
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

		if (qrequest->is_write) {
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

	return false;
}


/** check result of ATA command
 *	drdy_required - true if drdy must be set by device
 *	error_mask - bits to be checked in error register
 *	is_write - true, if command was a write command
 */

bool
check_output(ide_device_info *device, bool drdy_required,
	int error_mask, bool is_write)
{
	ide_bus_info *bus = device->bus;
	uint8 status;

	// check IRQ timeout
	if (bus->sync_wait_timeout) {
		bus->sync_wait_timeout = false;

		device->subsys_status = SCSI_CMD_TIMEOUT;
		return false;
	}

	status = bus->controller->get_altstatus(bus->channel_cookie);

	// if device is busy, other flags are indeterminate
	if ((status & ide_status_bsy) != 0) {
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		return false;
	}

	if (drdy_required && ((status & ide_status_drdy) == 0)) {
		device->subsys_status = SCSI_SEQUENCE_FAIL;
		return false;
	}

	if ((status & ide_status_err) != 0) {
		uint8 error;

		if (bus->controller->read_command_block_regs(bus->channel_cookie,
				&device->tf, ide_mask_error) != B_OK) {
			device->subsys_status = SCSI_HBA_ERR;
			return false;
		}

		error = device->tf.read.error & error_mask;

		if ((error & ide_error_icrc) != 0) {
			set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_LUN_COM_CRC);
			return false;
		}

		if (is_write) {
			if ((error & ide_error_wp) != 0) {
				set_sense(device, SCSIS_KEY_DATA_PROTECT, SCSIS_ASC_WRITE_PROTECTED);
				return false;
			}
		} else {
			if ((error & ide_error_unc) != 0) {
				set_sense(device, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_UNREC_READ_ERR);
				return false;
			}
		}

		if ((error & ide_error_mc) != 0) {
			// XXX proper sense key?
			set_sense(device, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_MEDIUM_CHANGED);
			return false;
		}

		if ((error & ide_error_idnf) != 0) {
			// XXX strange error code, don't really know what it means
			set_sense(device, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_RANDOM_POS_ERROR);
			return false;
		}

		if ((error & ide_error_mcr) != 0) {
			// XXX proper sense key?
			set_sense(device, SCSIS_KEY_UNIT_ATTENTION, SCSIS_ASC_REMOVAL_REQUESTED);
			return false;
		}

		if ((error & ide_error_nm) != 0) {
			set_sense(device, SCSIS_KEY_MEDIUM_ERROR, SCSIS_ASC_NO_MEDIUM);
			return false;
		}

		if ((error & ide_error_abrt) != 0) {
			set_sense(device, SCSIS_KEY_ABORTED_COMMAND, SCSIS_ASC_NO_SENSE);
			return false;
		}

		// either there was no error bit set or it was masked out
		set_sense(device, SCSIS_KEY_HARDWARE_ERROR, SCSIS_ASC_INTERNAL_FAILURE);
		return false;
	}

	return true;
}


/** execute SET FEATURE command
 *	set subcommand in task file before calling this
 */

static bool
device_set_feature(ide_device_info *device, int feature)
{
	device->tf_param_mask = ide_mask_features;

	device->tf.write.features = feature;
	device->tf.write.command = IDE_CMD_SET_FEATURES;

	if (!send_command(device, NULL, true, 1, ide_state_sync_waiting))
		return false;

	wait_for_sync(device->bus);

	return check_output(device, true, ide_error_abrt, false);
}


static bool
configure_rmsn(ide_device_info *device)
{
	ide_bus_info *bus = device->bus;
	int i;

	if (!device->infoblock.RMSN_supported
		|| device->infoblock._127_RMSN_support != 1)
		return true;

	if (!device_set_feature(device, IDE_CMD_SET_FEATURES_ENABLE_MSN))
		return false;

	bus->controller->read_command_block_regs(bus->channel_cookie, &device->tf,
		ide_mask_LBA_mid | ide_mask_LBA_high);

	for (i = 0; i < 5; ++i) {
		// don't use TUR as it checks not ide_error_mcr | ide_error_mc | ide_error_wp
		// but: we don't check wp as well
		device->combined_sense = 0;

		device->tf_param_mask = 0;
		device->tf.write.command = IDE_CMD_GET_MEDIA_STATUS;

		if (!send_command(device, NULL, true, 15, ide_state_sync_waiting))
			continue;

		if (check_output(device, true,
				ide_error_nm | ide_error_abrt | ide_error_mcr | ide_error_mc,
				true)
			|| decode_sense_asc_ascq(device->combined_sense) == SCSIS_ASC_NO_MEDIUM)
			return true;
	}

	return false;
}


static bool
configure_command_queueing(ide_device_info *device)
{
	device->CQ_enabled = device->CQ_supported = false;

	if (!device->bus->can_CQ
		|| !device->infoblock.DMA_QUEUED_supported)
		return initialize_qreq_array(device, 1);

	if (device->infoblock.RELEASE_irq_supported
		&& !device_set_feature( device, IDE_CMD_SET_FEATURES_DISABLE_REL_INT))
		dprintf("Cannot disable release irq\n");

	if (device->infoblock.SERVICE_irq_supported
		&& !device_set_feature(device, IDE_CMD_SET_FEATURES_DISABLE_SERV_INT))
		dprintf("Cannot disable service irq\n");

	device->CQ_enabled = device->CQ_supported = true;

	SHOW_INFO0(2, "Enabled command queueing");

	// official IBM docs talk about 31 queue entries, though
	// their disks report 32; let's hope their docs are wrong
	return initialize_qreq_array(device, device->infoblock.queue_depth + 1);
}


bool
prep_ata(ide_device_info *device)
{
	ide_device_infoblock *infoblock = &device->infoblock;
	uint32 chs_capacity;

	SHOW_FLOW0(3, "");

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
			return false;
	}

	SHOW_FLOW0(3, "1");

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

	SHOW_FLOW0(3, "2");

	if (!configure_dma(device)
		|| !configure_command_queueing(device)
		|| !configure_rmsn(device))
		return false;

	SHOW_FLOW0(3, "3");

	return true;
}


void
enable_CQ(ide_device_info *device, bool enable)
{
}
