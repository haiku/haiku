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


//#define TRACE_ENGINE
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

	// We must make sure memory is written back in case the ring buffer
	// is in write combining mode - releasing the lock does this, as the
	// buffer is flushed on a locked memory operation (which is what this
	// benaphore does), but it must happen before writing the new tail...
	int32 flush;
	atomic_add(&flush, 1);

	write32(fRingBuffer.register_base + RING_BUFFER_TAIL, fRingBuffer.position);

	release_lock(&fRingBuffer.lock);
}


void
QueueCommands::Put(struct command &command, size_t size)
{
	uint32 count = size / sizeof(uint32);
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

	{
		QueueCommands queue(gInfo->shared_info->primary_ring_buffer);
		queue.PutFlush();
	}

	// TODO: this should only be a temporary solution!
	// a better way to do this would be to acquire the engine's lock and
	// sync to the latest token

	ring_buffer &ring = gInfo->shared_info->primary_ring_buffer;
	uint32 head, tail;
	do {
		head = read32(ring.register_base + RING_BUFFER_HEAD) & INTEL_RING_BUFFER_HEAD_MASK;
		tail = read32(ring.register_base + RING_BUFFER_TAIL) & INTEL_RING_BUFFER_HEAD_MASK;

		//snooze(100);
		// Isn't a snooze() a bit too slow? At least it's called *very* often in Haiku...
	} while (head != tail);
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
	intel_wait_engine_idle();
	return B_OK;
}


//	#pragma mark - engine acceleration


void
intel_screen_to_screen_blit(engine_token *token, blit_params *params, uint32 count)
{
	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	for (uint32 i = 0; i < count; i++) {
		xy_source_blit_command blit;
		blit.source_left = params[i].src_left;
		blit.source_top = params[i].src_top;
		blit.dest_left = params[i].dest_left;
		blit.dest_top = params[i].dest_top;
		blit.dest_right = params[i].dest_left + params[i].width + 1;
		blit.dest_bottom = params[i].dest_top + params[i].height + 1;

		queue.Put(blit, sizeof(blit));
	}
}


void
intel_fill_rectangle(engine_token *token, uint32 color, fill_rect_params *params,
	uint32 count)
{
	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	for (uint32 i = 0; i < count; i++) {
		xy_color_blit_command blit(false);
		blit.dest_left = params[i].left;
		blit.dest_top = params[i].top;
		blit.dest_right = params[i].right + 1;
		blit.dest_bottom = params[i].bottom + 1;
		blit.color = color;

		queue.Put(blit, sizeof(blit));
	}
}


void
intel_invert_rectangle(engine_token *token, fill_rect_params *params, uint32 count)
{
	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	for (uint32 i = 0; i < count; i++) {
		xy_color_blit_command blit(true);
		blit.dest_left = params[i].left;
		blit.dest_top = params[i].top;
		blit.dest_right = params[i].right + 1;
		blit.dest_bottom = params[i].bottom + 1;
		blit.color = 0xffffffff;

		queue.Put(blit, sizeof(blit));
	}
}


void
intel_fill_span(engine_token *token, uint32 color, uint16* _params, uint32 count)
{
	struct params {
		uint16	top;
		uint16	left;
		uint16	right;
	} *params = (struct params *)_params;

	QueueCommands queue(gInfo->shared_info->primary_ring_buffer);

	xy_setup_mono_pattern_command setup;
	setup.background_color = color;
	setup.pattern = 0;
	queue.Put(setup, sizeof(setup));

	for (uint32 i = 0; i < count; i++) {
		xy_scanline_blit_command blit;
		blit.dest_left = params[i].left;
		blit.dest_top = params[i].top;
		blit.dest_right = params[i].right;
		blit.dest_bottom = params[i].top;
	}
}
