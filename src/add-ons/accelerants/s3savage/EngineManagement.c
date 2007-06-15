/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2006-2007
*/

#include "GlobalData.h"
#include "AccelerantPrototypes.h"


static engine_token savage_engine_token = { 1, B_2D_ACCELERATION, NULL };


uint32 
ACCELERANT_ENGINE_COUNT(void)
{
	return 1;
}


status_t 
ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait,
						sync_token *st, engine_token **et)
{
	(void)capabilities;	// avoid compiler warning for unused arg
	(void)max_wait;		// avoid compiler warning for unused arg

	/* acquire the shared benaphore */
	AQUIRE_BEN(si->engine.lock)
	/* sync if required */
	if (st)
		SYNC_TO_TOKEN(st);

	/* return an engine token */
	*et = &savage_engine_token;
	return B_OK;
}


status_t 
RELEASE_ENGINE(engine_token *et, sync_token *st)
{
	/* update the sync token, if any */
	if (st)
		GET_SYNC_TOKEN(et, st);

	/* release the shared benaphore */
	RELEASE_BEN(si->engine.lock)
	return B_OK;
}


void 
WAIT_ENGINE_IDLE(void)
{
	si->WaitIdleEmpty();	// wait until engine is completely idle
}


status_t 
GET_SYNC_TOKEN(engine_token *et, sync_token *st)
{
	/* engine count will always be zero: we don't support syncing to token (yet) */
	st->engine_id = et->engine_id;
	st->counter = si->engine.count;
	return B_OK;
}


status_t 
SYNC_TO_TOKEN(sync_token *st)
{
	(void)st;		// avoid compiler warning for unused arg

	WAIT_ENGINE_IDLE();
	return B_OK;
}

