/*
 * Copyright 2002-2003, Thomas Kurschel. All rights reserved.
 * Copyright 2024, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

/*
	Part of Open SCSI bus manager

	CCB manager

	As allocation of ccb can be on the paging path we must use a
	locked pool.
*/

#include "scsi_internal.h"

#include <slab/Slab.h>



static object_cache* sCcbPool = NULL;


scsi_ccb *
scsi_alloc_ccb(scsi_device_info *device)
{
	SHOW_FLOW0( 3, "" );

	scsi_ccb* ccb = (scsi_ccb*)object_cache_alloc(sCcbPool, 0);
	ccb->completion_cond.Init(ccb, "scsi ccb");

	ccb->bus = device->bus;
	ccb->path_id = device->bus->path_id;

	ccb->state = SCSI_STATE_FINISHED;
	ccb->device = device;
	ccb->target_id = device->target_id;
	ccb->target_lun = device->target_lun;

	// reset some very important fields
	// TODO: should we better omit that to find bugs easier?
	ccb->sg_list = NULL;
	ccb->io_operation = NULL;
	ccb->sort = -1;

	SHOW_FLOW(3, "path=%d", ccb->path_id);

	return ccb;
}


void
scsi_free_ccb(scsi_ccb *ccb)
{
	SHOW_FLOW0( 3, "" );

	if (ccb->state != SCSI_STATE_FINISHED)
		panic("Tried to free ccb that's still in use (state %d)\n", ccb->state);

	// Ensure no other thread still holds the condition variable's spinlock.
	ccb->completion_cond.NotifyAll(B_ERROR);

	object_cache_free(sCcbPool, ccb, 0);
}


status_t
init_ccb_alloc()
{
	sCcbPool = create_object_cache("scsi ccb", sizeof(scsi_ccb), 0, NULL, NULL, NULL);
	if (sCcbPool == NULL)
		return B_NO_MEMORY;

	// it must be at least 1 for normal use and 1 for stand-by autosense request
	object_cache_set_minimum_reserve(sCcbPool, 2);

	return B_OK;
}


void
uninit_ccb_alloc()
{
	delete_object_cache(sCcbPool);
}
