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
	fSupplier(NULL),
	fCachedFrame(NULL),
	fCachedFrameSize(0),
	fCachedFrameValid(false),
	fUseFrameCaching(true)
{
}


ProxyVideoSupplier::~ProxyVideoSupplier()
{
	free(fCachedFrame);
}


status_t
ProxyVideoSupplier::FillBuffer(int64 startFrame, void* buffer,
	const media_raw_video_format& format, bool& wasCached)
{
	BAutolock _(fSupplierLock);
//printf("ProxyVideoSupplier::FillBuffer(%lld)\n", startFrame);
	if (fSupplier == NULL)
		return B_NO_INIT;

	if (fUseFrameCaching) {
		size_t bufferSize = format.display.bytes_per_row
			* format.display.line_count;
		if (fCachedFrame == NULL || fCachedFrameSize != bufferSize) {
			// realloc cached frame
			fCachedFrameValid = false;
			void* cachedFrame = realloc(fCachedFrame, bufferSize);
			if (cachedFrame != NULL) {
				fCachedFrame = cachedFrame, 
				fCachedFrameSize = bufferSize;
			} else
				fUseFrameCaching = false;
			fCachedFrameValid = false;
		}
	}

	if (fSupplier->CurrentFrame() == startFrame + 1) {
		if (fCachedFrameValid) {
			memcpy(buffer, fCachedFrame, fCachedFrameSize);
			wasCached = true;
			return B_OK;
		}
// TODO: The problem here is hidden in PlaybackManager::_PushState()
// not computing the correct current_frame for the new PlayingState.
		printf("ProxyVideoSupplier::FillBuffer(%lld) - TODO: Avoid "
			"asking for the same frame twice (%lld)!\n", startFrame,
			fSupplier->CurrentFrame());
	}

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
		if (startFrame - frame > 5)
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

	if (fUseFrameCaching && ret == B_OK) {
		memcpy(fCachedFrame, buffer, fCachedFrameSize);
		fCachedFrameValid = true;
	}

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

