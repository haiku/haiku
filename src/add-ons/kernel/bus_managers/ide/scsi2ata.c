/*
 * Copyright 2004-2007, Haiku, Inc. All RightsReserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open IDE bus manager

	Converts SCSI commands to ATA commands.
*/


#include "ide_internal.h"
#include "ide_sim.h"
#include "ide_cmds.h"

#include <string.h>


/** emulate MODE SENSE 10 command */

static void
ata_mode_sense_10(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_cmd_mode_sense_10 *cmd = (scsi_cmd_mode_sense_10 *)request->cdb;
	scsi_mode_param_header_10 param_header;
	scsi_modepage_control control;
	scsi_mode_param_block_desc block_desc;
	size_t totalLength = sizeof(scsi_mode_param_header_10)
		+ sizeof(scsi_mode_param_block_desc)
		+ sizeof(scsi_modepage_control);
	scsi_mode_param_dev_spec_da devspec = {
		_res0_0 : 0,
		dpo_fua : 0,
		_res0_6 : 0,
		write_protected : 0
	};
	uint32 allocationLength;

	SHOW_FLOW0(1, "Hi!");

	allocationLength = B_BENDIAN_TO_HOST_INT16(cmd->allocation_length);

	// we answer control page requests and "all pages" requests
	// (as the latter are the same as the first)
	if ((cmd->page_code != SCSI_MODEPAGE_CONTROL && cmd->page_code != SCSI_MODEPAGE_ALL)
		|| (cmd->page_control != SCSI_MODE_SENSE_PC_CURRENT
			&& cmd->page_control != SCSI_MODE_SENSE_PC_SAVED)) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	//param_header = (scsi_mode_param_header_10 *)request->data;
	param_header.mode_data_length = B_HOST_TO_BENDIAN_INT16(totalLength - 1);
	param_header.medium_type = 0; 		// XXX standard is a bit vague here
	param_header.dev_spec_parameter = *(uint8 *)&devspec;
	param_header.block_desc_length
		= B_HOST_TO_BENDIAN_INT16(sizeof(scsi_mode_param_block_desc));

	copy_sg_data(request, 0, allocationLength, &param_header,
		sizeof(param_header), false);

	/*block_desc = (scsi_mode_param_block_desc *)(request->data
		+ sizeof(*param_header));*/
	memset(&block_desc, 0, sizeof(block_desc));
	// density is reserved (0), descriptor apply to entire medium (num_blocks=0)
	// remains the blocklen to be set
	block_desc.high_blocklen = 0;
	block_desc.med_blocklen = 512 >> 8;
	block_desc.low_blocklen = 512 & 0xff;

	copy_sg_data(request, sizeof(param_header), allocationLength,
		&block_desc, sizeof(block_desc), false);

	/*contr = (scsi_modepage_contr *)(request->data
		+ sizeof(*param_header)
		+ ((uint16)param_header->high_block_desc_len << 8)
		+ param_header->low_block_desc_len);*/

	memset(&control, 0, sizeof(control));
	control.RLEC = false;
	control.DQue = !device->CQ_enabled;
	control.QErr = false;
		// when a command fails we requeue all
		// lost commands automagically
	control.QAM = SCSI_QAM_UNRESTRICTED;

	copy_sg_data(request, sizeof(param_header)
		+ B_BENDIAN_TO_HOST_INT16(param_header.block_desc_length),
		allocationLength, &control, sizeof(control), false);

	// the number of bytes that were transferred to buffer is
	// restricted by allocation length and by request data buffer size
	totalLength = min(totalLength, allocationLength);
	totalLength = min(totalLength, request->data_length);

	request->data_resid = request->data_length - totalLength;
}


/*! Emulate modifying control page */
static bool
ata_mode_select_control_page(ide_device_info *device, ide_qrequest *qrequest,
	scsi_modepage_control *page)
{
	if (page->header.page_length != sizeof(*page) - sizeof(page->header)) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR);
		return false;
	}

	// we only support enabling/disabling command queuing
	enable_CQ(device, !page->DQue);
	return true;
}


