/*
 * Copyright 2021 David Sebek, dasebek@gmail.com
 * Copyright 2004-2013 Haiku, Inc.
 * Copyright 2002-2003 Thomas Kurschel
 * All rights reserved. Distributed under the terms of the MIT License.
 */


//!	Handling of block device


#include <string.h>

#include <AutoDeleter.h>

#include "scsi_periph_int.h"


// UNMAP command limits
#define UNMAP_MAX_LBA_VALUE				UINT64_MAX
#define UNMAP_MAX_BLOCK_COUNT_VALUE		UINT32_MAX
#define UNMAP_MAX_DESCRIPTORS			4095
	// Limit imposed by the UNMAP command structure
#define UNMAP_DEFAULT_DESCRIPTORS		255
	// Reasonable default (?) when not specified by the device

// WRITE SAME (16) command limits
#define WS16_MAX_LBA_VALUE				UINT64_MAX
#define WS16_MAX_BLOCK_COUNT_VALUE		UINT32_MAX

// WRITE SAME (10) command limits
#define WS10_MAX_LBA_VALUE				UINT32_MAX
#define WS10_MAX_BLOCK_COUNT_VALUE		UINT16_MAX


struct CapacityInfo {
	// Result of the READ CAPACITY command
	bool capacityFilled;
	uint64 lastLba;
	uint32 blockSize;

	// Provisioining info from READ CAPACITY
	bool provisioningFilled;
	bool lbpme;
	bool lbprz;
};


struct UnmapSupport {
	// UNMAP commands supported by the device
	bool commandSupportFilled;
	bool unmapSupported;
	bool ws16Supported;
	bool ws10Supported;

	// Block limits for UNMAP commands
	bool blockLimitsFilled;
	uint32 maxUnmapLbaCount;
	uint32 maxUnmapDescriptorCount;
	uint64 maxWritesameLength;
};


static bool
prefer_read_capacity_16(scsi_periph_device_info* device)
{
	const scsi_res_inquiry* inquiryData = NULL;
	size_t inquiryDataLength;

	if (gDeviceManager->get_attr_raw(device->node, SCSI_DEVICE_INQUIRY_ITEM,
				(const void**)&inquiryData, &inquiryDataLength, true) != B_OK
		|| inquiryDataLength != sizeof(*inquiryData)) {
		return false;
	}

	if (inquiryData->protect)
		return true;

	if (inquiryData->ansi_version > 0x04 /* SPC-2 */)
		return true;

	return false;
}


static bool
vpd_pages_supported(scsi_periph_device_info* device)
{
	const scsi_res_inquiry* inquiryData = NULL;
	size_t inquiryDataLength;

	if (gDeviceManager->get_attr_raw(device->node, SCSI_DEVICE_INQUIRY_ITEM,
				(const void**)&inquiryData, &inquiryDataLength, true) != B_OK
		|| inquiryDataLength != sizeof(*inquiryData)) {
		return false;
	}

	if (inquiryData->ansi_version >= 0x04 /* SPC-2 */)
		return true;

	return false;
}


static status_t
read_capacity_10(scsi_periph_device_info* device, scsi_ccb* request,
	CapacityInfo* capacityInfo)
{
	capacityInfo->capacityFilled = false;
	capacityInfo->provisioningFilled = false;

	scsi_res_read_capacity capacityResult;
	memset(&capacityResult, 0, sizeof(capacityResult));

	scsi_cmd_read_capacity* cmd = (scsi_cmd_read_capacity*)request->cdb;
	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_READ_CAPACITY;
	// we don't set PMI (partial medium indicator) as we want the whole capacity;
	// in this case, all other parameters must be zero

	request->flags = SCSI_DIR_IN;
	request->cdb_length = sizeof(*cmd);
	request->sort = -1;
	request->timeout = device->std_timeout;

	request->data = (uint8*)&capacityResult;
	request->data_length = sizeof(capacityResult);
	request->sg_list = NULL;

	status_t res = periph_safe_exec(device, request);

	if (res == B_OK && request->data_resid == 0) {
		capacityInfo->capacityFilled = true;
		capacityInfo->lastLba
			= (uint32)B_BENDIAN_TO_HOST_INT32(capacityResult.lba);
		capacityInfo->blockSize
			= B_BENDIAN_TO_HOST_INT32(capacityResult.block_size);
	}

	return res;
}


