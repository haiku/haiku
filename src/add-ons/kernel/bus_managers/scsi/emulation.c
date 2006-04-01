/*
 * Copyright 2004-2006, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*
	Emulation of SCSI commands that a device cannot handle.

	Some SCSI devices don't support all SCSI commands, especially
	those connected via ATAPI, USB or FireWire. These commands are
	emulated here.
*/


#include "scsi_internal.h"

#include <vm.h>

#include <string.h>


// move some function to end of file to avoid inlining
static void set_sense(scsi_ccb *request, int sense_key, int sense_asc);
static bool copy_sg_data(scsi_ccb *request, uint offset, uint allocation_length,
	void *buffer, int size, bool to_buffer);
static void get_emulation_buffer(scsi_ccb *request);
static void replace_request_data(scsi_ccb *request);
static void release_emulation_buffer(scsi_ccb *request);
static void restore_request_data(scsi_ccb *request);


/** free emulation buffer */

void
scsi_free_emulation_buffer(scsi_device_info *device)
{
	if (device->buffer_area)
		delete_area(device->buffer_area);

	device->buffer_area = 0;
	device->buffer = NULL;
	device->buffer_sg_list = NULL;
	device->buffer_size = 0;

	if (device->buffer_sem > 0)
		delete_sem(device->buffer_sem);
}


/**	setup buffer used to emulate unsupported SCSI commands
 *	buffer_size must be power of two
 */

status_t
scsi_init_emulation_buffer(scsi_device_info *device, size_t buffer_size)
{
	physical_entry map[1];
	addr_t unaligned_phys, aligned_phys, aligned_addr, unaligned_addr;
	size_t total_size;

	SHOW_FLOW0(3, "");

	device->buffer_sem = create_sem(1, "SCSI emulation buffer");
	if (device->buffer_sem < 0) {
		SHOW_ERROR(1, "cannot create DMA buffer semaphore (%s)", strerror(device->buffer_sem));
		return device->buffer_sem;
	}

	// we append S/G list to buffer as it must be locked as well
	total_size = (buffer_size + sizeof(physical_entry) + B_PAGE_SIZE - 1) & ~(B_PAGE_SIZE - 1);

	// to satisfy alignment, we must allocate a buffer twice its required size
	// and find the properly aligned part of it, ouch!
	device->buffer_area = create_area("ATAPI buffer", (void *)&unaligned_addr, B_ANY_KERNEL_ADDRESS, 
								2 * total_size, B_CONTIGUOUS, 0);
	if (device->buffer_area < 0) {
		SHOW_ERROR( 1, "cannot create DMA buffer (%s)", strerror(device->buffer_area));

		delete_sem(device->buffer_sem);
		return device->buffer_area;
	}

	get_memory_map((void *)unaligned_addr, B_PAGE_SIZE, map, 1);

	// get aligned part
	unaligned_phys = (addr_t)map[0].address;
	aligned_phys = (unaligned_phys + buffer_size - 1) & ~(buffer_size - 1);
	aligned_addr = unaligned_addr + (aligned_phys - unaligned_phys);

	SHOW_FLOW(3, "unaligned_phys = %#lx, aligned_phys = %#lx, unaligned_addr = %#lx, aligned_addr = %#lx", 
		unaligned_phys, aligned_phys, unaligned_addr, aligned_addr);

	device->buffer = (void *)aligned_addr;
	device->buffer_size = buffer_size;
	// s/g list is directly after buffer
	device->buffer_sg_list = (void *)(aligned_addr + buffer_size);
	device->buffer_sg_list[0].address = (void *)aligned_phys;
	device->buffer_sg_list[0].size = buffer_size;
	device->buffer_sg_cnt = 1;

	return B_OK;
}


/**	some ATAPI devices don't like 6 byte read/write commands, so
 *	we translate them to their 10 byte partners;
 *	USB devices usually don't like 10 bytes either
 */

static bool
scsi_read_write_6(scsi_ccb *request)
{
	scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->orig_cdb;
	scsi_cmd_rw_10 *cdb = (scsi_cmd_rw_10 *)request->cdb;

	SHOW_FLOW0(3, "patching READ/WRITE(6) to READ/WRITE(10)");

	request->cdb_len = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = cmd->opcode + (SCSI_OP_READ_10 - SCSI_OP_READ_6);
	cdb->LUN = cmd->LUN;
	cdb->low_LBA = cmd->low_LBA;
	cdb->mid_LBA = cmd->mid_LBA;
	cdb->high_LBA = cmd->high_LBA;
	cdb->low_length = cmd->length;
	cdb->high_length = cmd->length == 0;
	cdb->control = cmd->control;

	return true;
}


