/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
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
	scsi_modepage_contr contr;
	scsi_mode_param_block_desc block_desc;
	size_t total_length = 
		sizeof(scsi_mode_param_header_10) +
		sizeof(scsi_mode_param_block_desc) +
		sizeof(scsi_modepage_contr);
	scsi_mode_param_dev_spec_da devspec = {
		res0_0 : 0,
		DPOFUA : 0,
		res0_6 : 0,
		WP : 0
	};
	int allocation_length;
	
	SHOW_ERROR0( 0, "Hi!" );
	
	allocation_length = ((int16)cmd->high_allocation_length << 8) 
		| cmd->low_allocation_length;
	
	// we answer control page requests and "all pages" requests
	// (as the latter are the same as the first)
	if( (cmd->page_code != SCSI_MODEPAGE_CONTROL &&
		 cmd->page_code != SCSI_MODEPAGE_ALL) ||
		(cmd->PC != SCSI_MODE_SENSE_PC_CURRENT &&
		 cmd->PC != SCSI_MODE_SENSE_PC_SAVED)) 
	{
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD );
		return;
	}

	//param_header = (scsi_mode_param_header_10 *)request->data;
	param_header.low_mode_data_len = total_length - 1;
	param_header.high_mode_data_len = 0;
	param_header.medium_type = 0; 		// XXX standard is a bit vague here
	param_header.dev_spec_parameter = *(uint8 *)&devspec;
	param_header.low_block_desc_len = sizeof( scsi_mode_param_block_desc );
	param_header.high_block_desc_len = 0;
	
	copy_sg_data( request, 0, allocation_length, 
		&param_header, sizeof( param_header ), false );
	
	/*block_desc = (scsi_mode_param_block_desc *)(request->data 
		+ sizeof( *param_header ));*/
	memset( &block_desc, 0, sizeof( block_desc ));
	// density is reserved (0), descriptor apply to entire medium (num_blocks=0)
	// remains the blocklen to be set
	block_desc.high_blocklen = 0;
	block_desc.med_blocklen = 512 >> 8;
	block_desc.low_blocklen = 512 & 0xff;
	
	copy_sg_data( request, sizeof( param_header ), allocation_length, 
		&block_desc, sizeof( block_desc ), false );

	/*contr = (scsi_modepage_contr *)(request->data 
		+ sizeof( *param_header )
		+ ((uint16)param_header->high_block_desc_len << 8) 
		+ param_header->low_block_desc_len);*/
	
	memset( &contr, 0, sizeof( contr ));
	contr.RLEC = false;
	contr.DQue = !device->CQ_enabled;
	contr.QErr = false;					// when a command fails we requeue all 
										// lost commands automagically
	contr.QAM = SCSI_QAM_UNRESTRICTED;
	
	copy_sg_data( request, 
		sizeof( param_header ) 
		+ ((uint16)param_header.high_block_desc_len << 8) 
		+ param_header.low_block_desc_len, 
		allocation_length,
		&contr, sizeof( contr ), false );

	// the number of bytes that were transferred to buffer is
	// restricted by allocation length and by request data buffer size
	total_length = min( total_length, allocation_length );
	total_length = min( total_length, request->data_len );
	
	request->data_resid = request->data_len - total_length;
	return;
}


// emulate modifying control page 
static bool ata_mode_select_contrpage( ide_device_info *device, ide_qrequest *qrequest,
	scsi_modepage_contr *page )
{
	if( page->header.page_length != sizeof( *page ) - sizeof( page->header )) {
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR );
		return false;
	}

	// we only support enabling/disabling command queuing
	enable_CQ( device, !page->DQue );
	return true;
}


