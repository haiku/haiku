/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include "accelerant.h"
#include "accelerant_protos.h"
#include "commands.h"


#define TRACE_ENGINE
#ifdef TRACE_ENGINE
extern "C" void _sPrintf(const char *format, ...);
#	define TRACE(x) _sPrintf x
#else
#	define TRACE(x) ;
#endif


static engine_token sEngineToken = {1, 0 /*B_2D_ACCELERATION*/, NULL};


QueueCommands::QueueCommands(ring_buffer &ring)
	:
	fRingBuffer(ring)
{
	acquire_lock(&fRingBuffer.lock);
}


QueueCommands::~QueueCommands()
{
	if (fRingBuffer.position & 0x07) {
		// make sure the command is properly aligned
		_Write(COMMAND_NOOP);
	}

	write32(fRingBuffer.register_base + RING_BUFFER_TAIL, fRingBuffer.position);

	// We must make sure memory is written back in case the ring buffer
	// is in write combining mode - releasing the lock does this, as the
	// buffer is flushed on a locked memory operation (which is what this
	// benaphore does)
	release_lock(&fRingBuffer.lock);
}


void
QueueCommands::Put(struct command &command)
{
	uint32 count = command.Length();
	uint32 *data = command.Data();

	_MakeSpace(count);

	for (uint32 i = 0; i < count; i++) {
		_Write(data[i]);
	}
}


void
QueueCommands::PutFlush()
{
	_MakeSpace(2);

	_Write(COMMAND_FLUSH);
	_Write(COMMAND_NOOP);
}


void
QueueCommands::PutWaitFor(uint32 event)
{
	_MakeSpace(2);

	_Write(COMMAND_WAIT_FOR_EVENT | event);
	_Write(COMMAND_NOOP);
}


void
QueueCommands::PutOverlayFlip(uint32 mode, bool updateCoefficients)
{
	_MakeSpace(2);

	_Write(COMMAND_OVERLAY_FLIP | mode);
	_Write((uint32)gInfo->shared_info->physical_overlay_registers
		| (updateCoefficients ? OVERLAY_UPDATE_COEFFICIENTS : 0));
}


void
QueueCommands::_MakeSpace(uint32 size)
{
	// TODO: make sure there is enough room to write the command!
}


void
QueueCommands::_Write(uint32 data)
{
	uint32 *target = (uint32 *)(fRingBuffer.base + fRingBuffer.position);
	*target = data;

	fRingBuffer.position = (fRingBuffer.position + sizeof(uint32)) & (fRingBuffer.size - 1);
}


//	#pragma mark -


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


//	#pragma mark - engine management


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


//	#pragma mark - engine acceleration


void
intel_screen_to_screen_blit(engine_token *token, blit_params *params, uint32 count)
{
	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	for (uint32 i = 0; i < count; i++) {
		blit_command blit;
		blit.source_left = params[i].src_left;
		blit.source_top = params[i].src_top;
		blit.dest_left = params[i].dest_left;
		blit.dest_top = params[i].dest_top;
		blit.dest_right = params[i].dest_left + params[i].width;
		blit.dest_bottom = params[i].dest_top + params[i].height;

		queue.Put(blit);
	}
}

