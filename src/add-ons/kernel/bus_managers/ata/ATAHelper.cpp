/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Copyright 2004-2007, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2002/03, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

#include "ATAPrivate.h"

#include <vm/vm.h>
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

	// skip unused S/G entries
	while (sgCount > 0 && offset >= sgList->size) {
		offset -= sgList->size;
		++sgList;
		--sgCount;
	}

	if (sgCount == 0)
		return false;

	// remaining bytes we are allowed to copy from/to ccb
	int requestSize = MIN(allocationLength, ccb->data_length) - offset;

	// copy one S/G entry at a time
	for (; size > 0 && requestSize > 0 && sgCount > 0; ++sgList, --sgCount) {
		size_t bytes;

		bytes = MIN(size, requestSize);
		bytes = MIN(bytes, sgList->size);

		if (toBuffer) {
			vm_memcpy_from_physical(buffer, sgList->address + offset, bytes,
				false);
		} else {
			vm_memcpy_to_physical(sgList->address + offset, buffer, bytes,
				false);
		}

		buffer = (char *)buffer + bytes;
		size -= bytes;
		offset = 0;
	}

	return size == 0;
}


void
swap_words(void *data, size_t size)
{
	uint16 *word = (uint16 *)data;
	size_t count = size / 2;
	while (count--) {
		*word = B_BENDIAN_TO_HOST_INT16(*word);
		word++;
	}
}