static status_t
read_capacity_16(scsi_periph_device_info* device, scsi_ccb* request,
	CapacityInfo* capacityInfo)
{
	capacityInfo->capacityFilled = false;
	capacityInfo->provisioningFilled = false;

	scsi_res_read_capacity_long capacityLongResult;
	memset(&capacityLongResult, 0, sizeof(capacityLongResult));

	scsi_cmd_read_capacity_long* cmd
		= (scsi_cmd_read_capacity_long*)request->cdb;
	memset(cmd, 0, sizeof(*cmd));
	cmd->opcode = SCSI_OP_SERVICE_ACTION_IN;
	cmd->service_action = SCSI_SAI_READ_CAPACITY_16;
	cmd->alloc_length = B_HOST_TO_BENDIAN_INT32(sizeof(capacityLongResult));

	request->flags = SCSI_DIR_IN;
	request->cdb_length = sizeof(*cmd);
	request->sort = -1;
	request->timeout = device->std_timeout;

	request->data = (uint8*)&capacityLongResult;
	request->data_length = sizeof(capacityLongResult);
	request->sg_list = NULL;

	status_t res = periph_safe_exec(device, request);

	if (res == B_OK && request->data_resid
			<= (int32)sizeof(scsi_res_read_capacity_long) - 12) {
		// At least the last LBA and sector size have been transfered
		capacityInfo->capacityFilled = true;
		capacityInfo->lastLba
			= B_BENDIAN_TO_HOST_INT64(capacityLongResult.lba);
		capacityInfo->blockSize
			= B_BENDIAN_TO_HOST_INT32(capacityLongResult.block_size);
	}

	if (res == B_OK && request->data_resid
			<= (int32)sizeof(scsi_res_read_capacity_long) - 15) {
		// lbpme and lbprz bits were received too
		capacityInfo->provisioningFilled = true;
		capacityInfo->lbpme = capacityLongResult.lbpme;
		capacityInfo->lbprz = capacityLongResult.lbprz;
	}

	return res;
}


static status_t
get_unmap_commands(scsi_periph_device_info* device, scsi_ccb* request,
	UnmapSupport* unmapSupport)
{
	unmapSupport->commandSupportFilled = false;

	scsi_page_lb_provisioning vpdProvisioning;
	memset(&vpdProvisioning, 0, sizeof(vpdProvisioning));
	status_t vpdStatus = vpd_page_get(device, request,
		SCSI_PAGE_LB_PROVISIONING, &vpdProvisioning, sizeof(vpdProvisioning));

	if (vpdStatus == B_OK
		&& request->data_resid <= (int32)sizeof(scsi_page_lb_provisioning) - 6
		&& vpdProvisioning.page_code == SCSI_PAGE_LB_PROVISIONING
		&& B_BENDIAN_TO_HOST_INT16(vpdProvisioning.page_length) >= 2) {
		unmapSupport->commandSupportFilled = true;
		unmapSupport->unmapSupported = vpdProvisioning.lbpu;
		unmapSupport->ws16Supported = vpdProvisioning.lbpws;
		unmapSupport->ws10Supported = vpdProvisioning.lbpws10;
	}

	if (vpdStatus == B_BAD_VALUE)
		return B_ERROR;

	return vpdStatus;
}


static status_t
get_unmap_limits(scsi_periph_device_info* device, scsi_ccb* request,
	UnmapSupport* unmapSupport)
{
	unmapSupport->blockLimitsFilled = false;

	scsi_page_block_limits vpdBlockLimits;
	memset(&vpdBlockLimits, 0, sizeof(vpdBlockLimits));
	status_t vpdStatus = vpd_page_get(device, request,
		SCSI_PAGE_BLOCK_LIMITS, &vpdBlockLimits, sizeof(vpdBlockLimits));

	if (vpdStatus == B_OK
		&& request->data_resid <= (int32)sizeof(scsi_page_block_limits) - 44
		&& vpdBlockLimits.page_code == SCSI_PAGE_BLOCK_LIMITS
		&& B_BENDIAN_TO_HOST_INT16(vpdBlockLimits.page_length) == 0x3c) {
		unmapSupport->blockLimitsFilled = true;
		unmapSupport->maxUnmapLbaCount = B_BENDIAN_TO_HOST_INT32(
			vpdBlockLimits.max_unmap_lba_count);
		unmapSupport->maxUnmapDescriptorCount = B_BENDIAN_TO_HOST_INT32(
			vpdBlockLimits.max_unmap_blk_count);
		unmapSupport->maxWritesameLength = B_BENDIAN_TO_HOST_INT64(
			vpdBlockLimits.max_write_same_length);
	}

	if (vpdStatus == B_BAD_VALUE)
		return B_ERROR;

	return vpdStatus;
}


