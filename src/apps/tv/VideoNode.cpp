/*
 * Copyright (c) 2004-2007 Marcus Overhagen <marcus@overhagen.de>
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without restriction, 
 * including without limitation the rights to use, copy, modify, 
 * merge, publish, distribute, sublicense, and/or sell copies of 
 * the Software, and to permit persons to whom the Software is 
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be 
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS IN THE SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <Window.h>
#include <TimeSource.h>
#include <MediaRoster.h>
#include <BufferGroup.h>
#include <Buffer.h>
#include <Bitmap.h>
#include <Locker.h>
#include <Debug.h>

#include "VideoNode.h"
#include "VideoView.h"

void
overlay_copy(uint32 lines, void *dst, uint32 dst_bpr, const void *src, 
	uint32 src_bpr)
{
//	bigtime_t start = system_time();
	int len = min_c(dst_bpr, src_bpr);
//	int len4 = len / 4;
	while (lines--) {
/*
		// this does not copy the last few bytes, if length is not aligned 
		// to 4 bytes
		asm ("rep\n\t""movsl"
		     : 
		     : "c" (len4), "S" (src), "D" (dst)
		     : "eax");
*/
/*
		const uint32 *s4 = (const uint32 *)src;
		uint32 *d4 = (uint32 *)dst;
		for (int i = 0; i < len4; i++)
			d4[i] = s4[i];
*/
/*
		const uint8 *s1 = (const uint8 *)s4;
		uint8 *d1 = (uint8 *)d4;
		int l1 = len1;
		while (l1--)
			*d1++ = *s1++;
*/
		memcpy(dst, src, len);
		src = (char *)src + src_bpr;
		dst = (char *)dst + dst_bpr;
	}
//	printf("overlay copy: %.06f sec\n", (system_time() - start) / 1000000.0);
}


VideoNode::VideoNode(const char *name, VideoView *view)
 :	BMediaNode(name)
 ,	BMediaEventLooper()
 ,	BBufferConsumer(B_MEDIA_RAW_VIDEO)
 ,	fVideoView(view)
 ,	fInput()
 ,	fOverlayEnabled(true)
 ,	fOverlayActive(false)
 ,	fDirectOverlayBuffer(false)
 ,	fBitmap(0)
 ,	fBitmapLocker(new BLocker("Video Node Locker"))
{
}


VideoNode::~VideoNode()
{
	Quit();
	DeleteBuffers();
	delete fBitmapLocker;
}


BMediaAddOn	*
VideoNode::AddOn(int32 *internal_id) const
{
	*internal_id = 0;
	return NULL;
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
								BTimedEventQueue::B_HANDLE_BUFFER,
								buffer,
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
VideoNode::	HandleMessage(int32 message,
						  const void *data,
						  size_t size)
{
	if (BBufferConsumer::HandleMessage(message, data, size) == B_OK
		|| BMediaEventLooper::HandleMessage(message, data, size) == B_OK)
		return B_OK;

	return B_ERROR;
}


void
VideoNode::HandleEvent(const media_timed_event *event,
					   bigtime_t lateness,
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
		default:
			printf("VideoNode::HandleEvent unknown event");
			break;
	}
}


void
VideoNode::ProducerDataStatus(const media_destination &dst,
							 int32 status,
							 bigtime_t at_media_time)
{
	// do nothing
}


status_t
VideoNode::GetLatencyFor(const media_destination &dst,
						 bigtime_t *out_latency,
						 media_node_id *out_id)
{
	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;
	
	*out_latency = 10000;
	*out_id = TimeSource()->ID();
	return B_OK;
}

status_t
VideoNode::AcceptFormat(const media_destination &dst,
						media_format *format)
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
VideoNode::Connected(const media_source &src,
					 const media_destination &dst,
					 const media_format &format,
					 media_input *out_input)
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

	color_space colorspace = format.u.raw_video.display.format;
	BRect		frame(0, 0, format.u.raw_video.display.line_width - 1, 
		format.u.raw_video.display.line_count - 1);
	status_t	err;

	DeleteBuffers();
	err = CreateBuffers(frame, colorspace, fOverlayEnabled);
	if (err) {
		printf("VideoNode::Connected failed, fOverlayEnabled = %d\n", 
			fOverlayEnabled);
		return err;
	}	

	*out_input = fInput;

	return B_OK;
	
}


