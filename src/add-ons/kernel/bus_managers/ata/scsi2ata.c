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

#define TRACE	dprintf
#define FLOW	dprintf



// set sense buffer according to device sense in ata_request
void
scsi_set_sense(scsi_sense *sense, const ata_request *request)
{
	memset(sense, 0, sizeof(*sense));

	sense->error_code = SCSIS_CURR_ERROR;
	sense->sense_key = request->senseKey;
	sense->add_sense_length = sizeof(*sense) - 7;
	sense->asc = request->senseAsc;
	sense->ascq = request->senseAscq;
	sense->sense_key_spec.raw.SKSV = 0;	// no additional info
}


static void
scsi_mode_sense_10(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_cmd_mode_sense_10 *cmd = (scsi_cmd_mode_sense_10 *)ccb->cdb;
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
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	//param_header = (scsi_mode_param_header_10 *)ccb->data;
	param_header.mode_data_length = B_HOST_TO_BENDIAN_INT16(totalLength - 1);
	param_header.medium_type = 0; 		// XXX standard is a bit vague here
	param_header.dev_spec_parameter = *(uint8 *)&devspec;
	param_header.block_desc_length
		= B_HOST_TO_BENDIAN_INT16(sizeof(scsi_mode_param_block_desc));

	copy_sg_data(ccb, 0, allocationLength, &param_header,
		sizeof(param_header), false);

	/*block_desc = (scsi_mode_param_block_desc *)(ccb->data 
		+ sizeof(*param_header));*/
	memset(&block_desc, 0, sizeof(block_desc));
	// density is reserved (0), descriptor apply to entire medium (num_blocks=0)
	// remains the blocklen to be set
	block_desc.high_blocklen = 0;
	block_desc.med_blocklen = 512 >> 8;
	block_desc.low_blocklen = 512 & 0xff;

	copy_sg_data(ccb, sizeof(param_header), allocationLength,
		&block_desc, sizeof(block_desc), false);

	/*contr = (scsi_modepage_contr *)(ccb->data 
		+ sizeof(*param_header)
		+ ((uint16)param_header->high_block_desc_len << 8) 
		+ param_header->low_block_desc_len);*/

	memset(&control, 0, sizeof(control));
	control.RLEC = false;
	control.DQue = 1;//!device->CQ_enabled;
	control.QErr = false;
		// when a command fails we requeue all 
		// lost commands automagically
	control.QAM = SCSI_QAM_UNRESTRICTED;

	copy_sg_data(ccb, sizeof(param_header)
		+ B_BENDIAN_TO_HOST_INT16(param_header.block_desc_length),
		allocationLength, &control, sizeof(control), false);

	// the number of bytes that were transferred to buffer is
	// restricted by allocation length and by ccb data buffer size
	totalLength = min(totalLength, allocationLength);
	totalLength = min(totalLength, ccb->data_length);

	ccb->data_resid = ccb->data_length - totalLength;
}


/*! Emulate modifying control page */
static bool
ata_mode_select_control_page(ide_device_info *device, ata_request *request,
	scsi_modepage_control *page)
{
	if (page->header.page_length != sizeof(*page) - sizeof(page->header)) {
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR);
		return false;
	}

	// we only support enabling/disabling command queuing
//	enable_CQ(device, !page->DQue);
	return true;
}


/*! Emulate MODE SELECT 10 command */
static void
scsi_mode_select_10(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_cmd_mode_select_10 *cmd = (scsi_cmd_mode_select_10 *)ccb->cdb;
	scsi_mode_param_header_10 param_header;
	scsi_modepage_header page_header;
	uint32 totalLength;
	uint32 modepageOffset;
	char modepage_buffer[64];	// !!! enlarge this to support longer mode pages

	if (cmd->save_pages || cmd->pf != 1) {
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	totalLength = min(ccb->data_length,
		B_BENDIAN_TO_HOST_INT16(cmd->param_list_length));

	// first, retrieve page header to get size of different chunks
	//param_header = (scsi_mode_param_header_10 *)ccb->data;
	if (!copy_sg_data(ccb, 0, totalLength, &param_header, sizeof(param_header), true))
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
		if (!copy_sg_data(ccb, modepageOffset, totalLength,
				&page_header, sizeof(page_header), true))
			goto err;

		// get size of one page and copy it to buffer
		pageLength = page_header.page_length + sizeof(scsi_modepage_header);

		// the buffer has a maximum size - this is really standard compliant but
		// sufficient for our needs
		if (pageLength > sizeof(modepage_buffer))
			goto err;

		if (!copy_sg_data(ccb, modepageOffset, totalLength,
				&modepage_buffer, min(pageLength, sizeof(modepage_buffer)), true))
			goto err;

		// modify page;
		// currently, we only support the control mode page
		switch (page_header.page_code) {
			case SCSI_MODEPAGE_CONTROL:
				if (!ata_mode_select_control_page(device, request,
						(scsi_modepage_control *)modepage_buffer))
					return;
				break;

			default:
				ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_PARAM_LIST_FIELD);
				return;
		}

		modepageOffset += pageLength;
	}

	if (modepageOffset != totalLength)
		goto err;

	ccb->data_resid = ccb->data_length - totalLength;
	return;

	// if we arrive here, data length was incorrect
