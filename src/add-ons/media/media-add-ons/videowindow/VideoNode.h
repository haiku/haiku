/*
 * VideoNode.h - "Video Window" media add-on.
 *
 * Copyright (C) 2006 Marcus Overhagen <marcus@overhagen.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */
#ifndef __VIDEO_NODE_H_
#define __VIDEO_NODE_H_

#include <BufferConsumer.h>
#include <MediaEventLooper.h>

class VideoView;

class VideoNode : public BMediaEventLooper, public BBufferConsumer
{
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

	status_t		HandleMessage(
							int32 message,
							const void *data,
							size_t size);

	void			HandleEvent(
							const media_timed_event *event,
							bigtime_t lateness,
							bool realTimeEvent);

	status_t		AcceptFormat(
							const media_destination &dst,
							media_format *format);

	void			ProducerDataStatus(
							const media_destination &dst,
							int32 status,
							bigtime_t at_media_time);
															
	status_t		GetLatencyFor(
							const media_destination &dst,
							bigtime_t *out_latency,
							media_node_id *out_id);

	status_t		Connected(
							const media_source &src,
							const media_destination &dst,
							const media_format &format,
							media_input *out_input);

	void			Disconnected(
							const media_source &src,
							const media_destination &dst);

	status_t		FormatChanged(
							const media_source &src,
							const media_destination &dst,
							int32 from_change_count,
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

#endif