// emulate MODE SELECT 10 command
static void ata_mode_select_10( ide_device_info *device, ide_qrequest *qrequest )
{
	scsi_ccb *request = qrequest->request;
	scsi_cmd_mode_select_10 *cmd = (scsi_cmd_mode_select_10 *)request->cdb;
	scsi_mode_param_header_10 param_header;
	scsi_modepage_header page_header;
	int total_length;
	uint modepage_offset;
	char modepage_buffer[64];	// !!! enlarge this to support longer mode pages
	
	if( cmd->SP || cmd->PF != 1 ) {
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD );
		return;
	}
	
	total_length = min( request->data_len, 
		((uint16)cmd->high_param_list_length << 8) | cmd->low_param_list_length);
	
	// first, retrieve page header to get size of different chunks
	//param_header = (scsi_mode_param_header_10 *)request->data;
	if( !copy_sg_data( request, 0, total_length, 
		&param_header, sizeof( param_header ), true ))
		goto err;
	
	total_length = min( total_length, 
		(((uint16)param_header.high_mode_data_len << 8) | param_header.low_mode_data_len) + 1 );
	
	// this is the start of the first mode page;
	// we ignore the block descriptor silently
	modepage_offset = 	
		sizeof( param_header ) + 
		(((uint16)param_header.high_block_desc_len << 8) | param_header.low_block_desc_len);
		
	// go through list of pages		
	while( modepage_offset < total_length ) {
		int page_len;
		
		// get header to know how long page is
		if( !copy_sg_data( request, modepage_offset, total_length, 
			&page_header, sizeof( page_header ), true ))
			goto err;

		// get size of one page and copy it to buffer
		page_len = page_header.page_length + sizeof( scsi_modepage_header );
		
		// the buffer has a maximum size - this is really standard compliant but
		// sufficient for our needs
		if( page_len > sizeof( modepage_buffer ))
			goto err;
			
		if( !copy_sg_data( request, modepage_offset, total_length, 
			&modepage_buffer, min( page_len, sizeof( modepage_buffer )), true ))
			goto err;

		// modify page;
		// currently, we only support the control mode page
		switch( page_header.page_code ) {
		case SCSI_MODEPAGE_CONTROL:
			if( !ata_mode_select_contrpage( device, qrequest,
				(scsi_modepage_contr *)modepage_buffer ))
				return;
			break;
			
		default:
			set_sense( device, 
				SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_PARAM_LIST_FIELD );
			return;
		}
		
		modepage_offset += page_len;
	}
	
	if( modepage_offset != total_length )
		goto err;
		
	request->data_resid = request->data_len - total_length;
	return;

	// if we arrive here, data length was incorrect
err:
	set_sense( device, 
		SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_LIST_LENGTH_ERR );
}


// emulate TEST UNIT READY
static bool ata_tur( ide_device_info *device, ide_qrequest *qrequest )
{
	SHOW_FLOW0( 3, "" );
	
	if( !device->infoblock.RMSN_supported ||
		device->infoblock._127_RMSN_support != 1 )
		return true;

	// ask device about status		
	device->tf_param_mask = 0;
	device->tf.write.command = IDE_CMD_GET_MEDIA_STATUS;
	
	if( !send_command( device, qrequest, true, 15, ide_state_sync_waiting )) 
		return false;

	// bits ide_error_mcr | ide_error_mc | ide_error_wp are also valid
	// but not requested by TUR; ide_error_wp can safely be ignored, but
	// we don't want to loose media change (request) reports
	if( !check_output( device, true, 
		ide_error_nm | ide_error_abrt | ide_error_mcr | ide_error_mc, 
		false ))
	{
		// SCSI spec is unclear here: we shouldn't report "media change (request)"
		// but what to do if there is one? anyway - we report them
		;
	}
	
	return true;
}


// flush internal device cache
static bool ata_flush_cache( ide_device_info *device, ide_qrequest *qrequest )
{
	// we should also ask for FLUSH CACHE support, but everyone denies it
	// (looks like they cheat to gain some performance advantage, but
	//  that's pretty useless: everyone does it...)
	if( !device->infoblock.write_cache_supported )
		return true;
	
	device->tf_param_mask = 0;
	device->tf.lba.command = device->use_48bits ? IDE_CMD_FLUSH_CACHE_EXT 
		: IDE_CMD_FLUSH_CACHE;
	
	// spec says that this may take more then 30s, how much more?
	if( !send_command( device, qrequest, true, 60, ide_state_sync_waiting ))
		return false;
	
	wait_for_sync( device->bus );

	return check_output( device, true, ide_error_abrt, false );
}


