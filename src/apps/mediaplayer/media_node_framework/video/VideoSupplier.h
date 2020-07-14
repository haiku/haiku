/*
 * Copyright 2001-2008 Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2001-2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#ifndef VIDEO_SUPPLIER_H
#define VIDEO_SUPPLIER_H


#include <SupportDefs.h>
#include <MediaFormats.h>


struct media_raw_video_format;


class VideoSupplier {
public:
								VideoSupplier();
	virtual						~VideoSupplier();

	virtual	const media_format&	Format() const = 0;
	virtual	status_t			FillBuffer(int64 startFrame, void* buffer,
									const media_raw_video_format& format,
									bool forceGeneration, bool& wasCached) = 0;

	virtual	void				DeleteCaches();

 	inline	bigtime_t			ProcessingLatency() const
 									{ return fProcessingLatency; }

protected:
		 	bigtime_t			fProcessingLatency;
};

#endif	// VIDEO_SUPPLIER_H