static void
determine_unmap_support(const UnmapSupport* unmapSupport,
	enum trim_command* unmapCommand, uint32* maxLbaCount,
	uint32* maxDescriptorCount)
{
#ifdef DEBUG_TRIM
	if (unmapSupport->commandSupportFilled)
		dprintf("TRIM: device reports (LBP VPD): LBPU = %d, LBPWS = %d,"
			" LBPWS10 = %d\n", unmapSupport->unmapSupported,
			unmapSupport->ws16Supported, unmapSupport->ws10Supported);
	else
		dprintf("TRIM: could not get the LBP VPD of the device\n");
	if (unmapSupport->blockLimitsFilled)
		dprintf("TRIM: device reports (Block Limits VPD):"
			"\nTRIM: MAXIMUM UNMAP LBA COUNT = %" B_PRIu32
			"\nTRIM: MAXIMUM UNMAP BLOCK DESCRIPTOR COUNT = %" B_PRIu32
			"\nTRIM: MAXIMUM WRITESAME LENGTH = %" B_PRIu64 "\n",
			unmapSupport->maxUnmapLbaCount,
			unmapSupport->maxUnmapDescriptorCount,
			unmapSupport->maxWritesameLength);
	else
		dprintf("TRIM: could not get Block Limits VPD of the device\n");
#endif

	*unmapCommand = TRIM_NONE;
	*maxLbaCount = 0;
	*maxDescriptorCount = 0;

	if (!unmapSupport->commandSupportFilled
		|| !unmapSupport->blockLimitsFilled)
		return;

	if (unmapSupport->unmapSupported
		&& unmapSupport->maxUnmapLbaCount > 0
		&& unmapSupport->maxUnmapDescriptorCount > 0) {
		*unmapCommand = TRIM_UNMAP;
		*maxLbaCount = unmapSupport->maxUnmapLbaCount;
		if (unmapSupport->maxUnmapDescriptorCount == UINT32_MAX
			|| unmapSupport->maxUnmapDescriptorCount > UNMAP_MAX_DESCRIPTORS) {
			// Choose a reasonable value instead
			*maxDescriptorCount = UNMAP_DEFAULT_DESCRIPTORS;
		} else {
			*maxDescriptorCount = unmapSupport->maxUnmapDescriptorCount;
		}
	}

	if (*unmapCommand == TRIM_NONE && unmapSupport->ws16Supported) {
		uint64 maxLength = unmapSupport->maxWritesameLength;
		if (maxLength == 0) {
			// WRITE SAME limit not reported, try UNMAP limit instead
			if (unmapSupport->maxUnmapLbaCount > 0)
				maxLength = unmapSupport->maxUnmapLbaCount;
			else
				maxLength = WS16_MAX_BLOCK_COUNT_VALUE;
		}
		*unmapCommand = TRIM_WRITESAME16;
		*maxLbaCount = min_c(maxLength, WS16_MAX_BLOCK_COUNT_VALUE);
		*maxDescriptorCount = 1;
	}

	if (*unmapCommand == TRIM_NONE && unmapSupport->ws10Supported) {
		uint64 maxLength = unmapSupport->maxWritesameLength;
		if (maxLength == 0) {
			// WRITE SAME limit not reported, try UNMAP limit instead
			if (unmapSupport->maxUnmapLbaCount > 0)
				maxLength = unmapSupport->maxUnmapLbaCount;
			else
				maxLength = WS10_MAX_BLOCK_COUNT_VALUE;
		}
		*unmapCommand = TRIM_WRITESAME10;
		*maxLbaCount = min_c(maxLength, WS10_MAX_BLOCK_COUNT_VALUE);
		*maxDescriptorCount = 1;
	}
}


