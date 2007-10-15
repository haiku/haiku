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
#ifndef __VIDEO_NODE_H_
#define __VIDEO_NODE_H_


#include <BufferConsumer.h>
#include <MediaEventLooper.h>

class BLocker;
class VideoView;


class VideoNode : public BMediaEventLooper, public BBufferConsumer {
public:
					VideoNode(const char *name, VideoView *view);
					~VideoNode();

	void			SetOverlayEnabled(bool yesno);
	bool			IsOverlayActive();
	
	void			LockBitmap();
	BBitmap *		Bitmap();
	void			UnlockBitmap();

protected:	
	BMediaAddOn	*	AddOn(int32 *internal_id) const;

	void			NodeRegistered();

	void			BufferReceived(BBuffer * buffer);

	status_t		GetNextInput(int32 *cookie,	media_input *out_input);
	void			DisposeInputCookie(int32 cookie);

	status_t		HandleMessage(int32 message, const void *data, size_t size);

	void			HandleEvent(const media_timed_event *event,
						bigtime_t lateness, bool realTimeEvent);

	status_t		AcceptFormat(const media_destination &dst,
						media_format *format);

	void			ProducerDataStatus(const media_destination &dst,
						int32 status, bigtime_t at_media_time);
															
	status_t		GetLatencyFor(const media_destination &dst,
						bigtime_t *out_latency, media_node_id *out_id);

	status_t		Connected(const media_source &src,
						const media_destination &dst,
						const media_format &format, media_input *out_input);

	void			Disconnected(const media_source &src,
						const media_destination &dst);

	status_t		FormatChanged(const media_source &src,
						const media_destination &dst, int32 from_change_count,
						const media_format &format);

protected:
	void			HandleBuffer(BBuffer *buffer);
	status_t		CreateBuffers(BRect frame, color_space cspace, bool overlay);
	void			DeleteBuffers();

protected:
	VideoView *		fVideoView;
	media_input		fInput;
	bool			fOverlayEnabled;
	bool			fOverlayActive;
	bool			fDirectOverlayBuffer;	// If the overlay memory is directly written by the producer node.
	BBitmap *		fBitmap;
	BLocker	*		fBitmapLocker;
};

#endif	// __VIDEO_NODE_H_
