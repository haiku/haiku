/*
 * Copyright 2004-2008, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*!	Emulation of SCSI commands that a device cannot handle.

	Some SCSI devices don't support all SCSI commands, especially
	those connected via ATAPI, USB or FireWire. These commands are
	emulated here.
*/


#include "scsi_internal.h"

#include <vm/vm.h>

#include <string.h>


// move some function to end of file to avoid inlining
static void set_sense(scsi_ccb *request, int sense_key, int sense_asc);
static bool copy_sg_data(scsi_ccb *request, uint offset, uint allocation_length,
	void *buffer, int size, bool to_buffer);
static void get_emulation_buffer(scsi_ccb *request);
static void replace_request_data(scsi_ccb *request);
static void release_emulation_buffer(scsi_ccb *request);
static void restore_request_data(scsi_ccb *request);


/*! Free emulation buffer */
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


/*!	Setup buffer used to emulate unsupported SCSI commands
	buffer_size must be power of two
*/
status_t
scsi_init_emulation_buffer(scsi_device_info *device, size_t buffer_size)
{
	physical_entry map[1];
	size_t total_size;

	SHOW_FLOW0(3, "");

	device->buffer_sem = create_sem(1, "SCSI emulation buffer");
	if (device->buffer_sem < 0) {
		SHOW_ERROR(1, "cannot create DMA buffer semaphore (%s)", strerror(device->buffer_sem));
		return device->buffer_sem;
	}

	// we append S/G list to buffer as it must be locked as well
	total_size = (buffer_size + sizeof(physical_entry) + B_PAGE_SIZE - 1)
		& ~(B_PAGE_SIZE - 1);

	void* address;
	virtual_address_restrictions virtualRestrictions = {};
	virtualRestrictions.address_specification = B_ANY_KERNEL_ADDRESS;
	physical_address_restrictions physicalRestrictions = {};
	physicalRestrictions.alignment = buffer_size;
	device->buffer_area = create_area_etc(B_SYSTEM_TEAM, "ATAPI buffer",
		total_size, B_32_BIT_CONTIGUOUS, 0, 0, 0, &virtualRestrictions,
		&physicalRestrictions, &address);
		// TODO: Use B_CONTIGUOUS, if possible!

	if (device->buffer_area < 0) {
		SHOW_ERROR( 1, "cannot create DMA buffer (%s)", strerror(device->buffer_area));

		delete_sem(device->buffer_sem);
		return device->buffer_area;
	}

	get_memory_map(address, B_PAGE_SIZE, map, 1);

	// get aligned part
	phys_addr_t physicalAddress = map[0].address;

	SHOW_FLOW(3, "physical = %#" B_PRIxPHYSADDR ", address = %p",
		physicalAddress, address);

	device->buffer = (char*)address;
	device->buffer_size = buffer_size;
	// s/g list is directly after buffer
	device->buffer_sg_list = (physical_entry*)((char*)address + buffer_size);
	device->buffer_sg_list[0].address = physicalAddress;
	device->buffer_sg_list[0].size = buffer_size;
	device->buffer_sg_count = 1;

	return B_OK;
}


/*!	Some ATAPI devices don't like 6 byte read/write commands, so
	we translate them to their 10 byte counterparts;
	USB devices usually don't like 10 bytes either
*/
static bool
scsi_read_write_6(scsi_ccb *request)
{
	scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->orig_cdb;
	scsi_cmd_rw_10 *cdb = (scsi_cmd_rw_10 *)request->cdb;

	SHOW_FLOW0(3, "patching READ/WRITE(6) to READ/WRITE(10)");

	request->cdb_length = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = cmd->opcode + (SCSI_OP_READ_10 - SCSI_OP_READ_6);
	cdb->lun = cmd->lun;
	cdb->lba = B_HOST_TO_BENDIAN_INT32((uint32)cmd->low_lba
		| ((uint32)cmd->mid_lba << 8) | ((uint32)cmd->high_lba << 16));
	if (cmd->length == 0)
		cdb->length = B_HOST_TO_BENDIAN_INT16(256);
	else
		cdb->length = B_HOST_TO_BENDIAN_INT16((uint16)cmd->length);
	cdb->control = cmd->control;

#if 0
	{
		static uint32 lastLBA = 0;
		static uint16 lastLength = 0;
		static uint32 contigCount = 0;
		static uint64 totalContig = 0;

		uint32 currentLBA = B_BENDIAN_TO_HOST_INT32(cdb->lba);
		uint16 currentLength = B_BENDIAN_TO_HOST_INT16(cdb->length);

		if (lastLBA + lastLength == currentLBA) {
			contigCount++;
			totalContig++;
		} else
			contigCount = 0;

		lastLBA = currentLBA;
		lastLength = currentLength;

		dprintf("scsi_read_write_6: %lld lba %ld; length: %d\n", totalContig,
			B_BENDIAN_TO_HOST_INT32(cdb->lba),
			B_BENDIAN_TO_HOST_INT16(cdb->length));
	}
#endif

	return true;
}


