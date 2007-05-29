/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef VIDEO_SUPPLIER_H
#define VIDEO_SUPPLIER_H

#include <MediaDefs.h>

class VideoSupplier {
 public:
								VideoSupplier();
	virtual						~VideoSupplier();

	virtual	media_format		Format() const = 0;
	virtual	status_t			ReadFrame(void* buffer,
									bigtime_t* performanceTime) = 0;
	virtual	status_t			SeekToTime(bigtime_t* performanceTime) = 0;

	virtual	bigtime_t			Position() const = 0;
	virtual	bigtime_t			Duration() const = 0;
};

#endif // VIDEO_SUPPLIER_H
