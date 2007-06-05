/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef MEDIA_TRACK_AUDIO_SUPPLIER_H
#define MEDIA_TRACK_AUDIO_SUPPLIER_H

#include "AudioSupplier.h"

class BMediaTrack;

class MediaTrackAudioSupplier : public AudioSupplier {
 public:
								MediaTrackAudioSupplier(BMediaTrack* track);
	virtual						~MediaTrackAudioSupplier();

	virtual	media_format		Format() const;
	virtual	status_t			GetEncodedFormat(media_format* format) const;
	virtual	status_t			GetCodecInfo(media_codec_info* info) const;

	virtual	status_t			ReadFrames(void* buffer, int64* framesRead,
									bigtime_t* performanceTime);
	virtual	status_t			SeekToTime(bigtime_t* performanceTime);

	virtual	bigtime_t			Position() const
									{ return fPerformanceTime; }
	virtual	bigtime_t			Duration() const
									{ return fDuration; }
 private:
			BMediaTrack*		fMediaTrack;

			media_format		fFormat;

			bigtime_t			fPerformanceTime;
			bigtime_t			fDuration;
};

#endif // MEDIA_TRACK_AUDIO_SUPPLIER_H
