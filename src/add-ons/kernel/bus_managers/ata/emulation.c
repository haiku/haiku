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
		size_t bytes;

		bytes = min(size, requestSize);
		bytes = min(bytes, sgList->size);

		SHOW_FLOW(4, "buffer=%p, virt_addr=%p, bytes=%d, to_buffer=%d",
			buffer, (void *)((addr_t)sgList->address + offset), (int)bytes,
			toBuffer);

		if (toBuffer) {
			vm_memcpy_from_physical(buffer, (addr_t)sgList->address + offset,
				bytes, false);
		} else {
			vm_memcpy_to_physical((addr_t)sgList->address + offset, buffer,
				bytes, false);
		}

		buffer = (char *)buffer + bytes;
		size -= bytes;
		offset = 0;
	}

	return size == 0;
}
