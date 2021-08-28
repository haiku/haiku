/*
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Copyright (C) 2008 Maurice Kalinowski <haiku@kaldience.com>. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */
#include "VideoNode.h"
#include "VideoView.h"
#include "VideoWindow.h"


#include <Bitmap.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Debug.h>
#include <MediaRoster.h>
#include <Locker.h>
#include <TimeSource.h>
#include <Window.h>
#include <stdio.h>
#include <string.h>


VideoNode::VideoNode(const char *name)
 :	BMediaNode(name)
 ,	BMediaEventLooper()
 ,	BBufferConsumer(B_MEDIA_RAW_VIDEO)
 ,	fWindow(0)
 ,	fVideoView(0)
 ,	fInput()
 ,	fOverlayEnabled(true)
 ,	fOverlayActive(false)
 ,	fDirectOverlayBuffer(false)
 ,	fBitmap(0)
 ,	fBitmapLocker(new BLocker("Video Node Locker"))
 ,	fAddOn(0)
 ,	fInternalFlavorId(0)
{
	_InitDisplay();
}


VideoNode::VideoNode(const char *name, BMediaAddOn* addon, int32 id)
 :	BMediaNode(name)
 ,	BMediaEventLooper()
 ,	BBufferConsumer(B_MEDIA_RAW_VIDEO)
 ,	fWindow(0)
 ,	fVideoView(0)
 ,	fInput()
 ,	fOverlayEnabled(true)
 ,	fOverlayActive(false)
 ,	fDirectOverlayBuffer(false)
 ,	fBitmap(0)
 ,	fBitmapLocker(new BLocker("Video Node Locker"))
 ,	fAddOn(addon)
 ,	fInternalFlavorId(id)
{
	_InitDisplay();
}


VideoNode::~VideoNode()
{
	Quit();
	DeleteBuffers();
	delete fBitmapLocker;
	if (fWindow && fWindow->Lock())
		fWindow->Quit();
}


BMediaAddOn	*
VideoNode::AddOn(int32 *internal_id) const
{
	*internal_id = fInternalFlavorId;
	return fAddOn;
}


void
VideoNode::NodeRegistered()
{
	fInput.node = Node();
	fInput.source = media_source::null;
	fInput.destination.port = ControlPort();
	fInput.destination.id = 0;
	fInput.format.type = B_MEDIA_RAW_VIDEO;
	fInput.format.u.raw_video = media_raw_video_format::wildcard;
	strcpy(fInput.name, "video in");

	SetPriority(B_DISPLAY_PRIORITY);
	Run();
}


void
VideoNode::BufferReceived(BBuffer * buffer)
{
	if (RunState() != B_STARTED) {
		buffer->Recycle();
		return;
	}
	if (fOverlayActive && fDirectOverlayBuffer) {
		HandleBuffer(buffer);
	} else {
		media_timed_event event(buffer->Header()->start_time,
			BTimedEventQueue::B_HANDLE_BUFFER, buffer,
			BTimedEventQueue::B_RECYCLE_BUFFER);
		EventQueue()->AddEvent(event);
	}
}


status_t
VideoNode::GetNextInput(int32 *cookie,	media_input *out_input)
{
	if (*cookie < 1) {
		*out_input = fInput;
		*cookie += 1;
		return B_OK;
	}
	return B_ERROR;
}


void
VideoNode::DisposeInputCookie(int32 cookie)
{
	// nothing to do
}


status_t
VideoNode::	HandleMessage(int32 message, const void *data, size_t size)
{
	return B_ERROR;
}