/**	all ATAPI devices don't like 6 byte MODE SENSE, so we translate
 *	that to 10 byte MODE SENSE
 */

static bool
scsi_start_mode_sense_6(scsi_ccb *request)
{
	scsi_cmd_mode_sense_6 *cmd = (scsi_cmd_mode_sense_6 *)request->orig_cdb;
	scsi_cmd_mode_sense_10 *cdb = (scsi_cmd_mode_sense_10 *)request->cdb;
	size_t allocation_length;

	SHOW_FLOW0(3, "patching MODE SENSE(6) to MODE SENSE(10)");

	request->cdb_len = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = SCSI_OP_MODE_SENSE_10;
	cdb->DBD = cmd->DBD;
	cdb->LUN = cmd->LUN;
	cdb->page_code = cmd->page_code;
	cdb->PC = cmd->PC;

	allocation_length = cmd->allocation_length
		- sizeof(scsi_cmd_mode_sense_6) + sizeof(scsi_cmd_mode_sense_10);
	cdb->high_allocation_length = allocation_length >> 8;
	cdb->low_allocation_length = allocation_length & 0xff;

	SHOW_FLOW(3, "allocation_length=%ld", allocation_length);

	cdb->control = cmd->control;

	// data header of 10 byte version is longer, so use internal buffer
	// and copy it back once the command is finished
	get_emulation_buffer(request);
	replace_request_data(request);

	// restrict data buffer len to length specified in cdb
	request->data_len = allocation_length;
	return true;
}


/**	all ATAPI devices don't like 6 byte MODE SELECT, so we translate
 *	that to 10 byte MODE SELECT
 */

static bool
scsi_start_mode_select_6(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	scsi_cmd_mode_select_6 *cmd = (scsi_cmd_mode_select_6 *)request->orig_cdb;
	scsi_cmd_mode_select_10 *cdb = (scsi_cmd_mode_select_10 *)request->cdb;
	scsi_mode_param_header_6 header_6;
	scsi_mode_param_header_10 *header_10 = (scsi_mode_param_header_10 *)device->buffer;
	size_t param_list_length_6, param_list_length_10;

	SHOW_FLOW0(3, "patching MODE SELECT(6) to MODE SELECT(10)");

	// calculate new data buffer size
	param_list_length_6 = cmd->param_list_length;
	param_list_length_10 = param_list_length_6
		- sizeof(scsi_mode_param_header_6) + sizeof(scsi_mode_param_header_10);

	// we need to replace data header, thus use internal buffer		
	get_emulation_buffer(request);

	// make sure our buffer is large enough
	if (param_list_length_10 > device->buffer_size)
		goto err;

	// construct new cdb		
	request->cdb_len = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = SCSI_OP_MODE_SELECT_10;
	cdb->SP = cmd->SP;
	cdb->PF = cmd->PF;
	cdb->LUN = cmd->LUN;

	cdb->high_param_list_length = param_list_length_10 >> 8;
	cdb->low_param_list_length = param_list_length_10 & 0xff;

	SHOW_FLOW(3, "param_list_length=%ld", param_list_length_6);

	cdb->control = cmd->control;

	// copy and adapt header			
	if (!copy_sg_data(request, 0, param_list_length_6, &header_6, sizeof(header_6), true))
		goto err;

	memset(header_10, 0, sizeof(*header_10));

	// mode_data_len is reserved for MODE SELECT
	header_10->medium_type = header_6.medium_type;
	header_10->dev_spec_parameter = header_6.dev_spec_parameter;
	header_10->low_block_desc_len = header_6.block_desc_len;

	// append actual mode select data		
	if (!copy_sg_data( request, sizeof(header_6), param_list_length_6, header_10 + 1,
			param_list_length_10 - sizeof(*header_10), true))
		goto err;

	replace_request_data(request);

	// restrict buffer size to the one specified in cdb
	request->data_len = param_list_length_10;
	return true;

err:
	release_emulation_buffer(request);

	set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_PARAM_LIST_FIELD);
	return false;
}


/**	emulation procedure at start of command
 *	returns false if command is to be finished without further execution
 */

bool
scsi_start_emulation(scsi_ccb *request)
{
	//snooze( 1000000 );

	SHOW_FLOW(3, "command=%x", request->cdb[0]);

	memcpy(request->orig_cdb, request->cdb, SCSI_MAX_CDB_SIZE);
	request->orig_cdb_len = request->cdb_len;

	switch (request->orig_cdb[0]) {
		case SCSI_OP_READ_6:
		case SCSI_OP_WRITE_6:
			return scsi_read_write_6(request);

		case SCSI_OP_MODE_SENSE_6:
			return scsi_start_mode_sense_6(request);

		case SCSI_OP_MODE_SELECT_6:
			return scsi_start_mode_select_6(request);
	}

	return true;
}