status_t
periph_check_capacity(scsi_periph_device_info* device, scsi_ccb* request)
{
	CapacityInfo capacityInfo = {0};
	status_t res;

	SHOW_FLOW(3, "%p, %p", device, request);

	// driver doesn't support capacity callback - seems to be no block
	// device driver, so ignore
	if (device->callbacks->set_capacity == NULL)
		return B_OK;

	if (prefer_read_capacity_16(device)) {
		SHOW_FLOW0(3, "READ CAPACITY 16 tried first");
		res = read_capacity_16(device, request, &capacityInfo);

		if (res == B_ERROR) {
			SHOW_FLOW0(3, "READ CAPACITY 16 failed, trying READ CAPACITY 10");
			res = read_capacity_10(device, request, &capacityInfo);
		}
	} else {
		SHOW_FLOW0(3, "READ CAPACITY 10 tried first");
		res = read_capacity_10(device, request, &capacityInfo);

		if (res == B_OK && capacityInfo.capacityFilled
			&& capacityInfo.lastLba == UINT32_MAX) {
			SHOW_FLOW0(3, "Device is too large, trying READ CAPACITY 16");
			res = read_capacity_16(device, request, &capacityInfo);
		}
	}

	uint64 capacity;
	uint32 blockSize;

	if (capacityInfo.capacityFilled) {
		capacity = capacityInfo.lastLba + 1;
		blockSize = capacityInfo.blockSize;
	} else {
		capacity = 0;
		blockSize = 0;
	}

	enum trim_command unmapCommand = TRIM_NONE;
	uint32 maxLbaCount = 0;
	uint32 maxDescriptorCount = 0;

	if (capacityInfo.provisioningFilled
		&& capacityInfo.lbpme
		&& vpd_pages_supported(device)) {
		UnmapSupport unmapSupport = {0};

		// Don't fail if the device doesn't support the command
		// but fail if some other error happens
		if (res == B_OK) {
			status_t vpdStatus = get_unmap_commands(device, request,
				&unmapSupport);
			if (vpdStatus != B_OK && vpdStatus != B_ERROR)
				res = vpdStatus;
		}

		if (res == B_OK) {
			status_t vpdStatus = get_unmap_limits(device, request,
				&unmapSupport);
			if (vpdStatus != B_OK && vpdStatus != B_ERROR)
				res = vpdStatus;
		}

		determine_unmap_support(&unmapSupport, &unmapCommand,
				&maxLbaCount, &maxDescriptorCount);

		if (maxLbaCount == 0 || maxDescriptorCount == 0)
			unmapCommand = TRIM_NONE;
	}

	if (res == B_DEV_MEDIA_CHANGED) {
		// in this case, the error handler has already called check_capacity
		// recursively, so we ignore our (invalid) result
		SHOW_FLOW0(3, "ignore result because medium change");
		return B_DEV_MEDIA_CHANGED;
	}

	if (res == B_OK && !capacityInfo.capacityFilled)
		// Although the capacity and block size will be set to 0 in this case,
		// it is also better to inform the caller that these values were not
		// reported by the device
		res = B_ERROR;

	SHOW_FLOW(3, "capacity = %" B_PRIu64 ", block_size = %" B_PRIu32
		" (%sreported)", capacity, blockSize,
		capacityInfo.capacityFilled ? "" : "not ");
	SHOW_INFO(1, "TRIM: Setting trim support to %s",
		unmapCommand == TRIM_NONE ? "disabled"
			: unmapCommand == TRIM_UNMAP ? "UNMAP"
			: unmapCommand == TRIM_WRITESAME16 ? "WRITE SAME (16)"
			: unmapCommand == TRIM_WRITESAME10 ? "WRITE SAME (10)"
			: "unknown");
	SHOW_FLOW(3, "TRIM: Block limits: size = %" B_PRIu32
		", descriptors = %" B_PRIu32, maxLbaCount, maxDescriptorCount);

	mutex_lock(&device->mutex);
		// Was there a reason why this mutex
		// was previously locked much earlier?

	device->unmap_command = unmapCommand;
	device->max_unmap_lba_count = maxLbaCount;
	device->max_unmap_descriptor_count = maxDescriptorCount;

	device->block_size = blockSize;

	device->callbacks->set_capacity(device->periph_device,
		capacity, blockSize);

/*	device->byte2blk_shift = log2( device->block_size );
	if( device->byte2blk_shift < 0 ) {
		// this may be too restrictive...
		device->capacity = -1;
		return ERR_DEV_GENERAL;
	}*/

	mutex_unlock(&device->mutex);

	SHOW_FLOW(3, "done (%s)", strerror(res));

	return res;
}


