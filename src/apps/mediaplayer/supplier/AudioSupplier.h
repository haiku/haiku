/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef AUDIO_SUPPLIER_H
#define AUDIO_SUPPLIER_H

#include <MediaDefs.h>

class AudioSupplier {
 public:
								AudioSupplier();
	virtual						~AudioSupplier();

	virtual	media_format		Format() const = 0;
	virtual	status_t			ReadFrames(void* buffer, int64* framesRead,
									bigtime_t* performanceTime) = 0;
	virtual	status_t			SeekToTime(bigtime_t* performanceTime) = 0;

	virtual	bigtime_t			Position() const = 0;
	virtual	bigtime_t			Duration() const = 0;
};

#endif // AUDIO_SUPPLIER_H
