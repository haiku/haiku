/*
 * Copyright 2006-2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Debug.h>

#include "accelerant.h"
#include "accelerant_protos.h"
#include "commands.h"


#undef TRACE

//#define TRACE_ENGINE
#ifdef TRACE_ENGINE
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
		Write(COMMAND_NOOP);
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

	MakeSpace(count);

	for (uint32 i = 0; i < count; i++) {
		Write(data[i]);
	}
}


void
QueueCommands::PutFlush()
{
	MakeSpace(2);

	Write(COMMAND_FLUSH);
	Write(COMMAND_NOOP);
}


void
QueueCommands::PutWaitFor(uint32 event)
{
	MakeSpace(2);

	Write(COMMAND_WAIT_FOR_EVENT | event);
	Write(COMMAND_NOOP);
}


void
QueueCommands::PutOverlayFlip(uint32 mode, bool updateCoefficients)
{
	MakeSpace(2);

	Write(COMMAND_OVERLAY_FLIP | mode);

	uint32 registers;
	// G33 does not need a physical address for the overlay registers
	if (intel_uses_physical_overlay(*gInfo->shared_info))
		registers = gInfo->shared_info->physical_overlay_registers;
	else
		registers = gInfo->shared_info->overlay_offset;

	Write(registers | (updateCoefficients ? OVERLAY_UPDATE_COEFFICIENTS : 0));
}


void
QueueCommands::MakeSpace(uint32 size)
{
	ASSERT((size & 1) == 0);

	size *= sizeof(uint32);
	bigtime_t start = system_time();

	while (fRingBuffer.space_left < size) {
		// wait until more space is free
		uint32 head = read32(fRingBuffer.register_base + RING_BUFFER_HEAD)
			& INTEL_RING_BUFFER_HEAD_MASK;

		if (head <= fRingBuffer.position)
			head += fRingBuffer.size;

		fRingBuffer.space_left = head - fRingBuffer.position;

		if (fRingBuffer.space_left < size) {
			if (system_time() > start + 1000000LL) {
				TRACE(("intel_extreme: engine stalled, head %lx\n", head));
				break;
			}
			spin(10);
		}
	}

	fRingBuffer.space_left -= size;
}


void
QueueCommands::Write(uint32 data)
{
	uint32 *target = (uint32 *)(fRingBuffer.base + fRingBuffer.position);
	*target = data;

	fRingBuffer.position = (fRingBuffer.position + sizeof(uint32))
		& (fRingBuffer.size - 1);
}


//	#pragma mark -


void
uninit_ring_buffer(ring_buffer &ringBuffer)
{
	uninit_lock(&ringBuffer.lock);
	write32(ringBuffer.register_base + RING_BUFFER_CONTROL, 0);
}


void
setup_ring_buffer(ring_buffer &ringBuffer, const char* name)
{
	TRACE(("Setup ring buffer %s, offset %lx, size %lx\n", name,
		ringBuffer.offset, ringBuffer.size));

	if (init_lock(&ringBuffer.lock, name) < B_OK) {
		// disable ring buffer
		ringBuffer.size = 0;
		return;
	}

	uint32 ring = ringBuffer.register_base;
	ringBuffer.position = 0;
	ringBuffer.space_left = ringBuffer.size;

	write32(ring + RING_BUFFER_TAIL, 0);
	write32(ring + RING_BUFFER_START, ringBuffer.offset);
	write32(ring + RING_BUFFER_CONTROL,
		((ringBuffer.size - B_PAGE_SIZE) & INTEL_RING_BUFFER_SIZE_MASK)
		| INTEL_RING_BUFFER_ENABLED);
}


//	#pragma mark - engine management


/*! Return number of hardware engines */
uint32
intel_accelerant_engine_count(void) 
{
	TRACE(("intel_accelerant_engine_count()\n"));
	return 1;
}


status_t
intel_acquire_engine(uint32 capabilities, uint32 maxWait, sync_token* syncToken,
	engine_token** _engineToken)
{
	TRACE(("intel_acquire_engine()\n"));
	*_engineToken = &sEngineToken;

	if (acquire_lock(&gInfo->shared_info->engine_lock) != B_OK)
		return B_ERROR;

	if (syncToken)
		intel_sync_to_token(syncToken);

	return B_OK;
}


status_t
intel_release_engine(engine_token* engineToken, sync_token* syncToken)
{
	TRACE(("intel_release_engine()\n"));
	if (syncToken != NULL)
		syncToken->engine_id = engineToken->engine_id;

	release_lock(&gInfo->shared_info->engine_lock);
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

	bigtime_t start = system_time();

	ring_buffer &ring = gInfo->shared_info->primary_ring_buffer;
	uint32 head, tail;
	while (true) {
		head = read32(ring.register_base + RING_BUFFER_HEAD)
			& INTEL_RING_BUFFER_HEAD_MASK;
		tail = read32(ring.register_base + RING_BUFFER_TAIL)
			& INTEL_RING_BUFFER_HEAD_MASK;

		if (head == tail)
			break;

		if (system_time() > start + 1000000LL) {
			// the engine seems to be locked up!
			TRACE(("intel_extreme: engine locked up, head %lx!\n", head));
			break;
		}

		spin(10);
	}
}


status_t
intel_get_sync_token(engine_token* engineToken, sync_token* syncToken)
{
	TRACE(("intel_get_sync_token()\n"));
	return B_OK;
}


status_t
intel_sync_to_token(sync_token* syncToken)
{
	TRACE(("intel_sync_to_token()\n"));
	intel_wait_engine_idle();
	return B_OK;
}


//	#pragma mark - engine acceleration


void
intel_screen_to_screen_blit(engine_token* token, blit_params* params,
	uint32 count)
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
intel_fill_rectangle(engine_token* token, uint32 color,
	fill_rect_params* params, uint32 count)
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
intel_invert_rectangle(engine_token* token, fill_rect_params* params,
	uint32 count)
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
intel_fill_span(engine_token* token, uint32 color, uint16* _params,
	uint32 count)
{
	struct params {
		uint16	top;
		uint16	left;
		uint16	right;
	} *params = (struct params*)_params;

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
