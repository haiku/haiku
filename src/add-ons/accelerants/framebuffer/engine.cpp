/*
 * Copyright 2005, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include "accelerant_protos.h"
#include "vesa_info.h"


static engine_token sEngineToken = {1, 0 /*B_2D_ACCELERATION*/, NULL};


uint32
framebuffer_accelerant_engine_count(void)
{
	return 1;
}


status_t
framebuffer_acquire_engine(uint32 capabilities, uint32 max_wait, sync_token *syncToken,
	engine_token **_engineToken)
{
	*_engineToken = &sEngineToken;

	return B_OK;
}


status_t
framebuffer_release_engine(engine_token *engineToken, sync_token *syncToken)
{
	if (syncToken != NULL)
		syncToken->engine_id = engineToken->engine_id;

	return B_OK;
}


void
framebuffer_wait_engine_idle(void)
{
}


status_t
framebuffer_get_sync_token(engine_token *engineToken, sync_token *syncToken)
{
	return B_OK;
}


status_t
framebuffer_sync_to_token(sync_token *syncToken)
{
	return B_OK;
}