static status_t
trim_unmap(scsi_periph_device_info* device, scsi_ccb* request,
	scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks)
{
	uint64 maxLength = UNMAP_MAX_BLOCK_COUNT_VALUE;
	uint64 maxBlocksInRequest = device->max_unmap_lba_count;
	uint32 maxDescriptors = device->max_unmap_descriptor_count;

	*trimmedBlocks = 0;

	// Allocate a single buffer and re-use it between requests
	size_t expectedDescriptorCount = 0;
	for (uint32 i = 0; i < rangeCount; i++) {
		expectedDescriptorCount += ranges[i].size / maxLength;
		if (ranges[i].size % maxLength != 0)
			expectedDescriptorCount++;
	}
	expectedDescriptorCount = min_c(expectedDescriptorCount, maxDescriptors);

	size_t unmapListAllocatedSize = (expectedDescriptorCount - 1)
			* sizeof(scsi_unmap_block_descriptor)
		+ sizeof(scsi_unmap_parameter_list);

	scsi_unmap_parameter_list* unmapList
		= (scsi_unmap_parameter_list*)malloc(unmapListAllocatedSize);
	if (unmapList == NULL)
		return B_NO_MEMORY;

	MemoryDeleter deleter(unmapList);

	status_t status = B_OK;
	uint32 descriptorIndex = 0;
	uint64 trimmedBlocksInRequest = 0;
	memset(unmapList, 0, unmapListAllocatedSize);
	for (uint32 i = 0; i < rangeCount; i++) {
		uint64 lba = ranges[i].lba;
		uint64 length = ranges[i].size;

		if (length == 0)
			continue; // Length of 0 would be ignored by the device anyway

		if (lba > UNMAP_MAX_LBA_VALUE) {
			SHOW_ERROR0(1, "LBA value is too large!"
				" This unmap range will be skipped.");
			continue;
		}

		// Split large ranges if needed.
		// Range length is limited by:
		//   - the UNMAP_MAX_BLOCK_COUNT_VALUE constant
		//   - the total number of LBAs in one UNMAP command is limited by
		//     the MAX UNMAP LBA COUNT field in the Block Limits VPD page
		while (length > 0) {
			uint64 trimLength = min_c(length, maxLength);
			trimLength = min_c(trimLength,
					maxBlocksInRequest - trimmedBlocksInRequest);
			unmapList->blocks[descriptorIndex].lba
				= B_HOST_TO_BENDIAN_INT64(lba);
			unmapList->blocks[descriptorIndex].block_count
				= B_HOST_TO_BENDIAN_INT32(trimLength);
			descriptorIndex++;
			trimmedBlocksInRequest += trimLength;

			// Split into multiple requests if needed.
			// The number of UNMAP block descriptors is limited by:
			//   - the number of block descriptors cannot exceed the
			//     MAXIMUM UNMAP PARAMETER COUNT value in the Block Limits VPD
			//   - the size of our buffer
			//   - what fits in one UNMAP command
			//   - the total number of LBAs in one UNMAP command is limited by
			//     the MAX UNMAP LBA COUNT field in the Block Limits VPD page
			if (descriptorIndex >= maxDescriptors
				|| descriptorIndex >= expectedDescriptorCount
				|| descriptorIndex >= UNMAP_MAX_DESCRIPTORS
				|| trimmedBlocksInRequest >= maxBlocksInRequest
				|| (i == rangeCount - 1 && length <= maxLength))
			{
				uint16 unmapListSize = (descriptorIndex - 1)
						* sizeof(scsi_unmap_block_descriptor)
					+ sizeof(scsi_unmap_parameter_list);
				unmapList->data_length = B_HOST_TO_BENDIAN_INT16(unmapListSize
					- offsetof(scsi_unmap_parameter_list, block_data_length));
				unmapList->block_data_length
					= B_HOST_TO_BENDIAN_INT16(unmapListSize
						- offsetof(scsi_unmap_parameter_list, blocks));

				scsi_cmd_unmap* cmd = (scsi_cmd_unmap*)request->cdb;
				memset(cmd, 0, sizeof(*cmd));
				cmd->opcode = SCSI_OP_UNMAP;
				cmd->length = B_HOST_TO_BENDIAN_INT16(unmapListSize);

				request->flags = SCSI_DIR_OUT;
				request->cdb_length = sizeof(*cmd);
				request->sort = B_BENDIAN_TO_HOST_INT64(
					unmapList->blocks[0].lba);
				request->timeout = device->std_timeout;

				request->data = (uint8*)unmapList;
				request->data_length = unmapListSize;
				request->sg_list = NULL;

				SHOW_FLOW(3, "UNMAP data used %" B_PRIu16
					" of %" B_PRIuSIZE " allocated bytes",
					unmapListSize, unmapListAllocatedSize);

#ifdef DEBUG_TRIM
				uint16 scsiRangeCount = (uint16)B_BENDIAN_TO_HOST_INT16(
					unmapList->block_data_length)
					/ sizeof(scsi_unmap_block_descriptor);
				uint64 count = 0;
				dprintf("TRIM: SCSI: sending an UNMAP command to"
					" the device (blocks):\n");
				for (uint16 i = 0; i < scsiRangeCount; i++) {
					dprintf("[%3" B_PRIu16 "] %" B_PRIu64 " : %" B_PRIu32 "\n",
						i, (uint64)B_BENDIAN_TO_HOST_INT64(
							unmapList->blocks[i].lba),
						(uint32)B_BENDIAN_TO_HOST_INT32(
							unmapList->blocks[i].block_count));
					count += (uint32)B_BENDIAN_TO_HOST_INT32(
							unmapList->blocks[i].block_count);
				}
				if (device->max_unmap_lba_count >= count)
					dprintf("TRIM: SCSI: Previous UNMAP command would fit %"
						B_PRIu64 " more LBAs\n",
						device->max_unmap_lba_count - count);
				else
					dprintf("TRIM: SCSI: Previous UNMAP ranges exceed the"
						" device limit!\n");
#endif /* DEBUG_TRIM */

				status = periph_safe_exec(device, request);

				// peripheral layer only creates "read" error
				if (status == B_DEV_READ_ERROR)
					return B_DEV_WRITE_ERROR;
				else if (status != B_OK)
					return status;

				*trimmedBlocks += trimmedBlocksInRequest;

				descriptorIndex = 0;
				trimmedBlocksInRequest = 0;
				memset(unmapList, 0, unmapListSize);
			}

			length -= trimLength;
			lba += trimLength;
		}
	}

	return status;
}


