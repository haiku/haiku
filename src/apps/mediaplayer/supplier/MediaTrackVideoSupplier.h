/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MEDIA_TRACK_VIDEO_SUPPLIER_H
#define MEDIA_TRACK_VIDEO_SUPPLIER_H

#include "VideoSupplier.h"

class BMediaTrack;

class MediaTrackVideoSupplier : public VideoSupplier {
 public:
								MediaTrackVideoSupplier(BMediaTrack* track,
									color_space preferredFormat);
	virtual						~MediaTrackVideoSupplier();

	virtual	media_format		Format() const;
	virtual	status_t			ReadFrame(void* buffer,
									bigtime_t* performanceTime);
	virtual	status_t			SeekToTime(bigtime_t* performanceTime);

	virtual	bigtime_t			Position() const
									{ return fPerformanceTime; }
	virtual	bigtime_t			Duration() const
									{ return fDuration; }

	virtual	BRect				Bounds() const;
	virtual	color_space			ColorSpace() const;
	virtual	uint32				BytesPerRow() const;

 private:
			BMediaTrack*		fVideoTrack;

			media_format		fFormat;

			bigtime_t			fPerformanceTime;
			bigtime_t			fDuration;
};

#endif // MEDIA_TRACK_VIDEO_SUPPLIER_H