/*!	All ATAPI devices don't like 6 byte MODE SENSE, so we translate
	that to 10 byte MODE SENSE
*/
static bool
scsi_start_mode_sense_6(scsi_ccb *request)
{
	scsi_cmd_mode_sense_6 *cmd = (scsi_cmd_mode_sense_6 *)request->orig_cdb;
	scsi_cmd_mode_sense_10 *cdb = (scsi_cmd_mode_sense_10 *)request->cdb;
	size_t allocationLength;

	SHOW_FLOW0(3, "patching MODE SENSE(6) to MODE SENSE(10)");

	request->cdb_length = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = SCSI_OP_MODE_SENSE_10;
	cdb->disable_block_desc = cmd->disable_block_desc;
	cdb->lun = cmd->lun;
	cdb->page_code = cmd->page_code;
	cdb->page_control = cmd->page_control;

	allocationLength = cmd->allocation_length
		- sizeof(scsi_cmd_mode_sense_6) + sizeof(scsi_cmd_mode_sense_10);
	cdb->allocation_length = B_HOST_TO_BENDIAN_INT16(allocationLength);

	SHOW_FLOW(3, "allocation_length=%ld", allocationLength);

	cdb->control = cmd->control;

	// data header of 10 byte version is longer, so use internal buffer
	// and copy it back once the command is finished
	get_emulation_buffer(request);
	replace_request_data(request);

	// restrict data buffer len to length specified in cdb
	request->data_length = allocationLength;
	return true;
}


/*!	All ATAPI devices don't like 6 byte MODE SELECT, so we translate
	that to 10 byte MODE SELECT
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
	request->cdb_length = sizeof(*cdb);
	memset(cdb, 0, sizeof(*cdb));

	cdb->opcode = SCSI_OP_MODE_SELECT_10;
	cdb->save_pages = cmd->save_pages;
	cdb->pf = cmd->pf;
	cdb->lun = cmd->lun;
	cdb->param_list_length = B_HOST_TO_BENDIAN_INT16(param_list_length_10);

	SHOW_FLOW(3, "param_list_length=%ld", param_list_length_6);

	cdb->control = cmd->control;

	// copy and adapt header
	if (!copy_sg_data(request, 0, param_list_length_6, &header_6, sizeof(header_6), true))
		goto err;

	memset(header_10, 0, sizeof(*header_10));

	// mode_data_len is reserved for MODE SELECT
	header_10->medium_type = header_6.medium_type;
	header_10->dev_spec_parameter = header_6.dev_spec_parameter;
	header_10->block_desc_length = B_HOST_TO_BENDIAN_INT16(
		(uint16)header_6.block_desc_length);

	// append actual mode select data
	if (!copy_sg_data(request, sizeof(header_6), param_list_length_6, header_10 + 1,
			param_list_length_10 - sizeof(*header_10), true))
		goto err;

	replace_request_data(request);

	// restrict buffer size to the one specified in cdb
	request->data_length = param_list_length_10;
	return true;

err:
	release_emulation_buffer(request);

	set_sense(request, SCSIS_KEY_ILLEGAL_REQUEST, SCSIS_ASC_INV_PARAM_LIST_FIELD);
	return false;
}


/*!	Emulation procedure at start of command
	returns false if command is to be finished without further execution
*/
bool
scsi_start_emulation(scsi_ccb *request)
{
	//snooze( 1000000 );

	SHOW_FLOW(3, "command=%x", request->cdb[0]);

	memcpy(request->orig_cdb, request->cdb, SCSI_MAX_CDB_SIZE);
	request->orig_cdb_length = request->cdb_length;

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


/*! Back-translate MODE SENSE 10 to MODE SENSE 6 */
static void
scsi_finish_mode_sense_10_6(scsi_ccb *request)
{
	scsi_device_info *device = request->device;
	scsi_mode_param_header_6 header_6;
	scsi_mode_param_header_10 *header_10 = (scsi_mode_param_header_10 *)device->buffer;
	int transfer_size_6, transfer_size_10;

	if (request->subsys_status != SCSI_REQ_CMP
		|| request->device_status != SCSI_STATUS_GOOD) {
		// on error, do nothing
		restore_request_data(request);
		release_emulation_buffer(request);
		return;
	}

	// check how much data we got from device and thus will copy into
	// request data
	transfer_size_10 = request->data_length - request->data_resid;
	transfer_size_6 = transfer_size_10
		- sizeof(scsi_mode_param_header_10 *) + sizeof(scsi_mode_param_header_6 *);

	SHOW_FLOW(0, "fixing MODE SENSE(6) (%d bytes)", transfer_size_6);

	restore_request_data(request);

	// adapt header

	// convert total length
	// (+1 is there because mode_data_len in 10 byte header ignores first
	// two bytes, whereas in the 6 byte header it ignores only one byte)
	header_6.mode_data_length = B_BENDIAN_TO_HOST_INT16(header_10->mode_data_length)
		- sizeof(scsi_mode_param_header_10) + sizeof(scsi_mode_param_header_6)
		+ 1;

	header_6.medium_type = header_10->medium_type;
	header_6.dev_spec_parameter = header_10->dev_spec_parameter;
	header_6.block_desc_length = B_BENDIAN_TO_HOST_INT16(header_10->block_desc_length);

	// copy adapted header
	copy_sg_data(request, 0, transfer_size_6, &header_6, sizeof(header_6), false);

	// copy remaining data
	copy_sg_data(request, sizeof(header_6), transfer_size_6,
		header_10 + 1, transfer_size_10 - sizeof(*header_10), false);

	request->data_resid = request->data_length - transfer_size_6;

	release_emulation_buffer(request);
}


/*! Back-translate MODE SELECT 10 to MODE SELECT 6 */
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


/*! Fix inquiry data; some ATAPI devices return wrong version */
static void
scsi_finish_inquiry(scsi_ccb *request)
{
	int transferSize;
	scsi_res_inquiry res;

	SHOW_FLOW0(3, "fixing INQUIRY");

	if (request->subsys_status != SCSI_REQ_CMP
		|| request->device_status != SCSI_STATUS_GOOD)
		return;

	transferSize = request->data_length - request->data_resid;

	copy_sg_data(request, 0, transferSize, &res, sizeof(res), true);

	SHOW_FLOW(3, "ANSI version: %d, response data format: %d",
		res.ansi_version, res.response_data_format);

	res.ansi_version = 2;
	res.response_data_format = 2;

	copy_sg_data(request, 0, transferSize, &res, sizeof(res), false);
}


/*! Adjust result of emulated request */
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
	request->cdb_length = request->orig_cdb_length;
}


