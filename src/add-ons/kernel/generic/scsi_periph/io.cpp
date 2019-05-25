/*
 * Copyright 2004-2011, Haiku, Inc. All Rights Reserved.
 * Copyright 2002-03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


//!	Everything doing the real input/output stuff.


#include "scsi_periph_int.h"
#include <scsi.h>

#include <string.h>
#include <stdlib.h>

#include <AutoDeleter.h>
#include <kernel.h>
#include <syscall_restart.h>


static status_t
inquiry(scsi_periph_device_info *device, scsi_inquiry *inquiry)
{
	const scsi_res_inquiry *device_inquiry = NULL;
	size_t inquiryLength;

	if (gDeviceManager->get_attr_raw(device->node, SCSI_DEVICE_INQUIRY_ITEM,
			(const void **)&device_inquiry, &inquiryLength, true) != B_OK)
		return B_ERROR;

	if (IS_USER_ADDRESS(inquiry)) {
		if (user_memcpy(&inquiry, device_inquiry,
			min_c(inquiryLength, sizeof(scsi_inquiry))) != B_OK) {
			return B_BAD_ADDRESS;
		}
	} else if (is_called_via_syscall()) {
		return B_BAD_ADDRESS;
	} else {
		memcpy(&inquiry, device_inquiry,
			min_c(inquiryLength, sizeof(scsi_inquiry)));
	}
	return B_OK;
}


static status_t
vpd_page_inquiry(scsi_periph_device_info *device, uint8 page, void* data,
	uint16 length)
{
	SHOW_FLOW0(0, "");

	scsi_ccb* ccb = device->scsi->alloc_ccb(device->scsi_device);
	if (ccb == NULL)
		return B_NO_MEMORY;

	scsi_cmd_inquiry *cmd = (scsi_cmd_inquiry *)ccb->cdb;
	memset(cmd, 0, sizeof(scsi_cmd_inquiry));
	cmd->opcode = SCSI_OP_INQUIRY;
	cmd->lun = ccb->target_lun;
	cmd->evpd = 1;
	cmd->page_code = page;
	cmd->allocation_length = length;

	ccb->flags = SCSI_DIR_IN;
	ccb->cdb_length = sizeof(scsi_cmd_inquiry);

	ccb->sort = -1;
	ccb->timeout = device->std_timeout;

	ccb->data = (uint8*)data;
	ccb->sg_list = NULL;
	ccb->data_length = length;

	status_t status = periph_safe_exec(device, ccb);

	device->scsi->free_ccb(ccb);

	return status;
}


status_t
vpd_page_get(scsi_periph_device_info *device, uint8 page, void* data,
	uint16 length)
{
	SHOW_FLOW0(0, "");

	status_t status = vpd_page_inquiry(device, 0, data, length);
	if (status != B_OK)
		return status; // or B_BAD_VALUE

	if (page == 0)
		return B_OK;

	scsi_page_list *list_data = (scsi_page_list*)data;
	int page_length = min_c(list_data->page_length, length -
		offsetof(scsi_page_list, pages));
	for (int i = 0; i < page_length; i++) {
		if (list_data->pages[i] == page)
			return vpd_page_inquiry(device, page, data, length);
	}

	// TODO buffer might be not big enough

	return B_BAD_VALUE;
}



static status_t
prevent_allow(scsi_periph_device_info *device, bool prevent)
{
	scsi_cmd_prevent_allow cmd;

	SHOW_FLOW0(0, "");

	memset(&cmd, 0, sizeof(cmd));
	cmd.opcode = SCSI_OP_PREVENT_ALLOW;
	cmd.prevent = prevent;

	return periph_simple_exec(device, (uint8 *)&cmd, sizeof(cmd), NULL, 0,
		SCSI_DIR_NONE);
}


/*! Keep this in sync with scsi_raw driver!!! */
static status_t
raw_command(scsi_periph_device_info *device, raw_device_command *cmd)
{
	scsi_ccb *request;

	SHOW_FLOW0(0, "");

	request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	request->flags = 0;
	bool dataIn = (cmd->flags & B_RAW_DEVICE_DATA_IN) != 0;
	bool dataOut = !dataIn && cmd->data_length != 0;

	void* buffer = cmd->data;
	MemoryDeleter bufferDeleter;
	if (buffer != NULL) {
		if (IS_USER_ADDRESS(buffer)) {
			buffer = malloc(cmd->data_length);
			if (buffer == NULL) {
				device->scsi->free_ccb(request);
				return B_NO_MEMORY;
			}
			bufferDeleter.SetTo(buffer);
			if (dataOut
				&& user_memcpy(buffer, cmd->data, cmd->data_length) != B_OK) {
				goto bad_address;
			}
		} else {
			if (is_called_via_syscall())
				goto bad_address;
		}
	}

	if (dataIn)
		request->flags |= SCSI_DIR_IN;
	else if (dataOut)
		request->flags |= SCSI_DIR_OUT;
	else
		request->flags |= SCSI_DIR_NONE;

	request->data = (uint8*)buffer;
	request->sg_list = NULL;
	request->data_length = cmd->data_length;
	request->sort = -1;
	request->timeout = cmd->timeout;

	memcpy(request->cdb, cmd->command, SCSI_MAX_CDB_SIZE);
	request->cdb_length = cmd->command_length;

	device->scsi->sync_io(request);

	// TBD: should we call standard error handler here, or may the
	// actions done there (like starting the unit) confuse the application?

	cmd->cam_status = request->subsys_status;
	cmd->scsi_status = request->device_status;

	if ((request->subsys_status & SCSI_AUTOSNS_VALID) != 0
		&& cmd->sense_data != NULL) {
		size_t length = min_c(cmd->sense_data_length,
			(size_t)SCSI_MAX_SENSE_SIZE - request->sense_resid);
		if (IS_USER_ADDRESS(cmd->sense_data)) {
			if (user_memcpy(cmd->sense_data, request->sense, length) != B_OK)
				goto bad_address;
		} else if (is_called_via_syscall()) {
			goto bad_address;
		} else {
			memcpy(cmd->sense_data, request->sense, length);
		}
	}

	if (dataIn && user_memcpy(cmd->data, buffer, cmd->data_length) != B_OK)
		goto bad_address;

	if ((cmd->flags & B_RAW_DEVICE_REPORT_RESIDUAL) != 0) {
		// this is a bit strange, see Be's sample code where I pinched this from;
		// normally, residual means "number of unused bytes left"
		// but here, we have to return "number of used bytes", which is the opposite
		cmd->data_length = cmd->data_length - request->data_resid;
		cmd->sense_data_length = SCSI_MAX_SENSE_SIZE - request->sense_resid;
	}

	device->scsi->free_ccb(request);

	return B_OK;

bad_address:
	device->scsi->free_ccb(request);

	return B_BAD_ADDRESS;
}


