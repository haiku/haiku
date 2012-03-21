/*	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 *	Distributed under the terms of the Be Sample Code license.
 *
 *	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include "VideoConsumer.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <Buffer.h>
#include <BufferGroup.h>
#include <NodeInfo.h>
#include <Bitmap.h>
#include <View.h>
#include <scheduler.h>
#include <TimeSource.h>
#include <MediaRoster.h>

#include "ColorSpaceToString.h"
#include "NodeManager.h"
#include "VideoTarget.h"


// debugging
//#define TRACE_VIDEO_CONSUMER
#ifdef TRACE_VIDEO_CONSUMER
# define TRACE(x...)		printf(x)
# define PROGRESS(x...)		printf(x)
# define FUNCTION(x...)		printf(x)
# define LOOP(x...)			printf(x)
# define ERROR(x...)		fprintf(stderr, x)
#else
# define TRACE(x...)
# define PROGRESS(x...)
# define FUNCTION(x...)
# define LOOP(x...)
# define ERROR(x...)		fprintf(stderr, x)
#endif

#define M1 ((double)1000000.0)
static const bigtime_t kMaxBufferLateness = 20000LL;


VideoConsumer::VideoConsumer(const char* name, BMediaAddOn* addon,
		const uint32 internal_id, NodeManager* manager,
		VideoTarget* target)
	: BMediaNode(name),
	  BMediaEventLooper(),
	  BBufferConsumer(B_MEDIA_RAW_VIDEO),
	  fInternalID(internal_id),
	  fAddOn(addon),
	  fConnectionActive(false),
	  fMyLatency(3000),
	  fOurBuffers(false),
	  fBuffers(NULL),
	  fManager(manager),
	  fTargetLock(),
	  fTarget(target),
	  fLastBufferIndex(-1),
	  fTryOverlay(true)
{
	FUNCTION("VideoConsumer::VideoConsumer\n");

	AddNodeKind(B_PHYSICAL_OUTPUT);
	SetEventLatency(0);
	
	for (uint32 i = 0; i < kBufferCount; i++) {
		fBitmap[i] = NULL;
		fBufferMap[i] = NULL;
	}
	
	SetPriority(B_DISPLAY_PRIORITY);
}


VideoConsumer::~VideoConsumer()
{
	Quit();
	DeleteBuffers();
}


BMediaAddOn*
VideoConsumer::AddOn(long* cookie) const
{
	FUNCTION("VideoConsumer::AddOn\n");
	// do the right thing if we're ever used with an add-on
	*cookie = fInternalID;
	return fAddOn;
}


void
VideoConsumer::NodeRegistered()
{
	FUNCTION("VideoConsumer::NodeRegistered\n");
	fIn.destination.port = ControlPort();
	fIn.destination.id = 0;
	fIn.source = media_source::null;
	fIn.format.type = B_MEDIA_RAW_VIDEO;
	// wild cards yet
	fIn.format.u.raw_video = media_raw_video_format::wildcard;
	fIn.format.u.raw_video.interlace = 1;
	fIn.format.u.raw_video.display.format = B_NO_COLOR_SPACE;
	fIn.format.u.raw_video.display.bytes_per_row = 0;
	fIn.format.u.raw_video.display.line_width = 0;
	fIn.format.u.raw_video.display.line_count = 0;

	Run();
}


status_t
VideoConsumer::RequestCompleted(const media_request_info& info)
{
	FUNCTION("VideoConsumer::RequestCompleted\n");
	switch(info.what) {
		case media_request_info::B_SET_OUTPUT_BUFFERS_FOR:
			if (info.status != B_OK)
				ERROR("VideoConsumer::RequestCompleted: Not using our "
					"buffers!\n");
			break;

		default:
			break;
	}
	return B_OK;
}


status_t
VideoConsumer::HandleMessage(int32 message, const void* data, size_t size)
{
	return B_OK;
}


void
VideoConsumer::BufferReceived(BBuffer* buffer)
{
	LOOP("VideoConsumer::Buffer #%ld received\n", buffer->ID());

	if (RunState() == B_STOPPED) {
		buffer->Recycle();
		return;
	}

	media_timed_event event(buffer->Header()->start_time,
		BTimedEventQueue::B_HANDLE_BUFFER, buffer,
		BTimedEventQueue::B_RECYCLE_BUFFER);
	EventQueue()->AddEvent(event);
}


void
VideoConsumer::ProducerDataStatus(const media_destination& forWhom,
	int32 status, bigtime_t atMediaTime)
{
	FUNCTION("VideoConsumer::ProducerDataStatus\n");

	if (forWhom != fIn.destination)	
		return;
}


status_t
VideoConsumer::CreateBuffers(const media_format& format)
{
	FUNCTION("VideoConsumer::CreateBuffers\n");
	
	// delete any old buffers
	DeleteBuffers();	

	status_t status = B_OK;

	// create a buffer group
	uint32 width = format.u.raw_video.display.line_width;
	uint32 height = format.u.raw_video.display.line_count;	
	color_space colorSpace = format.u.raw_video.display.format;
	PROGRESS("VideoConsumer::CreateBuffers - Width = %ld - Height = %ld - "
		"Colorspace = %d\n", width, height, colorSpace);

	fBuffers = new BBufferGroup();
	status = fBuffers->InitCheck();
	if (B_OK != status) {
		ERROR("VideoConsumer::CreateBuffers - ERROR CREATING BUFFER GROUP\n");
		return status;
	}

	// and attach the bitmaps to the buffer group
	BRect bounds(0, 0, width - 1, height - 1);
	for (uint32 i = 0; i < kBufferCount; i++) {
		// figure out the bitmap creation flags
		uint32 bitmapFlags = 0;
		if (fTryOverlay) {
			// try to use hardware overlay
			bitmapFlags |= B_BITMAP_WILL_OVERLAY;
			if (i == 0)
				bitmapFlags |= B_BITMAP_RESERVE_OVERLAY_CHANNEL;
		} else
			bitmapFlags = B_BITMAP_IS_LOCKED;
			
		fBitmap[i] = new BBitmap(bounds, bitmapFlags, colorSpace);
		status = fBitmap[i]->InitCheck();
		if (status >= B_OK) {
			buffer_clone_info info;

			uint8* bits = (uint8*)fBitmap[i]->Bits();
			info.area = area_for(bits);
			area_info bitmapAreaInfo;
			status = get_area_info(info.area, &bitmapAreaInfo);
			if (status != B_OK) {
				fprintf(stderr, "VideoConsumer::CreateBuffers() - "
					"get_area_info(): %s\n", strerror(status));
				return status;
			}

//printf("area info for bitmap %ld (%p):\n", i, fBitmap[i]->Bits());
//printf("        area: %ld\n", bitmapAreaInfo.area);
//printf("        size: %ld\n", bitmapAreaInfo.size);
//printf("        lock: %ld\n", bitmapAreaInfo.lock);
//printf("  protection: %ld\n", bitmapAreaInfo.protection);
//printf("    ram size: %ld\n", bitmapAreaInfo.ram_size);
//printf("  copy_count: %ld\n", bitmapAreaInfo.copy_count);
//printf("   out_count: %ld\n", bitmapAreaInfo.out_count);
//printf("     address: %p\n", bitmapAreaInfo.address);

			info.offset = bits - (uint8*)bitmapAreaInfo.address;
			info.size = (size_t)fBitmap[i]->BitsLength();
			info.flags = 0;
			info.buffer = 0;
				// the media buffer id

			BBuffer* buffer = NULL;
			if ((status = fBuffers->AddBuffer(info, &buffer)) != B_OK) {
				ERROR("VideoConsumer::CreateBuffers - ERROR ADDING BUFFER "
					"TO GROUP (%ld): %s\n", i, strerror(status));
				return status;
			} else {
				PROGRESS("VideoConsumer::CreateBuffers - SUCCESSFUL ADD "
					"BUFFER TO GROUP\n");
			}
			fBufferMap[i] = buffer;
		} else {
			ERROR("VideoConsumer::CreateBuffers - ERROR CREATING VIDEO RING "
				"BUFFER (Index %ld Width %ld Height %ld Colorspace %d: %s\n",
				i, width, height, colorSpace, strerror(status));
			return status;
		}	
	}
	
	FUNCTION("VideoConsumer::CreateBuffers - EXIT\n");
	return status;
}


void
VideoConsumer::DeleteBuffers()
{
	FUNCTION("VideoConsumer::DeleteBuffers\n");
	if (fBuffers) {
		fTargetLock.Lock();
		if (fLastBufferIndex >= 0) {
			if (fTarget)
				fTarget->SetBitmap(NULL);
			fLastBufferIndex = -1;
		}
		fTargetLock.Unlock();

		delete fBuffers;
		fBuffers = NULL;

		for (uint32 i = 0; i < kBufferCount; i++) {
			snooze(20000);
			delete fBitmap[i];
			fBitmap[i] = NULL;
		}
	}
	FUNCTION("VideoConsumer::DeleteBuffers - EXIT\n");
}


void
VideoConsumer::SetTarget(VideoTarget* target)
{
	fTargetLock.Lock();
	if (fTarget)
		fTarget->SetBitmap(NULL);
	fTarget = target;
	if (fTarget && fLastBufferIndex >= 0)
		fTarget->SetBitmap(fBitmap[fLastBufferIndex]);
	fTargetLock.Unlock();
}


void
VideoConsumer::SetTryOverlay(bool tryOverlay)
{
	fTryOverlay = tryOverlay;
}


status_t
VideoConsumer::Connected(const media_source& producer,
	const media_destination& where, const media_format& format,
	media_input* outInput)
{
	FUNCTION("VideoConsumer::Connected\n");
	
	fIn.source = producer;
	fIn.format = format;
	fIn.node = Node();
	sprintf(fIn.name, "Video Consumer");
	*outInput = fIn;

	uint32 userData = 0;
	int32 changeTag = 1;
	status_t ret = CreateBuffers(format);
	if (ret == B_OK) {
		// TODO: With overlay bitmaps, there seems to be a problem with
		// mapping the BBitmap areas into the BBuffers. Until that is fixed,
		// don't enable a shared BBufferGroup.
		if (!fTryOverlay) {
			ret = SetOutputBuffersFor(producer, fIn.destination, 
				fBuffers, &userData, &changeTag, true);
			if (ret != B_OK)
				ERROR("SetOutputBuffersFor() failed: %s\n", strerror(ret));
		}
		fIn.format.u.raw_video.display.bytes_per_row
			= fBitmap[0]->BytesPerRow();
	} else {
		ERROR("VideoConsumer::Connected - COULDN'T CREATE BUFFERS\n");
		return ret;
	}

	*outInput = fIn;
		// bytes per row might have changed
	fConnectionActive = true;

	FUNCTION("VideoConsumer::Connected - EXIT\n");
	return B_OK;
}


void
VideoConsumer::Disconnected(const media_source& producer,
	const media_destination& where)
{
	FUNCTION("VideoConsumer::Disconnected\n");

	if (where != fIn.destination || producer != fIn.source)
		return;

	// reclaim our buffers
	int32 changeTag = 0;
	SetOutputBuffersFor(producer, fIn.destination, NULL, NULL, &changeTag,
		false);
	if (fOurBuffers) {
		status_t reclaimError = fBuffers->ReclaimAllBuffers();
		if (reclaimError != B_OK) {
			fprintf(stderr, "VideoConsumer::Disconnected() - Failed to "
				"reclaim our buffers: %s\n", strerror(reclaimError));
		}
	}
	// disconnect the connection
	fIn.source = media_source::null;
	fConnectionActive = false;

	// Unset the target's bitmap. Just to be safe -- since it is usually
	// done when the stop event arrives, but someone may disonnect
	// without stopping us before.
	_UnsetTargetBuffer();
}


status_t
VideoConsumer::AcceptFormat(const media_destination& dest, media_format* format)
{
	FUNCTION("VideoConsumer::AcceptFormat\n");
	
	if (dest != fIn.destination) {
		ERROR("VideoConsumer::AcceptFormat - BAD DESTINATION\n");
		return B_MEDIA_BAD_DESTINATION;	
	}
	
	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;
	
	if (format->type != B_MEDIA_RAW_VIDEO) {
		ERROR("VideoConsumer::AcceptFormat - BAD FORMAT\n");
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.format
			!= media_raw_video_format::wildcard.display.format) {
		uint32 flags = 0;
		bool supported = bitmaps_support_space(
			format->u.raw_video.display.format, &flags);
#ifndef HAIKU_TARGET_PLATFORM_HAIKU
		// GRRR! BeOS implementation claims not
		// to support these formats, while they work just fine.
		switch (format->u.raw_video.display.format) {
			case B_YCbCr422:
			case B_YCbCr411:
			case B_YCbCr444:
			case B_YCbCr420:
				supported = true;
				break;
			default:
				break;
		}
#endif
		if (!supported) {
			// cannot create bitmaps with such a color space
			ERROR("AcceptFormat - unsupported color space for BBitmaps "
				"(%s)!\n",
				color_space_to_string(format->u.raw_video.display.format));
			return B_MEDIA_BAD_FORMAT;
		}
		if (!fTryOverlay && (flags & B_VIEWS_SUPPORT_DRAW_BITMAP) == 0) {
			// BViews do not support drawing such a bitmap
			ERROR("AcceptFormat - BViews cannot draw bitmaps in given "
				"colorspace (%s)!\n",
				color_space_to_string(format->u.raw_video.display.format));
			return B_MEDIA_BAD_FORMAT;
		}
	}

	#ifdef TRACE_VIDEO_CONSUMER
		char string[256];
		string[0] = 0;
		string_for_format(*format, string, 256);
		FUNCTION("VideoConsumer::AcceptFormat: %s\n", string);
	#endif

	return B_OK;
}


status_t
VideoConsumer::GetNextInput(int32* cookie, media_input* outInput)
{
	FUNCTION("VideoConsumer::GetNextInput\n");

	// custom build a destination for this connection
	// put connection number in id

	if (*cookie < 1) {
		fIn.node = Node();
		fIn.destination.id = *cookie;
		sprintf(fIn.name, "Video Consumer");
		*outInput = fIn;
		(*cookie)++;
		return B_OK;
	} else {
		return B_MEDIA_BAD_DESTINATION;
	}
}


void
VideoConsumer::DisposeInputCookie(int32 /*cookie*/)
{
}