void
VideoNode::HandleEvent(const media_timed_event *event, bigtime_t lateness,
	bool realTimeEvent)
{
	switch (event->type) {
		case BTimedEventQueue::B_START:
			break;
		case BTimedEventQueue::B_STOP:
			EventQueue()->FlushEvents(event->event_time,
				BTimedEventQueue::B_ALWAYS, true,
				BTimedEventQueue::B_HANDLE_BUFFER);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
			HandleBuffer((BBuffer *)event->pointer);
			break;
		case BTimedEventQueue::B_SEEK:
			fprintf(stderr, "VideoNode::HandleEvent Seek event not handled\n");
			break;
		default:
			fprintf(stderr, "VideoNode::HandleEvent unknown event\n");
			break;
	}
}


void
VideoNode::ProducerDataStatus(const media_destination &dst, int32 status,
	bigtime_t at_media_time)
{
	// do nothing
}


status_t
VideoNode::GetLatencyFor(const media_destination &dst,  bigtime_t *out_latency,
	media_node_id *out_id)
{
	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	*out_latency = 10000;
	*out_id = TimeSource()->ID();
	return B_OK;
}


status_t
VideoNode::AcceptFormat(const media_destination &dst, media_format *format)
{
	/* The connection process:
	 *                BBufferProducer::FormatProposal
	 * we are here => BBufferConsumer::AcceptFormat
	 *                BBufferProducer::PrepareToConnect
	 *                BBufferConsumer::Connected
	 *                BBufferProducer::Connect
	 */
	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	if (format->type == B_MEDIA_NO_TYPE)
		format->type = B_MEDIA_RAW_VIDEO;

	if (format->type != B_MEDIA_RAW_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	return B_OK;
}


status_t
VideoNode::Connected(const media_source &src, const media_destination &dst,
	const media_format &format, media_input *out_input)
{
	/* The connection process:
	 *                BBufferProducer::FormatProposal
	 *                BBufferConsumer::AcceptFormat
	 *                BBufferProducer::PrepareToConnect
	 * we are here => BBufferConsumer::Connected
	 *                BBufferProducer::Connect
	 */

	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	fInput.source = src;
	fInput.format = format;

	if (fInput.format.u.raw_video.field_rate < 1.0)
		fInput.format.u.raw_video.field_rate = 25.0;

	// In order to display video we need to create a buffer that is either
	// in the overlay colorspace B_YCbCr422
	// or the requested colorspace if not B_YCbCr422
	// and we need to tell the node upstream of our choice

	BRect frame(0, 0, format.u.raw_video.display.line_width - 1,
		format.u.raw_video.display.line_count - 1);

	DeleteBuffers();
	status_t err;

	if (format.u.raw_video.display.format == B_NO_COLOR_SPACE) {
		// upstream node is leaving it up to us so we try overlay then
		// B_RGBA32 (We probably should try what format the screen is)
		err = CreateBuffers(frame, B_YCbCr422, true);
		SetOverlayEnabled(err == B_OK);
		if (!fOverlayEnabled) {
			// no overlay available so fall back to RGBA32
			err = CreateBuffers(frame, B_RGBA32, false);
		}
	} else if (format.u.raw_video.display.format == B_YCbCr422) {
		// upstream node is likely requesting overlay
		err = CreateBuffers(frame, B_YCbCr422, true);
		SetOverlayEnabled(err == B_OK);
		// if we cannot give them what they want then return error
	} else {
		// upstream node is requesting some other format
		SetOverlayEnabled(false);
		err = CreateBuffers(frame, format.u.raw_video.display.format,
			fOverlayEnabled);
	}

	if (err) {
		fprintf(stderr, "VideoNode::Connected failed, fOverlayEnabled = %d\n",
			fOverlayEnabled);
		return err;
	} else {
		fInput.format.u.raw_video.display.format = fBitmap->ColorSpace();
	}

	*out_input = fInput;

	return B_OK;
}


void
VideoNode::Disconnected(const media_source &src, const media_destination &dst)
{
	if (src != fInput.source)
		return;
	if (dst != fInput.destination)
		return;

	DeleteBuffers();

	// disconnect the connection
	fInput.source = media_source::null;
}


status_t
VideoNode::FormatChanged(const media_source &src, const media_destination &dst,
	int32 from_change_count, const media_format &format)
{
	if (src != fInput.source)
		return B_MEDIA_BAD_SOURCE;
	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	color_space colorspace = format.u.raw_video.display.format;
	BRect frame(0, 0, format.u.raw_video.display.line_width - 1,
		format.u.raw_video.display.line_count - 1);

	status_t err;

	DeleteBuffers();
	if (fOverlayEnabled) {
		fVideoView->RemoveOverlay();
		err = CreateBuffers(frame, colorspace, true); // try overlay
		if (err) {
			fprintf(stderr, "VideoNode::FormatChanged creating overlay buffer failed\n");
			err = CreateBuffers(frame, colorspace, false); // no overlay
		}
	} else {
		err = CreateBuffers(frame, colorspace, false); // no overlay
	}

	if (err) {
		fprintf(stderr, "VideoNode::FormatChanged failed (lost buffer group!)\n");
		return B_MEDIA_BAD_FORMAT;
	}

	fInput.format = format;

	return B_OK;
}


void
VideoNode::HandleBuffer(BBuffer *buffer)
{
	LockBitmap();
	if (fBitmap && fWindow && fVideoView) {
//		bigtime_t start = system_time();
		if (fOverlayActive) {
			if (B_OK == fBitmap->LockBits()) {
				memcpy(fBitmap->Bits(), buffer->Data(), fBitmap->BitsLength());
				fBitmap->UnlockBits();
			}
		} else {
			memcpy(fBitmap->Bits(), buffer->Data(), fBitmap->BitsLength());
		}
//		printf("overlay copy: %Ld usec\n", system_time() - start);
	}
	UnlockBitmap();

	buffer->Recycle();

	fVideoView->DrawFrame();
}


void
VideoNode::SetOverlayEnabled(bool yesno)
{
	fOverlayEnabled = yesno;
}


void
VideoNode::LockBitmap()
{
	fBitmapLocker->Lock();
}


BBitmap *
VideoNode::Bitmap()
{
	return fBitmap;
}


void
VideoNode::UnlockBitmap()
{
	fBitmapLocker->Unlock();
}


bool
VideoNode::IsOverlayActive()
{
	return fOverlayActive;
}


status_t
VideoNode::CreateBuffers(BRect frame, color_space cspace, bool overlay)
{
	printf("VideoNode::CreateBuffers: frame %d,%d,%d,%d colorspace 0x%08x, "
		"overlay %d\n", int(frame.left), int(frame.top), int(frame.right),
		int(frame.bottom), int(cspace), overlay);

	LockBitmap();
	ASSERT(fBitmap == 0);

	uint32 flags = 0;
	if (overlay)
		flags = B_BITMAP_WILL_OVERLAY | B_BITMAP_RESERVE_OVERLAY_CHANNEL;

	fBitmap = new BBitmap(frame, flags, cspace);
	if (!(fBitmap && fBitmap->InitCheck() == B_OK && fBitmap->IsValid())) {
		delete fBitmap;
		fBitmap = NULL;
		fOverlayActive = false;
		UnlockBitmap();
		fprintf(stderr, "VideoNode::CreateBuffers failed\n");
		return B_MEDIA_BAD_FORMAT;
	}
	fOverlayActive = overlay;
	UnlockBitmap();

	return B_OK;
}


void
VideoNode::DeleteBuffers()
{
	LockBitmap();
	delete fBitmap;
	fBitmap = NULL;
	UnlockBitmap();
}


void
VideoNode::_InitDisplay()
{
	// TODO: Get rid of hardcoded values
	BRect size(0,0,320,240);
	fVideoView = new VideoView(size, "Video View", B_FOLLOW_ALL_SIDES,
		B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE, this);

	size.OffsetBy(40.f, 40.f);
	fWindow = new VideoWindow(size, fVideoView);
	fWindow->Show();
}
