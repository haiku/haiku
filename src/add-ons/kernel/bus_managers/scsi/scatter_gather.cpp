/*
 * Copyright 2004-2008, Haiku, Inc. All RightsReserved.
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */

/*!
	Creates temporary Scatter/Gather table if the peripheral
	driver has provided a simple pointer only.
*/

#include "scsi_internal.h"
#include "KernelExport_ext.h"

#include <string.h>
#include <iovec.h>

#include <algorithm>


static locked_pool_cookie temp_sg_pool;


static bool
fill_temp_sg(scsi_ccb *ccb)
{
	status_t res;
	scsi_bus_info *bus = ccb->bus;
	uint32 dma_boundary = bus->dma_params.dma_boundary;
	uint32 max_sg_block_size = bus->dma_params.max_sg_block_size;
	uint32 max_sg_blocks = std::min(bus->dma_params.max_sg_blocks,
		(uint32)MAX_TEMP_SG_FRAGMENTS);
	iovec vec = {
		ccb->data,
		ccb->data_length
	};
	size_t num_entries;
	size_t mapped_len;
	uint32 cur_idx;
	physical_entry *temp_sg = (physical_entry *)ccb->sg_list;

	res = get_iovec_memory_map(&vec, 1, 0, ccb->data_length, temp_sg, max_sg_blocks,
				&num_entries, &mapped_len);

	if (res != B_OK) {
		SHOW_ERROR(2, "cannot create temporary S/G list for IO request (%s)", strerror(res));
		return false;
	}

	if (mapped_len != ccb->data_length)
		goto too_complex;

	if (dma_boundary != ~(uint32)0 || ccb->data_length > max_sg_block_size) {
		// S/G list may not be controller-compatible:
		// we have to split offending entries
		SHOW_FLOW(3, "Checking violation of dma boundary 0x%x and entry size 0x%x",
			(int)dma_boundary, (int)max_sg_block_size);

		for (cur_idx = 0; cur_idx < num_entries; ++cur_idx) {
			addr_t max_len;

			// calculate space upto next dma boundary crossing
			max_len = (dma_boundary + 1) -
				(temp_sg[cur_idx].address & dma_boundary);
			// restrict size per sg item
			max_len = std::min(max_len, (addr_t)max_sg_block_size);

			SHOW_FLOW(4, "addr=%#" B_PRIxPHYSADDR ", size=%x, max_len=%x, "
				"idx=%d, num=%d", temp_sg[cur_idx].address,
				(int)temp_sg[cur_idx].size, (int)max_len, (int)cur_idx,
				(int)num_entries);

			if (max_len < temp_sg[cur_idx].size) {
				// split sg block
				if (++num_entries > max_sg_blocks)
					goto too_complex;

				memmove(&temp_sg[cur_idx + 1], &temp_sg[cur_idx],
					(num_entries - 1 - cur_idx) * sizeof(physical_entry));

				temp_sg[cur_idx].size = max_len;
				temp_sg[cur_idx + 1].address
					= temp_sg[cur_idx + 1].address + max_len;
				temp_sg[cur_idx + 1].size -= max_len;
			}
		}
	}

	ccb->sg_count = num_entries;

	return true;

too_complex:
	SHOW_ERROR( 2, "S/G list to complex for IO request (max %d entries)",
		MAX_TEMP_SG_FRAGMENTS );

	return false;
}


/** create temporary SG for request */

bool
create_temp_sg(scsi_ccb *ccb)
{
	physical_entry *temp_sg;
	status_t res;

	SHOW_FLOW(3, "ccb=%p, data=%p, data_length=%" B_PRIu32, ccb, ccb->data,
		ccb->data_length);

	ccb->sg_list = temp_sg = (physical_entry*)locked_pool->alloc(temp_sg_pool);
	if (temp_sg == NULL) {
		SHOW_ERROR0(2, "cannot allocate memory for IO request!");
		return false;
	}

	res = lock_memory(ccb->data, ccb->data_length, B_DMA_IO
		| ((ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_IN ? B_READ_DEVICE : 0));

	if (res != B_OK) {
		SHOW_ERROR(2, "cannot lock memory for IO request (%s)", strerror(res));
		goto err;
	}

	if (fill_temp_sg(ccb))
		// this is the success path
		return true;

	unlock_memory(ccb->data, ccb->data_length, B_DMA_IO
		| ((ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_IN ? B_READ_DEVICE : 0));

err:
	locked_pool->free(temp_sg_pool, temp_sg);
	return false;
}


/** cleanup temporary SG list */

void
uninit_temp_sg(void)
{
	locked_pool->destroy(temp_sg_pool);
}


/** destroy SG list buffer */

void
cleanup_tmp_sg(scsi_ccb *ccb)
{
	status_t res;

	SHOW_FLOW(3, "ccb=%p, data=%p, data_length=%d",
		ccb, ccb->data, (int)ccb->data_length);

	res = unlock_memory(ccb->data, ccb->data_length, B_DMA_IO
		|  ((ccb->flags & SCSI_DIR_MASK) == SCSI_DIR_IN ? B_READ_DEVICE : 0));

	if (res != B_OK) {
		SHOW_FLOW0(3, "Cannot unlock previously locked memory!");
		panic("Cannot unlock previously locked memory!");
	}

	locked_pool->free(temp_sg_pool, (physical_entry *)ccb->sg_list);

	// restore previous state
	ccb->sg_list = NULL;
}


/** create SG list buffer */

int
init_temp_sg(void)
{
	temp_sg_pool = locked_pool->create(
		MAX_TEMP_SG_FRAGMENTS * sizeof(physical_entry),
		sizeof(physical_entry) - 1, 0,
		B_PAGE_SIZE, MAX_TEMP_SG_LISTS, 1,
		"scsi_temp_sg_pool", B_CONTIGUOUS, NULL, NULL, NULL);

	if (temp_sg_pool == NULL)
		return B_NO_MEMORY;

	return B_OK;
}