status_t
VideoConsumer::GetLatencyFor(const media_destination& whom,
	bigtime_t* _latency, media_node_id* _timeSource)
{
	FUNCTION("VideoConsumer::GetLatencyFor\n");
	
	if (whom != fIn.destination)
		return B_MEDIA_BAD_DESTINATION;
	
	*_latency = fMyLatency;
	*_timeSource = TimeSource()->ID();
	return B_OK;
}


status_t
VideoConsumer::FormatChanged(const media_source& producer,
	const media_destination& consumer, int32 fromChangeCount,
	const media_format& format)
{
	FUNCTION("VideoConsumer::FormatChanged\n");
	
	if (consumer != fIn.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (producer != fIn.source)
		return B_MEDIA_BAD_SOURCE;

	fIn.format = format;
	
	return CreateBuffers(format);
}


void
VideoConsumer::HandleEvent(const media_timed_event* event, bigtime_t lateness,
	bool realTimeEvent)
{
	LOOP("VideoConsumer::HandleEvent\n");
	
	switch (event->type) {
		case BTimedEventQueue::B_START:
			PROGRESS("VideoConsumer::HandleEvent - START\n");
			_SetPerformanceTimeBase(event->event_time);
			break;
		case BTimedEventQueue::B_WARP:
		case BTimedEventQueue::B_SEEK:
			PROGRESS("VideoConsumer::HandleEvent - WARP or SEEK\n");
			_SetPerformanceTimeBase(event->bigdata);
			break;

		case BTimedEventQueue::B_STOP:
			PROGRESS("VideoConsumer::HandleEvent - STOP\n");
			EventQueue()->FlushEvents(event->event_time, BTimedEventQueue::B_ALWAYS, true, BTimedEventQueue::B_HANDLE_BUFFER);
			// unset the target's bitmap
			_UnsetTargetBuffer();
			break;

		case BTimedEventQueue::B_HANDLE_BUFFER:
			LOOP("VideoConsumer::HandleEvent - HANDLE BUFFER\n");
			_HandleBuffer(static_cast<BBuffer*>(event->pointer));
			break;
		default:
			ERROR("VideoConsumer::HandleEvent - BAD EVENT\n");
			break;
	}			
}


void
VideoConsumer::_SetPerformanceTimeBase(bigtime_t performanceTime)
{
	fPerformanceTimeBase = performanceTime;
}


void
VideoConsumer::_HandleBuffer(BBuffer* buffer)
{
	if (RunState() != B_STARTED || !fConnectionActive) {
		TRACE("RunState() != B_STARTED\n");
		buffer->Recycle();
		return;
	}

	// See if this is one of our BBitmap buffers
	uint32 index = 0;
	fOurBuffers = true;
	while (index < kBufferCount) {
		if (buffer->ID() == fBufferMap[index]->ID())
			break;
		else
			index++;
	}
	if (index == kBufferCount) {
		// Buffers belong to consumer
		// NOTE: We maintain this in a member variable, since we still need
		// to recycle this buffer later on, in case it was the last buffer
		// received before shutting down.
		fOurBuffers = false;
		index = (fLastBufferIndex + 1) % kBufferCount;
	}

	bool recycle = true;
	bigtime_t now = TimeSource()->Now();
	if (RunMode() == B_OFFLINE
		|| now < buffer->Header()->start_time + kMaxBufferLateness) {
		// Only display the buffer if it's not too late, or if we are
		// in B_OFFLINE run-mode.
		if (!fOurBuffers) {
			memcpy(fBitmap[index]->Bits(), buffer->Data(),
				fBitmap[index]->BitsLength());
		}
		bigtime_t tooEarly = buffer->Header()->start_time - now;
		if (tooEarly > 3000)
			snooze(tooEarly);

		fTargetLock.Lock();
		if (fTarget) {
			fTarget->SetBitmap(fBitmap[index]);
			if (fOurBuffers) {
				// recycle the previous but not the current buffer
				if (fLastBufferIndex >= 0)
					fBufferMap[fLastBufferIndex]->Recycle();
				recycle = false;
			}
			fLastBufferIndex = index;
		}
		fTargetLock.Unlock();
	} else {
		// Drop the buffer if it's too late.
		if (fManager->LockWithTimeout(10000) == B_OK) {
			fManager->FrameDropped();
			fManager->Unlock();
		}
		PROGRESS("VideoConsumer::HandleEvent - DROPPED FRAME\n"
			"   start_time: %lld, current: %lld, latency: %lld\n",
			buffer->Header()->start_time, TimeSource()->Now(),
			SchedulingLatency());
	}
	if (recycle)
		buffer->Recycle();
}


void
VideoConsumer::_UnsetTargetBuffer()
{
	fTargetLock.Lock();
	if (fLastBufferIndex >= 0) {
		if (fTarget)
			fTarget->SetBitmap(NULL);
		if (fOurBuffers)
			fBufferMap[fLastBufferIndex]->Recycle();
		fLastBufferIndex = -1;
	}
	fTargetLock.Unlock();
}