/*! Set sense of request */
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


/*!	Copy data between request data and buffer
	request			- request to copy data from/to
	offset			- offset of data in request
	allocation_length- limit of request's data buffer according to CDB
	buffer			- data to copy data from/to
	size			- number of bytes to copy
	to_buffer		- true: copy from request to buffer
					  false: copy from buffer to request
	return: true, if data of request was large enough
*/
static bool
copy_sg_data(scsi_ccb *request, uint offset, uint allocation_length,
	void *buffer, int size, bool to_buffer)
{
	const physical_entry *sg_list = request->sg_list;
	int sg_count = request->sg_count;
	int req_size;

	SHOW_FLOW(3, "offset=%u, req_size_limit=%d, size=%d, sg_list=%p, sg_count=%d, %s buffer",
		offset, allocation_length, size, sg_list, sg_count, to_buffer ? "to" : "from");

	// skip unused S/G entries
	while (sg_count > 0 && offset >= sg_list->size) {
		offset -= sg_list->size;
		++sg_list;
		--sg_count;
	}

	if (sg_count == 0)
		return 0;

	// remaining bytes we are allowed to copy from/to request
	req_size = min_c(allocation_length, request->data_length) - offset;

	// copy one S/G entry at a time
	for (; size > 0 && req_size > 0 && sg_count > 0; ++sg_list, --sg_count) {
		size_t bytes;

		bytes = min_c(size, req_size);
		bytes = min_c(bytes, sg_list->size);

		SHOW_FLOW(0, "buffer = %p, virt_addr = %#" B_PRIxPHYSADDR ", bytes = %"
			B_PRIuSIZE ", to_buffer = %d", buffer, sg_list->address + offset,
			bytes, to_buffer);

		if (to_buffer) {
			vm_memcpy_from_physical(buffer, sg_list->address + offset, bytes,
				false);
		} else {
			vm_memcpy_to_physical(sg_list->address + offset, buffer, bytes,
				false);
		}

		buffer = (char *)buffer + bytes;
		size -= bytes;
		offset = 0;
	}

	return size == 0;
}


/*! Allocate emulation buffer */
static void
get_emulation_buffer(scsi_ccb *request)
{
	scsi_device_info *device = request->device;

	SHOW_FLOW0(3, "");

	acquire_sem(device->buffer_sem);
}


/*!	Replace request data with emulation buffer, saving original pointer;
	you must have called get_emulation_buffer() first
*/
static void
replace_request_data(scsi_ccb *request)
{
	scsi_device_info *device = request->device;

	SHOW_FLOW0(3, "");

	request->orig_sg_list = request->sg_list;
	request->orig_sg_count = request->sg_count;
	request->orig_data_length = request->data_length;

	request->sg_list = device->buffer_sg_list;
	request->sg_count = device->buffer_sg_count;
	request->data_length = device->buffer_size;
}


/*! Release emulation buffer */
static void
release_emulation_buffer(scsi_ccb *request)
{
	SHOW_FLOW0(3, "");

	release_sem(request->device->buffer_sem);
}


/*! Restore original request data pointers */
static void
restore_request_data(scsi_ccb *request)
{
	SHOW_FLOW0(3, "");

	request->sg_list = request->orig_sg_list;
	request->sg_count = request->orig_sg_count;
	request->data_length = request->orig_data_length;
}