err:
	ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR);
}


static void
scsi_test_unit_ready(ide_device_info *device, ata_request *request)
{
	TRACE("scsi_test_unit_ready\n");

	if (device->infoblock.RMSN_supported == 0
		|| device->infoblock._127_RMSN_support != 1)
		return;

	// ask device about status		
	device->tf_param_mask = 0;
	device->tf.write.command = IDE_CMD_GET_MEDIA_STATUS;

	if (ata_send_command(device, request, ATA_DRDY_REQUIRED, 15000000) != B_OK) {
		ata_request_set_status(request, SCSI_SEQUENCE_FAIL);
		return;
	}

	// bits ide_error_mcr | ide_error_mc | ide_error_wp are also valid
	// but not requested by TUR; ide_error_wp can safely be ignored, but
	// we don't want to loose media change (request) reports
	ata_finish_command(device, request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, 
		ide_error_nm | ide_error_abrt | ide_error_mcr | ide_error_mc);
		// SCSI spec is unclear here: we shouldn't report "media change (request)"
		// but what to do if there is one? anyway - we report them
}


/*! Flush internal device cache */
static bool
scsi_synchronize_cache(ide_device_info *device, ata_request *request)
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
	if (ata_send_command(device, request, ATA_DRDY_REQUIRED, 60000000) != B_OK)
		return false;

	if (ata_finish_command(device, request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, ide_error_abrt) != B_OK)
		return false;

	return true;
}


/*!	Load or eject medium
	load = true - load medium
*/
static bool
scsi_load_eject(ide_device_info *device, ata_request *request, bool load)
{
	if (load) {
		// ATA doesn't support loading
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_NOT_SUPPORTED);
		return false;
	}

	device->tf_param_mask = 0;
	device->tf.lba.command = IDE_CMD_MEDIA_EJECT;

	if (ata_send_command(device, request, ATA_DRDY_REQUIRED, 15000000) != B_OK)
		return false;

	if (ata_finish_command(device, request, ATA_WAIT_FINISH | ATA_DRDY_REQUIRED, ide_error_abrt | ide_error_nm) != B_OK)
		return false;

	return true;
}


static void
scsi_prevent_allow(ide_device_info *device, ata_request *request, bool prevent)
{
	ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_ILL_FUNCTION);
}


static void
scsi_inquiry(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_res_inquiry data;
	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)ccb->cdb;
	uint32 allocation_length = cmd->allocation_length;
	uint32 transfer_size;

	if (cmd->evpd || cmd->page_code) {
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
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
	data.cmd_queue = 0;//device->queue_depth > 1;
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

	copy_sg_data(ccb, 0, allocation_length, &data, sizeof(data), false);

	transfer_size = min(sizeof(data), allocation_length);
	transfer_size = min(transfer_size, ccb->data_length);

	ccb->data_resid = ccb->data_length - transfer_size;
}



static void
scsi_read_capacity(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_res_read_capacity data;
	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)ccb->cdb;
	uint32 lastBlock;

	if (cmd->pmi || cmd->lba) {
		ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	// TODO: 512 bytes fixed block size?
	data.block_size = B_HOST_TO_BENDIAN_INT32(512);

	lastBlock = device->total_sectors - 1;
	data.lba = B_HOST_TO_BENDIAN_INT32(lastBlock);

	copy_sg_data(ccb, 0, ccb->data_length, &data, sizeof(data), false);
	ccb->data_resid = max(ccb->data_length - sizeof(data), 0);
}


void
scsi_request_sense(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_cmd_request_sense *cmd = (scsi_cmd_request_sense *)ccb->cdb;
	scsi_sense sense;
	uint32 transferSize;

	// Copy sense data from last request into data buffer of current request.
	// The sense data of last request is still present in the current request,
	// as is isn't been cleared by ata_exec_io for SCSI_OP_REQUEST_SENSE.

	if (request->senseKey != 0)
		scsi_set_sense(&sense, request);
	else
		memset(&sense, 0, sizeof(sense));

	copy_sg_data(ccb, 0, cmd->allocation_length, &sense, sizeof(sense), false);

	transferSize = min(sizeof(sense), cmd->allocation_length);
	transferSize = min(transferSize, ccb->data_length);

	ccb->data_resid = ccb->data_length - transferSize;

	// reset sense information on read
	ata_request_clear_sense(request);
}