static status_t
trim_writesame16(scsi_periph_device_info* device, scsi_ccb* request,
	scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks)
{
	status_t status = B_OK;
	*trimmedBlocks = 0;

	for (uint32 i = 0; i < rangeCount; i++) {
		uint64 lba = ranges[i].lba;
		uint64 length = ranges[i].size;

		if (length == 0)
			continue; // length of 0 would mean the rest of the device!

		if (lba > WS16_MAX_LBA_VALUE) {
			SHOW_ERROR0(1, "LBA value is too large!"
				" This unmap range will be skipped.");
			continue;
		}

		// Split the range into multiple requests if needed
		uint64 maxLength = min_c(device->max_unmap_lba_count,
				WS16_MAX_BLOCK_COUNT_VALUE);
		while (length > 0) {
			uint64 trimLength = min_c(length, maxLength);
			if (trimLength == 0) {
				SHOW_ERROR0(1,
					"Error: Length of zero in WRITE SAME (16) detected");
				break;
			}

			void* block = malloc(device->block_size);
			if (block == NULL)
				return B_NO_MEMORY;
			MemoryDeleter deleter(block);
			memset(block, 0, device->block_size);

			scsi_cmd_wsame_16* cmd = (scsi_cmd_wsame_16*)request->cdb;
			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = SCSI_OP_WRITE_SAME_16;
			cmd->unmap = 1;
			cmd->lba = B_HOST_TO_BENDIAN_INT64(lba);
			cmd->length = B_HOST_TO_BENDIAN_INT32(trimLength);
			//cmd->ndob = 1; // no data is needed if this bit is enabled

			request->flags = SCSI_DIR_OUT;
			request->cdb_length = sizeof(*cmd);
			request->sort = lba;
			request->timeout = device->std_timeout;

			request->data = (uint8*)block;
			request->data_length = device->block_size;
			request->sg_list = NULL;

#ifdef DEBUG_TRIM
			dprintf("TRIM: SCSI: sending a WRITE SAME (16) command to"
				" the device (blocks):\n");
			dprintf("%" B_PRIu64 " : %" B_PRIu32 "\n",
				(uint64)B_BENDIAN_TO_HOST_INT64(cmd->lba),
				(uint32)B_BENDIAN_TO_HOST_INT32(cmd->length));
#endif

			status = periph_safe_exec(device, request);

			// peripheral layer only creates "read" error
			if (status == B_DEV_READ_ERROR)
				return B_DEV_WRITE_ERROR;
			else if (status != B_OK)
				return status;

			*trimmedBlocks += trimLength;
			length -= trimLength;
			lba += trimLength;
		}
	}

	return status;
}