/*! Universal read/write function */
static status_t
read_write(scsi_periph_device_info *device, scsi_ccb *request,
	io_operation *operation, uint64 offset, size_t originalNumBlocks,
	physical_entry* vecs, size_t vecCount, bool isWrite,
	size_t* _bytesTransferred)
{
	uint32 blockSize = device->block_size;
	size_t numBlocks = originalNumBlocks;
	uint32 pos = offset;
	err_res res;
	int retries = 0;

	do {
		size_t numBytes;
		bool isReadWrite10 = false;

		request->flags = isWrite ? SCSI_DIR_OUT : SCSI_DIR_IN;

		// io_operations are generated by a DMAResource and thus contain DMA
		// safe physical vectors
		if (operation != NULL)
			request->flags |= SCSI_DMA_SAFE;

		// make sure we avoid 10 byte commands if they aren't supported
		if (!device->rw10_enabled || device->preferred_ccb_size == 6) {
			// restricting transfer is OK - the block manager will
			// take care of transferring the rest
			if (numBlocks > 0x100)
				numBlocks = 0x100;

			// no way to break the 21 bit address limit
			if (offset > 0x200000)
				return B_BAD_VALUE;

			// don't allow transfer cross the 24 bit address limit
			// (I'm not sure whether this is allowed, but this way we
			// are sure to not ask for trouble)
			if (offset < 0x100000)
				numBlocks = min_c(numBlocks, 0x100000 - pos);
		}

		numBytes = numBlocks * blockSize;
		if (numBlocks != originalNumBlocks)
			panic("I/O operation would need to be cut.");

		request->data = NULL;
		request->sg_list = vecs;
		request->data_length = numBytes;
		request->sg_count = vecCount;
		request->io_operation = operation;
		request->sort = pos;
		request->timeout = device->std_timeout;
		// see whether daemon instructed us to post an ordered command;
		// reset flag after read
		SHOW_FLOW(3, "flag=%x, next_tag=%x, ordered: %s",
			(int)request->flags, (int)device->next_tag_action,
			(request->flags & SCSI_ORDERED_QTAG) != 0 ? "yes" : "no");

		// use shortest commands whenever possible
		if (offset + numBlocks < 0x200000LL && numBlocks <= 0x100) {
			scsi_cmd_rw_6 *cmd = (scsi_cmd_rw_6 *)request->cdb;

			isReadWrite10 = false;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = isWrite ? SCSI_OP_WRITE_6 : SCSI_OP_READ_6;
			cmd->high_lba = (pos >> 16) & 0x1f;
			cmd->mid_lba = (pos >> 8) & 0xff;
			cmd->low_lba = pos & 0xff;
			cmd->length = numBlocks;

			request->cdb_length = sizeof(*cmd);
		} else if (offset + numBlocks < 0x100000000LL && numBlocks <= 0x10000) {
			scsi_cmd_rw_10 *cmd = (scsi_cmd_rw_10 *)request->cdb;

			isReadWrite10 = true;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = isWrite ? SCSI_OP_WRITE_10 : SCSI_OP_READ_10;
			cmd->relative_address = 0;
			cmd->force_unit_access = 0;
			cmd->disable_page_out = 0;
			cmd->lba = B_HOST_TO_BENDIAN_INT32(pos);
			cmd->length = B_HOST_TO_BENDIAN_INT16(numBlocks);

			request->cdb_length = sizeof(*cmd);
		} else if (offset + numBlocks < 0x100000000LL && numBlocks <= 0x10000000) {
			scsi_cmd_rw_12 *cmd = (scsi_cmd_rw_12 *)request->cdb;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = isWrite ? SCSI_OP_WRITE_12 : SCSI_OP_READ_12;
			cmd->relative_address = 0;
			cmd->force_unit_access = 0;
			cmd->disable_page_out = 0;
			cmd->lba = B_HOST_TO_BENDIAN_INT32(pos);
			cmd->length = B_HOST_TO_BENDIAN_INT32(numBlocks);

			request->cdb_length = sizeof(*cmd);
		} else {
			scsi_cmd_rw_16 *cmd = (scsi_cmd_rw_16 *)request->cdb;

			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = isWrite ? SCSI_OP_WRITE_16 : SCSI_OP_READ_16;
			cmd->force_unit_access_non_volatile = 0;
			cmd->force_unit_access = 0;
			cmd->disable_page_out = 0;
			cmd->lba = B_HOST_TO_BENDIAN_INT64(offset);
			cmd->length = B_HOST_TO_BENDIAN_INT32(numBlocks);

			request->cdb_length = sizeof(*cmd);
		}

		// TODO: last chance to detect errors that occured during concurrent accesses
		//status_t status = handle->pending_error;
		//if (status != B_OK)
		//	return status;

		device->scsi->async_io(request);

		acquire_sem(request->completion_sem);

		// ask generic peripheral layer what to do now
		res = periph_check_error(device, request);

		// TODO: bytes might have been transferred even in the error case!
		switch (res.action) {
			case err_act_ok:
				*_bytesTransferred = numBytes - request->data_resid;
				break;

			case err_act_start:
				res = periph_send_start_stop(device, request, 1,
					device->removable);
				if (res.action == err_act_ok)
					res.action = err_act_retry;
				break;

			case err_act_invalid_req:
				// if this was a 10 byte command, the device probably doesn't
				// support them, so disable them and retry
				if (isReadWrite10) {
					atomic_and(&device->rw10_enabled, 0);
					res.action = err_act_retry;
				} else
					res.action = err_act_fail;
				break;
		}
	} while ((res.action == err_act_retry && retries++ < 3)
		|| (res.action == err_act_many_retries && retries++ < 30));

	// peripheral layer only created "read" error, so we have to
	// map them to "write" errors if this was a write request
	if (res.error_code == B_DEV_READ_ERROR && isWrite)
		return B_DEV_WRITE_ERROR;

	return res.error_code;
}