/** back-translate MODE SENSE 10 to MODE SENSE 6 */

static void
scsi_finish_mode_sense_10_6(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	scsi_mode_param_header_6 header_6;
	scsi_mode_param_header_10 *header_10 = (scsi_mode_param_header_10 *)device->buffer;
	int transfer_size_6, transfer_size_10;

	if (request->subsys_status != SCSI_REQ_CMP || request->device_status != SCSI_STATUS_GOOD) {
		// on error, do nothing
		release_emulation_buffer(request);
		return;
	}

	// check how much data we got from device and thus will copy into
	// request data
	transfer_size_10 = request->data_len - request->data_resid;
	transfer_size_6 = transfer_size_10 
		- sizeof(scsi_mode_param_header_10 *) + sizeof(scsi_mode_param_header_6 *);

	SHOW_FLOW(0, "fixing MODE SENSE(6) (%d bytes)", transfer_size_6);

	restore_request_data(request);

	// adapt header

	// convert total length
	// (+1 is there because mode_data_len in 10 byte header ignores first
	// two bytes, whereas in the 6 byte header it ignores only one byte)
	header_6.mode_data_len = header_10->low_mode_data_len
		- sizeof(scsi_mode_param_header_10) + sizeof(scsi_mode_param_header_6)
		+ 1;

	header_6.medium_type = header_10->medium_type;
	header_6.dev_spec_parameter = header_10->dev_spec_parameter;
	header_6.block_desc_len = header_10->low_block_desc_len;

	// copy adapted header			
	copy_sg_data(request, 0, transfer_size_6, &header_6, sizeof(header_6), false);

	// copy remaining data
	copy_sg_data(request, sizeof(header_6), transfer_size_6, 
		header_10 + 1, transfer_size_10 - sizeof(*header_10), false);

	request->data_resid = request->data_len - transfer_size_6;

	release_emulation_buffer(request);
}


/** back-translate MODE SELECT 10 to MODE SELECT 6 */

static void
scsi_finish_mode_select_10_6(scsi_ccb *request)
{
	SHOW_FLOW0(3, "fixing MODE SELECT(6)");

	// adjust transmission length as we've used the longer
	// mode select 10 data header
	request->data_resid += sizeof(scsi_mode_param_header_6)
		- sizeof(scsi_mode_param_header_10);

	restore_request_data(request);
	release_emulation_buffer(request);
}


/** fix inquiry data; some ATAPI devices return wrong version */

static void
scsi_finish_inquiry(scsi_ccb *request)
{
	int transfer_size;
	scsi_res_inquiry res;

	SHOW_FLOW0(3, "fixing INQUIRY");

	if (request->subsys_status != SCSI_REQ_CMP || request->device_status != SCSI_STATUS_GOOD)
		return;

	transfer_size = request->data_len - request->data_resid;

	copy_sg_data(request, 0, transfer_size, &res, sizeof(res), true);

	SHOW_FLOW(3, "ANSI version: %d, response data format: %d",
		res.ANSI_version, res.response_data_format);

	res.ANSI_version = 2;
	res.response_data_format = 2;

	copy_sg_data(request, 0, transfer_size, &res, sizeof(res), false);
}


/** adjust result of emulated request */

void
scsi_finish_emulation(scsi_ccb *request)
{
	SHOW_FLOW0(3, "");

	switch ((((int)request->cdb[0]) << 8) | request->orig_cdb[0]) {
		case (SCSI_OP_MODE_SENSE_10 << 8) | SCSI_OP_MODE_SENSE_6:
			scsi_finish_mode_sense_10_6(request);
			break;

		case (SCSI_OP_MODE_SELECT_10 << 8) | SCSI_OP_MODE_SELECT_6:
			scsi_finish_mode_select_10_6(request);
			break;

		case (SCSI_OP_INQUIRY << 8) | SCSI_OP_INQUIRY:
			scsi_finish_inquiry(request);
			break;
	}

	// restore cdb
	memcpy(request->cdb, request->orig_cdb, SCSI_MAX_CDB_SIZE);
	request->cdb_len = request->orig_cdb_len;
}


/** set sense of request */

static void
set_sense(scsi_ccb *request, int sense_key, int sense_asc)
{
	scsi_sense *sense = (scsi_sense *)request->sense;

	SHOW_FLOW( 3, "sense_key=%d, sense_asc=%d", sense_key, sense_asc );

	request->subsys_status = SCSI_REQ_CMP;
	request->device_status = SCSI_STATUS_CHECK_CONDITION;

	// TBD: we can only handle requests with autosense
	// 		without autosense, we had to manage virtual sense data,
	//		which is probably not worth the hazzle
	if ((request->flags & SCSI_DIS_AUTOSENSE) != 0)
		return;

	memset(sense, 0, sizeof(*sense));

	sense->error_code = SCSIS_CURR_ERROR;
	sense->sense_key = sense_key;
	sense->add_sense_length = sizeof(*sense) - 7;
	sense->asc = (sense_asc >> 8) & 0xff;
	sense->ascq = sense_asc;
	sense->sense_key_spec.raw.SKSV = 0;	// no additional info

	request->subsys_status |= SCSI_AUTOSNS_VALID;
}


