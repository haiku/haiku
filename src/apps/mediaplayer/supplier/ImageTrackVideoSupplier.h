/*
 * Copyright 2011, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		SHINTA
 */
#ifndef IMAGE_TRACK_VIDEO_SUPPLIER_H
#define IMAGE_TRACK_VIDEO_SUPPLIER_H

#include <Bitmap.h>
#include <MediaFormats.h>

#include "VideoTrackSupplier.h"


class ImageTrackVideoSupplier : public VideoTrackSupplier {
public:
								ImageTrackVideoSupplier(BBitmap* bitmap,
									int32 trackIndex, status_t& initStatus);
	virtual						~ImageTrackVideoSupplier();

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

#if 0
	virtual	BRect				Bounds() const;
	virtual	color_space			ColorSpace() const;
	virtual	uint32				BytesPerRow() const;
#endif

	virtual	int32				TrackIndex() const
									{ return fTrackIndex; }

private:
			media_format		fFormat;

			bigtime_t			fPerformanceTime;
			bigtime_t			fDuration;
			int64				fCurrentFrame;

			BBitmap*			fBitmap;
			int32				fTrackIndex;
};


#endif // IMAGE_TRACK_VIDEO_SUPPLIER_H