static status_t
trim_writesame10(scsi_periph_device_info* device, scsi_ccb* request,
	scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks)
{
	status_t status = B_OK;
	*trimmedBlocks = 0;

	for (uint32 i = 0; i < rangeCount; i++) {
		uint64 lba = ranges[i].lba;
		uint64 length = ranges[i].size;

		if (length == 0)
			continue; // length of 0 would mean the rest of the device!

		if (lba > WS10_MAX_LBA_VALUE) {
			SHOW_ERROR0(1, "LBA value is too large!"
				" This unmap range will be skipped.");
			continue;
		}

		// Split the range into multiple requests if needed
		uint64 maxLength = min_c(device->max_unmap_lba_count,
				WS10_MAX_BLOCK_COUNT_VALUE);
		while (length > 0) {
			uint64 trimLength = min_c(length, maxLength);
			if (trimLength == 0) {
				SHOW_ERROR0(1,
					"Error: Length of zero in WRITE SAME (10) detected");
				break;
			}

			void* block = malloc(device->block_size);
			if (block == NULL)
				return B_NO_MEMORY;
			MemoryDeleter deleter(block);
			memset(block, 0, device->block_size);

			scsi_cmd_wsame_10* cmd = (scsi_cmd_wsame_10*)request->cdb;
			memset(cmd, 0, sizeof(*cmd));
			cmd->opcode = SCSI_OP_WRITE_SAME_10;
			cmd->unmap = 1;
			cmd->lba = B_HOST_TO_BENDIAN_INT32(lba);
			cmd->length = B_HOST_TO_BENDIAN_INT16(trimLength);

			request->flags = SCSI_DIR_OUT;
			request->cdb_length = sizeof(*cmd);
			request->sort = lba;
			request->timeout = device->std_timeout;

			request->data = (uint8*)block;
			request->data_length = device->block_size;
			request->sg_list = NULL;

#ifdef DEBUG_TRIM
			dprintf("TRIM: SCSI: sending a WRITE SAME (10) command to"
				" the device (blocks):\n");
			dprintf("%" B_PRIu32 " : %" B_PRIu16 "\n",
				(uint32)B_BENDIAN_TO_HOST_INT32(cmd->lba),
				(uint16)B_BENDIAN_TO_HOST_INT16(cmd->length));
#endif

			status = periph_safe_exec(device, request);

			// peripheral layer only creates "read" error
			if (status == B_DEV_READ_ERROR)
				return B_DEV_WRITE_ERROR;
			else if (status != B_OK)
				return status;

			*trimmedBlocks += trimLength;
			length -= trimLength;
			lba += trimLength;
		}
	}

	return status;
}


status_t
periph_trim_device(scsi_periph_device_info* device, scsi_ccb* request,
	scsi_block_range* ranges, uint32 rangeCount, uint64* trimmedBlocks)
{
	*trimmedBlocks = 0;

	if (device->unmap_command == TRIM_NONE
		|| device->max_unmap_lba_count == 0
		|| device->max_unmap_descriptor_count == 0)
		return B_UNSUPPORTED;

	switch (device->unmap_command) {
		case TRIM_UNMAP:
			return trim_unmap(device, request, ranges, rangeCount,
				trimmedBlocks);
		case TRIM_WRITESAME16:
			return trim_writesame16(device, request, ranges, rangeCount,
				trimmedBlocks);
		case TRIM_WRITESAME10:
			return trim_writesame10(device, request, ranges, rangeCount,
				trimmedBlocks);
		default:
			return B_UNSUPPORTED;
	}
}
