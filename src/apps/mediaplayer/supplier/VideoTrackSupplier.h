/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef VIDEO_TRACK_SUPPLIER_H
#define VIDEO_TRACK_SUPPLIER_H


#include <MediaDefs.h>
#include <MediaFormats.h>

class VideoTrackSupplier {
public:
								VideoTrackSupplier();
	virtual						~VideoTrackSupplier();

	virtual	const media_format&	Format() const = 0;
	virtual	status_t			GetEncodedFormat(media_format* format)
									const = 0;
	virtual	status_t			GetCodecInfo(media_codec_info* info) const = 0;
	virtual	status_t			ReadFrame(void* buffer,
									bigtime_t* performanceTime,
									const media_raw_video_format& format,
									bool& wasCached) = 0;
	virtual	status_t			FindKeyFrameForFrame(int64* frame) = 0;
	virtual	status_t			SeekToTime(bigtime_t* performanceTime) = 0;
	virtual	status_t			SeekToFrame(int64* frame) = 0;

	virtual	bigtime_t			Position() const = 0;
	virtual	bigtime_t			Duration() const = 0;
	virtual	int64				CurrentFrame() const = 0;

	virtual	int32				TrackIndex() const = 0;
};

#endif // VIDEO_TRACK_SUPPLIER_H
