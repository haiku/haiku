/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "intel_extreme.h"


static engine_token sEngineToken = {1, 0 /*B_2D_ACCELERATION*/, NULL};


// public function: return number of hardware engine

uint32
intel_accelerant_engine_count(void) 
{
	return 1;
}


status_t
intel_acquire_engine(uint32 capabilities, uint32 maxWait, sync_token *syncToken,
	engine_token **_engineToken)
{
	*_engineToken = &sEngineToken;

	return B_OK;
}


status_t
intel_release_engine(engine_token *engineToken, sync_token *syncToken)
{
	if (syncToken != NULL)
		syncToken->engine_id = engineToken->engine_id;

	return B_OK;
}


void
intel_wait_engine_idle(void)
{
}


status_t
intel_get_sync_token(engine_token *engineToken, sync_token *syncToken)
{
	return B_OK;
}


status_t
intel_sync_to_token(sync_token *syncToken)
{
	return B_OK;
}