/*! Emulate MODE SELECT 10 command */
static void
ata_mode_select_10(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_cmd_mode_select_10 *cmd = (scsi_cmd_mode_select_10 *)request->cdb;
	scsi_mode_param_header_10 param_header;
	scsi_modepage_header page_header;
	uint32 totalLength;
	uint32 modepageOffset;
	char modepage_buffer[64];	// !!! enlarge this to support longer mode pages

	if (cmd->save_pages || cmd->pf != 1) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	totalLength = min(request->data_length,
		B_BENDIAN_TO_HOST_INT16(cmd->param_list_length));

	// first, retrieve page header to get size of different chunks
	//param_header = (scsi_mode_param_header_10 *)request->data;
	if (!copy_sg_data(request, 0, totalLength, &param_header, sizeof(param_header), true))
		goto err;

	totalLength = min(totalLength,
		B_BENDIAN_TO_HOST_INT16(param_header.mode_data_length) + 1UL);

	// this is the start of the first mode page;
	// we ignore the block descriptor silently
	modepageOffset = sizeof(param_header)
		+ B_BENDIAN_TO_HOST_INT16(param_header.block_desc_length);

	// go through list of pages
	while (modepageOffset < totalLength) {
		uint32 pageLength;

		// get header to know how long page is
		if (!copy_sg_data(request, modepageOffset, totalLength,
				&page_header, sizeof(page_header), true))
			goto err;

		// get size of one page and copy it to buffer
		pageLength = page_header.page_length + sizeof(scsi_modepage_header);

		// the buffer has a maximum size - this is really standard compliant but
		// sufficient for our needs
		if (pageLength > sizeof(modepage_buffer))
			goto err;

		if (!copy_sg_data(request, modepageOffset, totalLength,
				&modepage_buffer, min(pageLength, sizeof(modepage_buffer)), true))
			goto err;

		// modify page;
		// currently, we only support the control mode page
		switch (page_header.page_code) {
			case SCSI_MODEPAGE_CONTROL:
				if (!ata_mode_select_control_page(device, qrequest,
						(scsi_modepage_control *)modepage_buffer))
					return;
				break;

			default:
				set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST,
					SCSIS_ASC_INV_PARAM_LIST_FIELD);
				return;
		}

		modepageOffset += pageLength;
	}

	if (modepageOffset != totalLength)
		goto err;

	request->data_resid = request->data_length - totalLength;
	return;

	// if we arrive here, data length was incorrect
err:
	set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR);
}


/*! Emulate TEST UNIT READY */
static bool
ata_test_unit_ready(ide_device_info *device, ide_qrequest *qrequest)
{
	SHOW_FLOW0(3, "");

	if (!device->infoblock.RMSN_supported
		|| device->infoblock._127_RMSN_support != 1)
		return true;

	// ask device about status
	device->tf_param_mask = 0;
	device->tf.write.command = IDE_CMD_GET_MEDIA_STATUS;

	if (!send_command(device, qrequest, true, 15, ide_state_sync_waiting))
		return false;

	// bits ide_error_mcr | ide_error_mc | ide_error_wp are also valid
	// but not requested by TUR; ide_error_wp can safely be ignored, but
	// we don't want to loose media change (request) reports
	if (!check_output(device, true,
			ide_error_nm | ide_error_abrt | ide_error_mcr | ide_error_mc,
			false)) {
		// SCSI spec is unclear here: we shouldn't report "media change (request)"
		// but what to do if there is one? anyway - we report them
		;
	}

	return true;
}


/*! Flush internal device cache */
static bool
ata_flush_cache(ide_device_info *device, ide_qrequest *qrequest)
{
	// we should also ask for FLUSH CACHE support, but everyone denies it
	// (looks like they cheat to gain some performance advantage, but
	//  that's pretty useless: everyone does it...)
	if (!device->infoblock.write_cache_supported)
		return true;

	device->tf_param_mask = 0;
	device->tf.lba.command = device->use_48bits ? IDE_CMD_FLUSH_CACHE_EXT
		: IDE_CMD_FLUSH_CACHE;

	// spec says that this may take more then 30s, how much more?
	if (!send_command(device, qrequest, true, 60, ide_state_sync_waiting))
		return false;

	wait_for_sync(device->bus);

	return check_output(device, true, ide_error_abrt, false);
}


/*!	Load or eject medium
	load = true - load medium
*/
static bool
ata_load_eject(ide_device_info *device, ide_qrequest *qrequest, bool load)
{
	if (load) {
		// ATA doesn't support loading
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_NOT_SUPPORTED);
		return false;
	}

	device->tf_param_mask = 0;
	device->tf.lba.command = IDE_CMD_MEDIA_EJECT;

	if (!send_command(device, qrequest, true, 15, ide_state_sync_waiting))
		return false;

	wait_for_sync(device->bus);

	return check_output(device, true, ide_error_abrt | ide_error_nm, false);
}


/*! Emulate PREVENT ALLOW command */
static bool
ata_prevent_allow(ide_device_info *device, bool prevent)
{
	set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_ILL_FUNCTION);
	return false;
}


/*! Emulate INQUIRY command */
static void
ata_inquiry(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_res_inquiry data;
	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)request->cdb;
	uint32 allocation_length = cmd->allocation_length;
	uint32 transfer_size;

	if (cmd->evpd || cmd->page_code) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	memset(&data, 0, sizeof(data));

	data.device_type = scsi_dev_direct_access;
	data.device_qualifier = scsi_periph_qual_connected;

	data.device_type_modifier = 0;
	data.removable_medium = false;

	data.ansi_version = 2;
	data.ecma_version = 0;
	data.iso_version = 0;

	data.response_data_format = 2;
	data.term_iop = false;
		// to be changed if we support TERM I/O

	data.additional_length = sizeof(scsi_res_inquiry) - 4;

	data.soft_reset = false;
	data.cmd_queue = device->queue_depth > 1;
	data.linked = false;

	// these values are free-style
	data.sync = false;
	data.write_bus16 = true;
	data.write_bus32 = false;

	data.relative_address = false;

	// the following fields are *much* to small, sigh...
	memcpy(data.vendor_ident, device->infoblock.model_number,
		sizeof(data.vendor_ident));
	memcpy(data.product_ident, device->infoblock.model_number + 8,
		sizeof(data.product_ident));
	memcpy(data.product_rev, "    ", sizeof(data.product_rev));

	copy_sg_data(request, 0, allocation_length, &data, sizeof(data), false);

	transfer_size = min(sizeof(data), allocation_length);
	transfer_size = min(transfer_size, request->data_length);

	request->data_resid = request->data_length - transfer_size;
}


