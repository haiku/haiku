/*
 * Copyright 1999, Be Incorporated.
 * Copyright 2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Be Incorporated
 *		Eric Petit <eric.petit@lapsus.org>
 */

#include "GlobalData.h"

static engine_token vmwareEngineToken = {1, B_2D_ACCELERATION, NULL};

uint32
ACCELERANT_ENGINE_COUNT()
{
	return 1;
}


status_t
ACQUIRE_ENGINE(uint32 capabilities, uint32 max_wait, sync_token * st,
	engine_token ** et)
{
	ACQUIRE_BEN(gSi->engineLock);
	if (st)
		SYNC_TO_TOKEN(st);
	*et = &vmwareEngineToken;
	return B_OK;
}
 

status_t
RELEASE_ENGINE(engine_token * et, sync_token * st)
{
	if (st)
		GET_SYNC_TOKEN(et, st);
	RELEASE_BEN(gSi->engineLock);
	return B_OK;
}


void
WAIT_ENGINE_IDLE()
{
	FifoSync();
}


status_t
GET_SYNC_TOKEN(engine_token * et, sync_token * st)
{
	st->engine_id = et->engine_id;
	st->counter = 0;
	return B_OK;
}


status_t
SYNC_TO_TOKEN(sync_token * st)
{
	WAIT_ENGINE_IDLE();
	return B_OK;
}

