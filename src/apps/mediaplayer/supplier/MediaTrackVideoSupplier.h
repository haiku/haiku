/*
 * Copyright 2007-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MEDIA_TRACK_VIDEO_SUPPLIER_H
#define MEDIA_TRACK_VIDEO_SUPPLIER_H

#include "VideoTrackSupplier.h"

#include <MediaFormats.h>

class BMediaTrack;

class MediaTrackVideoSupplier : public VideoTrackSupplier {
 public:
								MediaTrackVideoSupplier(BMediaTrack* track,
									int32 trackIndex, status_t& initStatus);
	virtual						~MediaTrackVideoSupplier();

	virtual	const media_format&	Format() const;
	virtual	status_t			GetEncodedFormat(media_format* format) const;
	virtual	status_t			GetCodecInfo(media_codec_info* info) const;

	virtual	status_t			ReadFrame(void* buffer,
									bigtime_t* performanceTime,
									const media_raw_video_format& format,
									bool& wasCached);
	virtual	status_t			FindKeyFrameForFrame(int64* frame);
	virtual	status_t			SeekToTime(bigtime_t* performanceTime);
	virtual	status_t			SeekToFrame(int64* frame);

	virtual	bigtime_t			Position() const
									{ return fPerformanceTime; }
	virtual	bigtime_t			Duration() const
									{ return fDuration; }
	virtual	int64				CurrentFrame() const
									{ return fCurrentFrame; }

	virtual	BRect				Bounds() const;
	virtual	color_space			ColorSpace() const;
	virtual	uint32				BytesPerRow() const;

	virtual	int32				TrackIndex() const
									{ return fTrackIndex; }

 private:
			status_t			_SwitchFormat(color_space format,
									uint32 bytesPerRow);
			status_t			_SetDecodedFormat(uint32 width, uint32 height,
									color_space format, uint32 bytesPerRow);

			BMediaTrack*		fVideoTrack;

			media_format		fFormat;

			bigtime_t			fPerformanceTime;
			bigtime_t			fDuration;
			int64				fCurrentFrame;

			int32				fTrackIndex;
};

#endif // MEDIA_TRACK_VIDEO_SUPPLIER_H
