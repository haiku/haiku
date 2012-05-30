/*
 * Copyright 2007-2012 Haiku, Inc.  All rights reserved.
 * Distributed under the terms of the MIT license.
 *
 * Authors:
 *		Gerald Zajac
 */

#include "accelerant.h"
#include "i810_regs.h"


static engine_token sEngineToken = { 1, B_2D_ACCELERATION, NULL };


uint32
AccelerantEngineCount(void)
{
	return 1;
}


status_t
AcquireEngine(uint32 capabilities, uint32 maxWait,
			sync_token* syncToken, engine_token** engineToken)
{
	(void)capabilities;	// avoid compiler warning for unused arg
	(void)maxWait;		// avoid compiler warning for unused arg

	if (gInfo.sharedInfo->engineLock.Acquire() != B_OK)
		return B_ERROR;

	if (syncToken)
		SyncToToken(syncToken);

	*engineToken = &sEngineToken;
	return B_OK;
}


status_t
ReleaseEngine(engine_token* engineToken, sync_token* syncToken)
{
	if (syncToken)
		GetSyncToken(engineToken, syncToken);

	gInfo.sharedInfo->engineLock.Release();
	return B_OK;
}


void
WaitEngineIdle(void)
{
	// Wait until engine is idle.

	int k = 10000000;
	
	while ((INREG16(INST_DONE) & 0x7B) != 0x7B && k > 0)
		k--; 
}


status_t
GetSyncToken(engine_token* engineToken, sync_token* syncToken)
{
	syncToken->engine_id = engineToken->engine_id;
	syncToken->counter = 0;
	return B_OK;
}


status_t
SyncToToken(sync_token* syncToken)
{
	(void)syncToken;		// avoid compiler warning for unused arg

	WaitEngineIdle();
	return B_OK;
}