// load or eject medium
// load = true - load medium
static bool ata_load_eject( ide_device_info *device, ide_qrequest *qrequest, bool load )
{
	if( load ) {
		// ATA doesn't support loading
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_PARAM_NOT_SUPPORTED );
		return false;
	}
	
	device->tf_param_mask = 0;
	device->tf.lba.command = IDE_CMD_MEDIA_EJECT;
	
	if( !send_command( device, qrequest, true, 15, ide_state_sync_waiting ))
		return false;
		
	wait_for_sync( device->bus );
	
	return check_output( device, true, ide_error_abrt | ide_error_nm, false );
}


// emulate PREVENT ALLOW command
static bool ata_prevent_allow( ide_device_info *device, bool prevent )
{
	set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_ILL_FUNCTION );
	return false;
}


// emulate INQUIRY command
static void ata_inquiry( ide_device_info *device, ide_qrequest *qrequest )
{
	scsi_ccb *request = qrequest->request;
	scsi_res_inquiry data;
	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)request->cdb;
	uint allocation_length = cmd->allocation_length;
	int transfer_size;
	
	if (cmd->EVPD || cmd->page_code) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	memset( &data, 0, sizeof( data ));
	
	data.device_type = scsi_dev_direct_access;
	data.device_qualifier = scsi_periph_qual_connected;
	
	data.device_type_modifier = 0;
	data.RMB = false;
	
	data.ANSI_version = 2;
	data.ECMA_version = 0;
	data.ISO_version = 0;
	
	data.response_data_format = 2;
	data.TrmIOP = false;		// to be changed if we support TERM I/O

	data.additional_length = sizeof( scsi_res_inquiry ) - 4;
	
	data.SftRe = false;
	data.CmdQue = device->queue_depth > 1;
	data.Linked = false;
	
	// these values are free-style
	data.Sync = false;
	data.WBus16 = true;
	data.WBus32 = false;
	
	data.RelAdr = false;
	
	// the following fields are *much* to small, sigh...
	memcpy( data.vendor_ident, device->infoblock.model_number, 
		sizeof( data.vendor_ident ));
	memcpy( data.product_ident, device->infoblock.model_number + 8, 
		sizeof( data.product_ident ));
	memcpy( data.product_rev, "    ", sizeof( data.product_rev ));
	
	copy_sg_data( request, 0, allocation_length, &data, sizeof( data ), false );
	
	transfer_size = min( sizeof( data ), allocation_length );
	transfer_size = min( transfer_size, request->data_len );
	
	request->data_resid = request->data_len - transfer_size;
}


/** emulate READ CAPACITY command */

static void
read_capacity(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_res_read_capacity data;
	scsi_cmd_read_capacity *cmd = (scsi_cmd_read_capacity *)request->cdb;
	uint32 last_block;

	if (cmd->PMI || cmd->top_LBA || cmd->high_LBA || cmd->mid_LBA || cmd->low_LBA) {
		set_sense(device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_CDB_FIELD);
		return;
	}

	// ToDo: 512 bytes fixed block size?
	data.top_block_size = (512 >> 24) & 0xff;
	data.high_block_size = (512 >> 16) & 0xff;
	data.mid_block_size = (512 >> 8) & 0xff;
	data.low_block_size = 512 & 0xff;

	last_block = device->total_sectors - 1;

	data.top_LBA = (last_block >> 24) & 0xff;
	data.high_LBA = (last_block >> 16) & 0xff;
	data.mid_LBA = (last_block >> 8) & 0xff;
	data.low_LBA = last_block & 0xff;

	copy_sg_data(request, 0, request->data_len, &data, sizeof(data), false);

	request->data_resid = max(request->data_len - sizeof(data), 0);
}


/** execute SCSI command */

