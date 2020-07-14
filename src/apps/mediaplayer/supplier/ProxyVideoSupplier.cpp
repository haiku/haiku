/*
 * Copyright 2008-2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */


#include "ProxyVideoSupplier.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Autolock.h>

#include "VideoTrackSupplier.h"


ProxyVideoSupplier::ProxyVideoSupplier()
	:
	fSupplierLock("video supplier lock"),
	fSupplier(NULL)
{
}


ProxyVideoSupplier::~ProxyVideoSupplier()
{
}


const media_format&
ProxyVideoSupplier::Format() const
{
	return fSupplier->Format();
}


status_t
ProxyVideoSupplier::FillBuffer(int64 startFrame, void* buffer,
	const media_raw_video_format& format, bool forceGeneration,
	bool& wasCached)
{
	bigtime_t now = system_time();

	BAutolock _(fSupplierLock);
//printf("ProxyVideoSupplier::FillBuffer(%lld)\n", startFrame);
	if (fSupplier == NULL)
		return B_NO_INIT;

	if (fSupplier->CurrentFrame() == startFrame + 1) {
		wasCached = true;
		return B_OK;
	}

	wasCached = false;
	status_t ret = B_OK;
	bigtime_t performanceTime = 0;
	if (fSupplier->CurrentFrame() != startFrame) {
		int64 frame = startFrame;
		ret = fSupplier->SeekToFrame(&frame);
		if (ret != B_OK)
			return ret;
		// Read frames until we reach the frame before the one we want to read.
		// But don't do it for more than 5 frames, or we will take too much
		// time. Doing it this way will still catch up to the next keyframe
		// eventually (we may return the wrong frames until the next keyframe).
		if (!forceGeneration && startFrame - frame > 5)
			return B_TIMED_OUT;
		while (frame < startFrame) {
			ret = fSupplier->ReadFrame(buffer, &performanceTime, format,
				wasCached);
			if (ret != B_OK)
				return ret;
			frame++;
		}
	}

	ret = fSupplier->ReadFrame(buffer, &performanceTime, format, wasCached);

	fProcessingLatency = system_time() - now;

	return ret;
}


void
ProxyVideoSupplier::DeleteCaches()
{
}


void
ProxyVideoSupplier::SetSupplier(VideoTrackSupplier* supplier)
{
	BAutolock _(fSupplierLock);

	fSupplier = supplier;
}

