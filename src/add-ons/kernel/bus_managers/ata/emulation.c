/*
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

//!	General SCSI emulation routines


#include "ide_internal.h"
#include "ide_sim.h"

#include <vm.h>

#include <string.h>


/*! Emulate REQUEST SENSE */
void
ide_request_sense(ide_device_info *device, ata_request *request)
{
	scsi_ccb *ccb = request->ccb;
	scsi_cmd_request_sense *cmd = (scsi_cmd_request_sense *)ccb->cdb;
	scsi_sense sense;
	uint32 transferSize;

	// cannot use finish_checksense here, as data is not copied into autosense buffer
	// but into normal data buffer, SCSI result is GOOD and CAM status is REQ_CMP

	if (device->combined_sense)
		create_sense(device, &sense);
	else
		memset(&sense, 0, sizeof(sense));

	copy_sg_data(ccb, 0, cmd->allocation_length, &sense, sizeof(sense), false);

	// reset sense information on read
	device->combined_sense = 0;

	transferSize = min(sizeof(sense), cmd->allocation_length);
	transferSize = min(transferSize, ccb->data_length);

	ccb->data_resid = ccb->data_length - transferSize;

	// normally, all flags are set to "success", but for Request Sense 
	// this would have overwritten the sense we want to read
	device->subsys_status = SCSI_REQ_CMP;
	ccb->device_status = SCSI_STATUS_GOOD;
}


/*!	Copy data between ccb data and buffer
	ccb			- ccb to copy data from/to
	offset			- offset of data in ccb
	allocation_length- limit of ccb's data buffer according to CDB
	buffer			- data to copy data from/to
	size			- number of bytes to copy
	to_buffer		- true: copy from ccb to buffer
					  false: copy from buffer to ccb
	return: true, if data of ccb was large enough
*/
bool
copy_sg_data(scsi_ccb *ccb, uint offset, uint allocationLength,
	void *buffer, int size, bool toBuffer)
{
	const physical_entry *sgList = ccb->sg_list;
	int sgCount = ccb->sg_count;
	int requestSize;

	SHOW_FLOW(3, "offset=%u, req_size_limit=%d, size=%d, sg_list=%p, sg_cnt=%d, %s buffer", 
		offset, allocationLength, size, sgList, sgCount, toBuffer ? "to" : "from");

	// skip unused S/G entries
	while (sgCount > 0 && offset >= sgList->size) {
		offset -= sgList->size;
		++sgList;
		--sgCount;
	}

	if (sgCount == 0)
		return 0;

	// remaining bytes we are allowed to copy from/to ccb 		
	requestSize = min(allocationLength, ccb->data_length) - offset;

	// copy one S/G entry at a time
	for (; size > 0 && requestSize > 0 && sgCount > 0; ++sgList, --sgCount) {
		addr_t virtualAddress;
		size_t bytes;

		bytes = min(size, requestSize);
		bytes = min(bytes, sgList->size);

		if (vm_get_physical_page((addr_t)sgList->address, &virtualAddress,
				PHYSICAL_PAGE_CAN_WAIT) != B_OK) 
			return false;

		SHOW_FLOW(4, "buffer=%p, virt_addr=%p, bytes=%d, to_buffer=%d",
			buffer, (void *)(virtualAddress + offset), (int)bytes, toBuffer);

		if (toBuffer)
			memcpy(buffer, (void *)(virtualAddress + offset), bytes);
		else
			memcpy((void *)(virtualAddress + offset), buffer, bytes);

		vm_put_physical_page(virtualAddress);

		buffer = (char *)buffer + bytes;
		size -= bytes;
		offset = 0;
	}

	return size == 0;
}
