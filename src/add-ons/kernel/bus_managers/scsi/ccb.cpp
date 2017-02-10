/*
** Copyright 2002/03, Thomas Kurschel. All rights reserved.
** Distributed under the terms of the MIT License.
*/

/*
	Part of Open SCSI bus manager

	CCB manager

	As allocation of ccb can be on the paging path we must use a
	locked pool.
*/

#include "scsi_internal.h"



// ccb are relatively large, so don't make it too small to not waste memory
#define CCB_CHUNK_SIZE 16*1024

// maximum number of CCBs - probably, we want to make that editable
// it must be at least 1 for normal use and 1 for stand-by autosense request
#define CCB_NUM_MAX 128


scsi_ccb *
scsi_alloc_ccb(scsi_device_info *device)
{
	scsi_ccb *ccb;

	SHOW_FLOW0( 3, "" );

	ccb = (scsi_ccb *)locked_pool->alloc(device->bus->ccb_pool);
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

	ccb->state = SCSI_STATE_FREE;

	locked_pool->free(ccb->bus->ccb_pool, ccb);
}


static status_t
ccb_low_alloc_hook(void *block, void *arg)
{
	scsi_ccb *ccb = (scsi_ccb *)block;
	scsi_bus_info *bus = (scsi_bus_info *)arg;
	status_t res;

	ccb->bus = bus;
	ccb->path_id = bus->path_id;
	ccb->state = SCSI_STATE_FREE;

	if ((res = ccb->completion_sem = create_sem(0, "ccb_sem")) < 0)
		return res;

	return B_OK;
}


static void
ccb_low_free_hook(void *block, void *arg)
{
	scsi_ccb *ccb = (scsi_ccb *)block;

	delete_sem(ccb->completion_sem);
}


status_t
scsi_init_ccb_alloc(scsi_bus_info *bus)
{
	// initially, we want no CCB allocated as the path_id of
	// the bus is not ready yet so the CCB cannot be initialized
	// correctly
	bus->ccb_pool = locked_pool->create(sizeof(scsi_ccb), sizeof(uint32) - 1, 0,
		CCB_CHUNK_SIZE, CCB_NUM_MAX, 0, "scsi_ccb_pool", B_CONTIGUOUS,
		ccb_low_alloc_hook, ccb_low_free_hook, bus);

	if (bus->ccb_pool == NULL)
		return B_NO_MEMORY;

	return B_OK;
}


void
scsi_uninit_ccb_alloc(scsi_bus_info *bus)
{
	locked_pool->destroy(bus->ccb_pool);
}