void
ata_exec_io(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	
	SHOW_FLOW( 3, "command=%x", request->cdb[0] );
		
	// ATA devices have one LUN only
	if( request->target_lun != 0 ) {
		request->subsys_status = SCSI_SEL_TIMEOUT;
		finish_request( qrequest, false );
		return;
	}
	
	// starting a request means deleting sense, so don't do it if
	// the command wants to read it
	if( request->cdb[0] != SCSI_OP_REQUEST_SENSE )	
		start_request( device, qrequest );
	
	switch( request->cdb[0] ) {
	case SCSI_OP_TUR:
		ata_tur( device, qrequest );
		break;

	case SCSI_OP_REQUEST_SENSE:
		ide_request_sense( device, qrequest );
		return;
		
	case SCSI_OP_FORMAT: /* FORMAT UNIT */
		// we could forward request to disk, but modern disks cannot
		// be formatted anyway, so we just refuse request
		// (exceptions are removable media devices, but to my knowledge
		// they don't have to be formatted as well)
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE );
		break;

	case SCSI_OP_INQUIRY: 
		ata_inquiry( device, qrequest );
		break;

	case SCSI_OP_MODE_SELECT_10:
		ata_mode_select_10( device, qrequest );
		break;
			
	case SCSI_OP_MODE_SENSE_10:
		ata_mode_sense_10( device, qrequest );
		break;
	
	case SCSI_OP_MODE_SELECT_6:
	case SCSI_OP_MODE_SENSE_6:
		// we've told SCSI bus manager to emulates these commands
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE );
		break;
		
	case SCSI_OP_RESERVE:
	case SCSI_OP_RELEASE:
		// though mandatory, this doesn't make much sense in a
		// single initiator environment; so what
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE );
		break;
		
	case SCSI_OP_START_STOP: {
		scsi_cmd_ssu *cmd = (scsi_cmd_ssu *)request->cdb;
		
		// with no LoEj bit set, we should only allow/deny further access
		// we ignore that (unsupported for ATA)
		// with LoEj bit set, we should additionally either load or eject the medium
		// (start = 0 - eject; start = 1 - load)
		
		if( !cmd->start )
			// we must always flush cache if start = 0
			ata_flush_cache( device, qrequest );
		
		if( cmd->LoEj )
			ata_load_eject( device, qrequest, cmd->start );
			
		break; }
		
	case SCSI_OP_PREVENT_ALLOW: {
		scsi_cmd_prevent_allow *cmd = (scsi_cmd_prevent_allow *)request->cdb;
		
		ata_prevent_allow( device, cmd->prevent );
		break; }
	
	case SCSI_OP_READ_CAPACITY:
		read_capacity( device, qrequest );
		break;		
	
	case SCSI_OP_VERIFY:
		// does anyone uses this function?
		// effectly, it does a read-and-compare, which IDE doesn't support
		set_sense( device, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE );
		break;
	
	case SCSI_OP_SYNCHRONIZE_CACHE:
		// we ignore range and immediate bit, we always immediately flush everything 
		ata_flush_cache( device, qrequest );
		break;
	
	// sadly, there are two possible read/write operation codes;
	// at least, the third one, read/write(12), is not valid for DAS
	case SCSI_OP_READ_6:
	case SCSI_OP_WRITE_6: {
		scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;
		uint32 pos;
		size_t length;
		
		pos = ((uint32)cmd->high_LBA << 16) | ((uint32)cmd->mid_LBA << 8)
			| (uint32)cmd->low_LBA;
			
		length = cmd->length != 0 ? cmd->length : 256;
		
		SHOW_FLOW( 3, "READ6/WRITE6 pos=%ux, length=%ux", (uint)pos, (uint)length );
		
		ata_send_rw( device, qrequest, pos, length, cmd->opcode == SCSI_OP_WRITE_6 );		
		return; }
		
	case SCSI_OP_READ_10:
	case SCSI_OP_WRITE_10: {
		scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;
		uint32 pos;
		size_t length;
		
		pos = ((uint32)cmd->top_LBA << 24) | ((uint32)cmd->high_LBA << 16) 
			| ((uint32)cmd->mid_LBA << 8) | (uint32)cmd->low_LBA;
			
		length = ((uint32)cmd->high_length << 8) | cmd->low_length;
		
		if( length != 0 ) {
			ata_send_rw( device, qrequest, pos, length, cmd->opcode == SCSI_OP_WRITE_10 );
		} else {
			// we cannot transfer zero blocks (apart from LBA48)
			finish_request( qrequest, false );
		}
		return;	}
		
	default:
		set_sense( device, 
			SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_OPCODE );
	}
	
	finish_checksense( qrequest );
}