void
VideoNode::Disconnected(const media_source &src,
						const media_destination &dst)
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
VideoNode::FormatChanged(const media_source &src,
						 const media_destination &dst,
						 int32 from_change_count,
						 const media_format &format)
{
	printf("VideoNode::FormatChanged enter\n");
	if (src != fInput.source)
		return B_MEDIA_BAD_SOURCE;
	if (dst != fInput.destination)
		return B_MEDIA_BAD_DESTINATION;

	color_space colorspace = format.u.raw_video.display.format;
	BRect		frame(0, 0, format.u.raw_video.display.line_width - 1, 
		format.u.raw_video.display.line_count - 1);
	status_t	err;

	DeleteBuffers();
	if (fOverlayEnabled) {
		fVideoView->RemoveOverlay();
		err = CreateBuffers(frame, colorspace, true); // try overlay
		if (err) {
			printf("VideoNode::FormatChanged creating overlay buffer "
				"failed\n");
			err = CreateBuffers(frame, colorspace, false); // no overlay
		}
	} else {
		err = CreateBuffers(frame, colorspace, false); // no overlay
	}
	if (err) {
		printf("VideoNode::FormatChanged failed (lost buffer group!)\n");
		return B_MEDIA_BAD_FORMAT;
	}	

	fInput.format = format;

	printf("VideoNode::FormatChanged leave\n");
	return B_OK;	
}


void
VideoNode::HandleBuffer(BBuffer *buffer)
{
//	printf("VideoNode::HandleBuffer\n");
	
	LockBitmap();
	if (fBitmap) {
//		bigtime_t start = system_time();
		if (fOverlayActive) {
			if (B_OK == fBitmap->LockBits()) {

//				memcpy(fBitmap->Bits(), buffer->Data(), fBitmap->BitsLength());

//				fBitmap->SetBits(buffer->Data(), fBitmap->BitsLength(), 0, 
//					fInput.format.u.raw_video.display.format);

				overlay_copy(fBitmap->Bounds().IntegerHeight() + 1, 
							 fBitmap->Bits(), fBitmap->BytesPerRow(), 
							 buffer->Data(), 
							 fInput.format.u.raw_video.display.bytes_per_row);

							
				fBitmap->UnlockBits();
			}
		} else {
//			memcpy(fBitmap->Bits(), buffer->Data(), fBitmap->BitsLength());

			overlay_copy(fBitmap->Bounds().IntegerHeight() + 1, 
						 fBitmap->Bits(), fBitmap->BytesPerRow(), 
						 buffer->Data(), 
						 fInput.format.u.raw_video.display.bytes_per_row);
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

//	int32 bytesPerRow = B_ANY_BYTES_PER_ROW;
//	if (cspace == B_YCbCr422)
//		bytesPerRow = (int(frame.Width()) + 1) * 2;

//	printf("overlay bitmap: requesting: bytes per row: %d\n", bytesPerRow);

	LockBitmap();
	ASSERT(fBitmap == 0);
	uint32 flags = overlay ? (B_BITMAP_WILL_OVERLAY 
		| B_BITMAP_RESERVE_OVERLAY_CHANNEL) : 0;
//	fBitmap = new BBitmap(frame, flags, cspace, bytesPerRow);
	fBitmap = new BBitmap(frame, flags, cspace);
	if (!(fBitmap && fBitmap->InitCheck() == B_OK && fBitmap->IsValid())) {
		delete fBitmap;
		fBitmap = 0;
		fOverlayActive = false;
		UnlockBitmap();
		printf("VideoNode::CreateBuffers failed\n");
		return B_ERROR;
	}
	printf("overlay bitmap: got: bytes per row: %" B_PRId32 "\n",
		fBitmap->BytesPerRow());
	fOverlayActive = overlay;
	UnlockBitmap();
	printf("VideoNode::CreateBuffers success\n");
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
