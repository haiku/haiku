/*
 * Copyright 2007-2008, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef AUDIO_TRACK_SUPPLIER_H
#define AUDIO_TRACK_SUPPLIER_H

#include <MediaDefs.h>
#include <MediaFormats.h>

#include "AudioReader.h"

class AudioTrackSupplier : public AudioReader {
public:
								AudioTrackSupplier();
	virtual						~AudioTrackSupplier();

	virtual	const media_format&	Format() const = 0;
	virtual	status_t			GetEncodedFormat(media_format* format)
									const = 0;
	virtual	status_t			GetCodecInfo(media_codec_info* info) const = 0;
	virtual	bigtime_t			Duration() const = 0;

	virtual	int32				TrackIndex() const = 0;
};

#endif // AUDIO_TRACK_SUPPLIER_H