/**	copy data between request data and buffer
 *	request			- request to copy data from/to
 *	offset			- offset of data in request
 *	allocation_length- limit of request's data buffer according to CDB
 *	buffer			- data to copy data from/to
 *	size			- number of bytes to copy
 *	to_buffer		- true: copy from request to buffer
 *					  false: copy from buffer to request
 *	return: true, if data of request was large enough
 */

static bool
copy_sg_data(scsi_ccb *request, uint offset, uint allocation_length,
	void *buffer, int size, bool to_buffer)
{
	const physical_entry *sg_list = request->sg_list;
	int sg_cnt = request->sg_cnt;
	int req_size;

	SHOW_FLOW(3, "offset=%u, req_size_limit=%d, size=%d, sg_list=%p, sg_cnt=%d, %s buffer", 
		offset, allocation_length, size, sg_list, sg_cnt, to_buffer ? "to" : "from");

	// skip unused S/G entries
	while (sg_cnt > 0 && offset >= sg_list->size) {
		offset -= sg_list->size;
		++sg_list;
		--sg_cnt;
	}

	if (sg_cnt == 0)
		return 0;

	// remaining bytes we are allowed to copy from/to request 		
	req_size = min(allocation_length, request->data_len) - offset;

	// copy one S/G entry at a time
	for (; size > 0 && req_size > 0 && sg_cnt > 0; ++sg_list, --sg_cnt) {
		size_t bytes;
		addr_t virtualAddress;

		bytes = min(size, req_size);
		bytes = min(bytes, sg_list->size);

		if (vm_get_physical_page((addr_t)sg_list->address, (void *)&virtualAddress,
				PHYSICAL_PAGE_CAN_WAIT) != B_OK) 
			return false;

		SHOW_FLOW(0, "buffer = %p, virt_addr = %#lx, bytes = %lu, to_buffer = %d",
			buffer, virtualAddress + offset, bytes, to_buffer);

		if (to_buffer)
			memcpy(buffer, (void *)(virtualAddress + offset), bytes);
		else
			memcpy((void *)(virtualAddress + offset), buffer, bytes);

#if 0
		{
			int i;
			
			for (i = 0; i < bytes; ++i) {
				char byte;

				if (to_buffer)
					byte = ((char *)virtualAddress)[offset + i];
				else
					byte = ((char *)buffer)[i];

				FAST_LOG1( request->device->log, ev_copy_sg_data, byte );
			}
		}
#endif

		vm_put_physical_page(virtualAddress);

		buffer = (char *)buffer + bytes;
		size -= bytes;
		offset = 0;
	}
	
	return size == 0;
}


/** allocate emulation buffer */

static void
get_emulation_buffer(scsi_ccb *request)
{
	scsi_device_info *device = request->device;

	SHOW_FLOW0( 3, "" );

	acquire_sem(device->buffer_sem);

	request->orig_sg_list = request->sg_list;
	request->orig_sg_cnt = request->sg_cnt;
	request->orig_data_len = request->data_len;

	request->sg_list = device->buffer_sg_list;
	request->sg_cnt = device->buffer_sg_cnt;
	request->data_len = device->buffer_size;	
}


/**	replace request data with emulation buffer, saving original pointer;
 *	you must have called get_emulation_buffer() first
 */

static void
replace_request_data(scsi_ccb *request)
{
	scsi_device_info *device = request->device;

	SHOW_FLOW0( 3, "" );

	request->orig_sg_list = request->sg_list;
	request->orig_sg_cnt = request->sg_cnt;
	request->orig_data_len = request->data_len;

	request->sg_list = device->buffer_sg_list;
	request->sg_cnt = device->buffer_sg_cnt;
	request->data_len = device->buffer_size;	
}


/** release emulation buffer */

static void
release_emulation_buffer(scsi_ccb *request)
{
	SHOW_FLOW0( 3, "" );

	release_sem(request->device->buffer_sem);
}


/** restore original request data pointers */

static void
restore_request_data(scsi_ccb *request)
{
	SHOW_FLOW0( 3, "" );

	request->sg_list = request->orig_sg_list;
	request->sg_cnt = request->orig_sg_cnt;
	request->data_len = request->orig_data_len;
}
