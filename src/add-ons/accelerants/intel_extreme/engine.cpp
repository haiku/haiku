/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant.h"
#include "accelerant_protos.h"


#define TRACE_ENGINE
#ifdef TRACE_ENGINE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


static engine_token sEngineToken = {1, 0 /*B_2D_ACCELERATION*/, NULL};


/**	The ring buffer must be locked when calling this function */

void
write_to_ring(ring_buffer &ring, uint32 value)
{
	uint32 *target = (uint32 *)(ring.base + ring.position);
	*target = value;

	ring.position = (ring.position + sizeof(uint32)) & (ring.size - 1);
}


/**	The ring buffer must be locked when calling this function */

void
ring_command_complete(ring_buffer &ring)
{
	if (ring.position & 0x07) {
		// make sure the command is properly aligned
		write_to_ring(ring, COMMAND_NOOP);
	}

	write32(ring.register_base + RING_BUFFER_TAIL, ring.position);

	// make sure memory is written back in case the ring buffer
	// is in write combining mode
	int32 test;
	atomic_add(&test, 1);
}


void
setup_ring_buffer(ring_buffer &ringBuffer, const char *name)
{
	TRACE(("Setup ring buffer %s, offset %lx, size %lx\n", name, ringBuffer.offset,
		ringBuffer.size));

	if (init_lock(&ringBuffer.lock, name) < B_OK) {
		// disable ring buffer
		ringBuffer.size = 0;
		return;
	}

	uint32 ring = ringBuffer.register_base;
	write32(ring + RING_BUFFER_TAIL, 0);
	write32(ring + RING_BUFFER_START, ringBuffer.offset);
	write32(ring + RING_BUFFER_CONTROL,
		((ringBuffer.size - B_PAGE_SIZE) & INTEL_RING_BUFFER_SIZE_MASK)
		| INTEL_RING_BUFFER_ENABLED);
}


//	#pragma mark -


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

