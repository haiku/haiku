/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/

/*
	Part of Open IDE bus manager

	General SCSI emulation routines
*/


#include "ide_internal.h"
#include "ide_sim.h"
#include "KernelExport_ext.h"

#include <string.h>


/** emulate REQUEST SENSE */

void
ide_request_sense(ide_device_info *device, ide_qrequest *qrequest)
{
	scsi_ccb *request = qrequest->request;
	scsi_cmd_request_sense *cmd = (scsi_cmd_request_sense *)request->cdb;
	scsi_sense sense;
	uint32 transfer_size;

	// cannot use finish_checksense here, as data is not copied into autosense buffer
	// but into normal data buffer, SCSI result is GOOD and CAM status is REQ_CMP

	if (device->combined_sense)
		create_sense(device, &sense);
	else
		memset(&sense, 0, sizeof(sense));

	copy_sg_data(request, 0, cmd->alloc_length, &sense, sizeof(sense), false);

	// reset sense information on read
	device->combined_sense = 0;

	transfer_size = min(sizeof(sense), cmd->alloc_length);
	transfer_size = min(transfer_size, request->data_len);

	request->data_resid = request->data_len - transfer_size;

	// normally, all flags are set to "success", but for Request Sense 
	// this would have overwritten the sense we want to read
	device->subsys_status = SCSI_REQ_CMP;
	request->device_status = SCSI_STATUS_GOOD;
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

bool
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
		void *virt_addr;

		bytes = min(size, req_size);
		bytes = min(bytes, sg_list->size);

		if (map_mainmemory((addr_t)sg_list->address, &virt_addr) != B_OK) 
			return false;

		SHOW_FLOW(4, "buffer=%p, virt_addr=%p, bytes=%d, to_buffer=%d",
			buffer, (void *)((addr_t)virt_addr + offset), (int)bytes, to_buffer);

		if (to_buffer)
			memcpy(buffer, (void *)((addr_t)virt_addr + offset), bytes);
		else
			memcpy((void *)((addr_t)virt_addr + offset), buffer, bytes);

		unmap_mainmemory(virt_addr);

		(char *)buffer += bytes;
		size -= bytes;
		offset = 0;
	}

	return size == 0;
}