/*! Emulate READ CAPACITY command */
static void
read_capacity(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_res_read_capacity data;
	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	uint32 lastBlock;

	if (cmd->pmi || cmd->lba) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	// TODO: 512 bytes fixed block size?
	data.block_size = B_HOST_TO_BENDIAN_INT32(512);

	lastBlock = device->total_sectors - 1;
	data.lba = B_HOST_TO_BENDIAN_INT32(lastBlock);

	copy_sg_data(request, 0, request->data_length, &data, sizeof(data), false);
	request->data_resid = max(request->data_length - sizeof(data), 0);
}


/*! Execute SCSI command */
void
ata_exec_io(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;

	SHOW_FLOW(3, "command=%x", request->cdb[0]);

	// ATA devices have one LUN only
	if (request->target_lun != 0) {
		request->subsys_status = SCSI_SEL_TIMEOUT;
		finish_request(qrequest, false);
		return;
	}

	// starting a request means deleting sense, so don't do it if
	// the command wants to read it
	if (request->cdb[0] != SCSI_OP_REQUEST_SENSE)
		start_request(device, qrequest);

	switch (request->cdb[0]) {
		case SCSI_OP_TEST_UNIT_READY:
			ata_test_unit_ready(device, qrequest);
			break;

		case SCSI_OP_REQUEST_SENSE:
			ide_request_sense(device, qrequest);
			return;

		case SCSI_OP_FORMAT: /* FORMAT UNIT */
			// we could forward request to disk, but modern disks cannot
			// be formatted anyway, so we just refuse request
			// (exceptions are removable media devices, but to my knowledge
			// they don't have to be formatted as well)
			set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_INQUIRY:
			ata_inquiry(device, qrequest);
			break;

		case SCSI_OP_MODE_SELECT_10:
			ata_mode_select_10(device, qrequest);
			break;

		case SCSI_OP_MODE_SENSE_10:
			ata_mode_sense_10(device, qrequest);
			break;

		case SCSI_OP_MODE_SELECT_6:
		case SCSI_OP_MODE_SENSE_6:
			// we've told SCSI bus manager to emulates these commands
			set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_RESERVE:
		case SCSI_OP_RELEASE:
			// though mandatory, this doesn't make much sense in a
			// single initiator environment; so what
			set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_START_STOP: {
			scsi_cmd_ssu *cmd = (scsi_cmd_ssu *)request->cdb;

			// with no LoEj bit set, we should only allow/deny further access
			// we ignore that (unsupported for ATA)
			// with LoEj bit set, we should additionally either load or eject the medium
			// (start = 0 - eject; start = 1 - load)

			if (!cmd->start)
				// we must always flush cache if start = 0
				ata_flush_cache(device, qrequest);

			if (cmd->load_eject)
				ata_load_eject(device, qrequest, cmd->start);

			break;
		}

		case SCSI_OP_PREVENT_ALLOW: {
			scsi_cmd_prevent_allow *cmd = (scsi_cmd_prevent_allow *)request->cdb;

			ata_prevent_allow(device, cmd->prevent);
			break;
		}

		case SCSI_OP_READ_CAPACITY:
			read_capacity(device, qrequest);
			break;

		case SCSI_OP_VERIFY_6:
			// does anyone uses this function?
			// effectly, it does a read-and-compare, which IDE doesn't support
			set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_SYNCHRONIZE_CACHE:
			// we ignore range and immediate bit, we always immediately flush everything
			ata_flush_cache(device, qrequest);
			break;

		// sadly, there are two possible read/write operation codes;
		// at least, the third one, read/write(12), is not valid for DAS
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;
			uint32 pos;
			size_t length;

			pos = ((uint32)cmd->high_lba << 16) | ((uint32)cmd->mid_lba << 8)
				| (uint32)cmd->low_lba;
			length = cmd->length != 0 ? cmd->length : 256;

			SHOW_FLOW(3, "READ6/WRITE6 pos=%lx, length=%lx", pos, length);

			ata_send_rw(device, qrequest, pos, length, cmd->opcode == SCSI_OP_WRITE_6);
			return;
		}

		case SCSI_OP_READ_10:
		case SCSI_OP_WRITE_10:
		{
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;
			uint32 pos;
			size_t length;

			pos = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			length = B_BENDIAN_TO_HOST_INT16(cmd->length);

			if (length != 0) {
				ata_send_rw(device, qrequest, pos, length, cmd->opcode == SCSI_OP_WRITE_10);
			} else {
				// we cannot transfer zero blocks (apart from LBA48)
				finish_request(qrequest, false);
			}
			return;
		}

		default:
			set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
	}

	finish_checksense(qrequest);
}