// #pragma mark - public functions


status_t
periph_ioctl(scsi_periph_handle_info *handle, int op, void *buffer,
	size_t length)
{
	switch (op) {
		case B_GET_MEDIA_STATUS:
		{
			status_t status = B_OK;
			if (length < sizeof(status))
				return B_BUFFER_OVERFLOW;

			if (handle->device->removable)
				status = periph_get_media_status(handle);

			SHOW_FLOW(2, "%s", strerror(status));

			if (IS_USER_ADDRESS(buffer)) {
				if (user_memcpy(buffer, &status, sizeof(status_t)) != B_OK)
					return B_BAD_ADDRESS;
			} else if (is_called_via_syscall()) {
				return B_BAD_ADDRESS;
			} else
				*(status_t *)buffer = status;
			return B_OK;
		}

		case B_GET_DEVICE_NAME:
		{
			// TODO: this should be written as an attribute to the node
			// Try driver further up first
			if (handle->device->scsi->ioctl != NULL) {
				status_t status = handle->device->scsi->ioctl(
					handle->device->scsi_device, op, buffer, length);
				if (status == B_OK)
					return B_OK;
			}

			// If that fails, get SCSI vendor/product
			const char* vendor;
			if (gDeviceManager->get_attr_string(handle->device->node,
					SCSI_DEVICE_VENDOR_ITEM, &vendor, true) == B_OK) {
				char name[B_FILE_NAME_LENGTH];
				strlcpy(name, vendor, sizeof(name));

				const char* product;
				if (gDeviceManager->get_attr_string(handle->device->node,
						SCSI_DEVICE_PRODUCT_ITEM, &product, true) == B_OK) {
					strlcat(name, " ", sizeof(name));
					strlcat(name, product, sizeof(name));
				}

				return user_strlcpy((char*)buffer, name, length) >= 0
					? B_OK : B_BAD_ADDRESS;
			}
			return B_ERROR;
		}

		case B_SCSI_INQUIRY:
			if (length < sizeof(scsi_inquiry))
				return B_BUFFER_OVERFLOW;
			return inquiry(handle->device, (scsi_inquiry *)buffer);

		case B_SCSI_PREVENT_ALLOW:
			bool result;
			if (length < sizeof(result))
				return B_BUFFER_OVERFLOW;
			if (IS_USER_ADDRESS(buffer)) {
				if (user_memcpy(&result, buffer, sizeof(result)) != B_OK)
					return B_BAD_ADDRESS;
			} else if (is_called_via_syscall()) {
				return B_BAD_ADDRESS;
			} else
				result = *(bool*)buffer;

			return prevent_allow(handle->device, result);

		case B_RAW_DEVICE_COMMAND:
		{
			raw_device_command command;
			raw_device_command* commandBuffer;
			if (length < sizeof(command))
				return B_BUFFER_OVERFLOW;
			if (IS_USER_ADDRESS(buffer)) {
				if (user_memcpy(&command, buffer, sizeof(command)) != B_OK)
					return B_BAD_ADDRESS;
				commandBuffer = &command;
			} else if (is_called_via_syscall()) {
				return B_BAD_ADDRESS;
			} else {
				commandBuffer = (raw_device_command*)buffer;
			}
			status_t status = raw_command(handle->device, commandBuffer);
			if (status == B_OK && commandBuffer == &command) {
				if (user_memcpy(buffer, &command, sizeof(command)) != B_OK)
					status = B_BAD_ADDRESS;
			}
			return status;
		}
		default:
			if (handle->device->scsi->ioctl != NULL) {
				return handle->device->scsi->ioctl(handle->device->scsi_device,
					op, buffer, length);
			}

			SHOW_ERROR(4, "Unknown ioctl: %x", op);
			return B_DEV_INVALID_IOCTL;
	}
}


