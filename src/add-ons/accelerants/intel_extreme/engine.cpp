/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant_protos.h"
#include "intel_extreme.h"


//#define TRACE_ENGINE
#ifdef TRACE_ENGINE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


static engine_token sEngineToken = {1, 0 /*B_2D_ACCELERATION*/, NULL};


/** Return number of hardware engines */

uint32
intel_accelerant_engine_count(void) 
{
	TRACE(("intel_accelerant_engine_count()\n"));
	return 1;
}


status_t
intel_acquire_engine(uint32 capabilities, uint32 maxWait, sync_token *syncToken,
	engine_token **_engineToken)
{
	TRACE(("intel_acquire_engine()\n"));
	*_engineToken = &sEngineToken;

	return B_OK;
}


status_t
intel_release_engine(engine_token *engineToken, sync_token *syncToken)
{
	TRACE(("intel_release_engine()\n"));
	if (syncToken != NULL)
		syncToken->engine_id = engineToken->engine_id;

	return B_OK;
}


void
intel_wait_engine_idle(void)
{
	TRACE(("intel_wait_engine_idle()\n"));
}


status_t
intel_get_sync_token(engine_token *engineToken, sync_token *syncToken)
{
	TRACE(("intel_get_sync_token()\n"));
	return B_OK;
}


status_t
intel_sync_to_token(sync_token *syncToken)
{
	TRACE(("intel_sync_to_token()\n"));
	return B_OK;
}

