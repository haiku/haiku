/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	other authors:
	Mark Watson
	Rudolf Cornelissen 3/2004-6/2008
*/

#define MODULE_BIT 0x10000000

#include "acc_std.h"


static engine_token nv_engine_token = { 1, B_2D_ACCELERATION, NULL };

uint32 ACCELERANT_ENGINE_COUNT(void)
{
	/* we have one acceleration engine */
	return 1;
}

status_t ACQUIRE_ENGINE_DMA(uint32 capabilities, uint32 max_wait, sync_token *st, engine_token **et)
{
	/* acquire the shared benaphore */
	AQUIRE_BEN(si->engine.lock)
	/* sync if required */
	if (st) SYNC_TO_TOKEN(st);

	/* make sure all needed engine cmd's are mapped to the FIFO */
	nv_acc_assert_fifo_dma();

	/* return an engine token */
	*et = &nv_engine_token;
	return B_OK;
}

status_t RELEASE_ENGINE(engine_token *et, sync_token *st)
{
	/* update the sync token, if any */
	if (st) GET_SYNC_TOKEN(et,st);

	/* release the shared benaphore */
	RELEASE_BEN(si->engine.lock)
	return B_OK;
}

void WAIT_ENGINE_IDLE(void)
{
	/*wait for the engine to be totally idle*/
	nv_acc_wait_idle_dma();
}

status_t GET_SYNC_TOKEN(engine_token *et, sync_token *st)
{
	/* engine count will always be zero: we don't support syncing to token (yet) */
	st->engine_id = et->engine_id;
	st->counter = si->engine.count;
	return B_OK;
}

status_t SYNC_TO_TOKEN(sync_token *st)
{
	/* wait until the engine is totally idle: we don't support syncing to token (yet) */
	/* note:
	 * AFAIK in order to be able to setup sync_to_token, we'd need a circular fifo
	 * buffer in (main) memory instead of directly programming the GPU fifo so we
	 * can tell (via a hardware maintained pointer into this circular fifo) where
	 * the acc engine is with executing commands! */
	WAIT_ENGINE_IDLE();

	return B_OK;
}