/*!	Kernel daemon - once in a minute, it sets a flag so that the next command
	is executed ordered; this way, we avoid starvation of SCSI commands inside
	the SCSI queuing system - the ordered command waits for all previous
	commands and thus no command can starve longer then a minute
*/
void
periph_sync_queue_daemon(void *arg, int iteration)
{
	scsi_periph_device_info *device = (scsi_periph_device_info *)arg;

	SHOW_FLOW0(3, "Setting ordered flag for next R/W access");
	atomic_or(&device->next_tag_action, SCSI_ORDERED_QTAG);
}


status_t
periph_read_write(scsi_periph_device_info *device, scsi_ccb *request,
	uint64 offset, size_t numBlocks, physical_entry* vecs, size_t vecCount,
	bool isWrite, size_t* _bytesTransferred)
{
	return read_write(device, request, NULL, offset, numBlocks, vecs, vecCount,
		isWrite, _bytesTransferred);
}


status_t
periph_io(scsi_periph_device_info *device, io_operation *operation,
	size_t* _bytesTransferred)
{
	const uint32 blockSize = device->block_size;
	if (blockSize == 0)
		return B_BAD_VALUE;

	// don't test rw10_enabled restrictions - this flag may get changed
	scsi_ccb *request = device->scsi->alloc_ccb(device->scsi_device);
	if (request == NULL)
		return B_NO_MEMORY;

	status_t status = read_write(device, request, operation,
		operation->Offset() / blockSize, operation->Length() / blockSize,
		(physical_entry *)operation->Vecs(), operation->VecCount(),
		operation->IsWrite(), _bytesTransferred);

	device->scsi->free_ccb(request);
	return status;
}