/*! Execute SCSI command */
void
ata_exec_io(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	
	FLOW("ata_exec_io: scsi command 0x%02x\n", ccb->cdb[0]);
		
	// ATA devices have one LUN only
	if (ccb->target_lun != 0) {
		FLOW("ata_exec_io: wrong target lun\n");
		ata_request_set_status(request, SCSI_SEL_TIMEOUT);
		ata_request_finish(request, false /* no resubmit */);
		return;
	}

	if (ccb->cdb[0] == SCSI_OP_REQUEST_SENSE) {
		// No initial clear sense, as this request is used
		// by the scsi stack to request the sense data of
		// the previous command.
		scsi_request_sense(device, request);
		ata_request_finish(request, false /* no resubmit */);
		return;
	}

	ata_request_clear_sense(request);

	switch (ccb->cdb[0]) {
		case SCSI_OP_TEST_UNIT_READY:
			scsi_test_unit_ready(device, request);
			break;

		case SCSI_OP_FORMAT: /* FORMAT UNIT */
			// we could forward ccb to disk, but modern disks cannot
			// be formatted anyway, so we just refuse ccb
			// (exceptions are removable media devices, but to my knowledge
			// they don't have to be formatted as well)
			ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_INQUIRY: 
 			scsi_inquiry(device, request);
			break;

		case SCSI_OP_MODE_SELECT_10:
			scsi_mode_select_10(device, request);
			break;

		case SCSI_OP_MODE_SENSE_10:
			scsi_mode_sense_10(device, request);
			break;

		case SCSI_OP_MODE_SELECT_6:
		case SCSI_OP_MODE_SENSE_6:
			// we've told SCSI bus manager to emulates these commands
			ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_RESERVE:
		case SCSI_OP_RELEASE:
			// though mandatory, this doesn't make much sense in a
			// single initiator environment; so what
			ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_START_STOP: {
			scsi_cmd_ssu *cmd = (scsi_cmd_ssu *)ccb->cdb;

			// with no LoEj bit set, we should only allow/deny further access
			// we ignore that (unsupported for ATA)
			// with LoEj bit set, we should additionally either load or eject the medium
			// (start = 0 - eject; start = 1 - load)

			if (!cmd->start)
				// we must always flush cache if start = 0
				scsi_synchronize_cache(device, request);

			if (cmd->load_eject)
				scsi_load_eject(device, request, cmd->start);
			break;
		}

		case SCSI_OP_PREVENT_ALLOW:
		{
			scsi_cmd_prevent_allow *cmd = (scsi_cmd_prevent_allow *)ccb->cdb;
			scsi_prevent_allow(device, request, cmd->prevent);
			break;
		}

		case SCSI_OP_READ_CAPACITY:
			scsi_read_capacity(device, request);
			break;

		case SCSI_OP_VERIFY:
			// does anyone uses this function?
			// effectly, it does a read-and-compare, which IDE doesn't support
			ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
			break;

		case SCSI_OP_SYNCHRONIZE_CACHE:
			// we ignore range and immediate bit, we always immediately flush everything 
			scsi_synchronize_cache(device, request);
			break;

		// sadly, there are two possible read/write operation codes;
		// at least, the third one, read/write(12), is not valid for DAS
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
		{
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)ccb->cdb;
			uint32 pos;
			size_t length;

			pos = ((uint32)cmd->high_lba << 16) | ((uint32)cmd->mid_lba << 8)
				| (uint32)cmd->low_lba;
			length = cmd->length != 0 ? cmd->length : 256;

			FLOW("READ6/WRITE6 pos=%lx, length=%lx\n", pos, length);

			ata_exec_read_write(device, request, pos, length, cmd->opcode == SCSI_OP_WRITE_6);
			return;
		}

		case SCSI_OP_READ_10:
		case SCSI_OP_WRITE_10:
		{
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)ccb->cdb;
			uint32 pos;
			size_t length;

			pos = B_BENDIAN_TO_HOST_INT32(cmd->lba);
			length = B_BENDIAN_TO_HOST_INT16(cmd->length);

			FLOW("READ10/WRITE10 pos=%lx, length=%lx\n", pos, length);

			if (length != 0) {
				ata_exec_read_write(device, request, pos, length, cmd->opcode == SCSI_OP_WRITE_10);
				return;
			} else {
				// we cannot transfer zero blocks (apart from LBA48)
				ata_request_set_status(request, SCSI_REQ_CMP);
			}
		}

		default:
			FLOW("command not implemented\n");
			ata_request_set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE);
	}
	ata_request_finish(request, false /* no resubmit */);
}
