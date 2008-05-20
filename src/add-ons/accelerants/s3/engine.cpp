/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.

	Other authors:
	Gerald Zajac 2007
*/

#include "accel.h"


static engine_token engineToken = { 1, B_2D_ACCELERATION, NULL };


uint32 
AccelerantEngineCount(void)
{
	return 1;
}


status_t 
AcquireEngine(uint32 capabilities, uint32 max_wait,
						sync_token* st, engine_token** et)
{
	(void)capabilities;	// avoid compiler warning for unused arg
	(void)max_wait;		// avoid compiler warning for unused arg

	// Acquire the shared benaphore.
	AQUIRE_BEN(gInfo.sharedInfo->engine.lock)
	// Sync if required.
	if (st)
		SyncToToken(st);

	// Return an engine token.
	*et = &engineToken;
	return B_OK;
}


status_t 
ReleaseEngine(engine_token* et, sync_token* st)
{
	// Update the sync token, if any.
	if (st)
		GetSyncToken(et, st);

	// Release the shared benaphore.
	RELEASE_BEN(gInfo.sharedInfo->engine.lock)
	return B_OK;
}


void 
WaitEngineIdle(void)
{
	gInfo.WaitIdleEmpty();	// wait until engine is completely idle
}


status_t 
GetSyncToken(engine_token* et, sync_token* st)
{
	// Engine count will always be zero: we don't support syncing to token (yet).
	st->engine_id = et->engine_id;
	st->counter = gInfo.sharedInfo->engine.count;
	return B_OK;
}


status_t 
SyncToToken(sync_token* st)
{
	(void)st;		// avoid compiler warning for unused arg

	WaitEngineIdle();
	return B_OK;
}

