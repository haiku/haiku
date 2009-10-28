/*
 * Copyright 2006 Marcus Overhagen <marcus@overhagen.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef __VIDEO_NODE_H_
#define __VIDEO_NODE_H_


#include <BufferConsumer.h>
#include <MediaEventLooper.h>


class VideoView;


class VideoNode : public BMediaEventLooper, public BBufferConsumer {
public:
								VideoNode(const char* name, VideoView* view);
	virtual						~VideoNode();

			void				SetOverlayEnabled(bool yesno);
			bool				IsOverlayActive();

			void				LockBitmap();
			BBitmap*			Bitmap();
			void				UnlockBitmap();

protected:
	virtual	BMediaAddOn*		AddOn(int32* internalID) const;

	// BMediaEventLooper implementation
	virtual	void				NodeRegistered();
	virtual	void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness, bool realTimeEvent);

	// BBuffer implementation
	virtual	status_t			HandleMessage(int32 message, const void* data,
									size_t size);

	virtual	status_t			AcceptFormat(const media_destination& dest,
									media_format* format);
	virtual	status_t			GetNextInput(int32* cookie,
									media_input* _input);
	virtual	void				DisposeInputCookie(int32 cookie);
	virtual	void				BufferReceived(BBuffer* buffer);
	virtual	void				ProducerDataStatus(
									const media_destination& dest, int32 status,
									bigtime_t atMediaTime);
	virtual	status_t			GetLatencyFor(const media_destination& dest,
									bigtime_t* _latency, media_node_id* _id);

	virtual	status_t			Connected(const media_source& source,
									const media_destination& dest,
									const media_format& format,
									media_input* _input);
	virtual	void				Disconnected(const media_source& source,
									const media_destination& dest);
	virtual status_t			FormatChanged(const media_source& source,
									const media_destination& dest,
									int32 fromChangeCount,
									const media_format& format);

protected:
			void				HandleBuffer(BBuffer* buffer);
			status_t			CreateBuffers(BRect frame, color_space space,
									bool overlay);
			void				DeleteBuffers();

protected:
			VideoView*			fVideoView;
			media_input			fInput;
			bool				fOverlayEnabled;
			bool				fOverlayActive;
			bool				fDirectOverlayBuffer;
				// If the overlay memory is directly written by the
				// producer node.
			BBitmap*			fBitmap;
			BLocker*			fBitmapLocker;
};

#endif	// __VIDEO_NODE_H_
