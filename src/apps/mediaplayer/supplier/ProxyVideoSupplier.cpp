/*
 * Copyright © 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#include "ProxyVideoSupplier.h"

#include <stdio.h>

#include <Autolock.h>

#include "VideoTrackSupplier.h"


ProxyVideoSupplier::ProxyVideoSupplier()
	: fSupplier(NULL)
{
}


ProxyVideoSupplier::~ProxyVideoSupplier()
{
}


status_t
ProxyVideoSupplier::FillBuffer(int64 startFrame, void* buffer,
	const media_format* format, bool& wasCached)
{
//printf("ProxyVideoSupplier::FillBuffer(%lld)\n", startFrame);
	if (fSupplier == NULL)
		return B_NO_INIT;

	bigtime_t performanceTime = 0;
	if (fSupplier->CurrentFrame() != startFrame) {
		int64 frame = startFrame;
		status_t ret = fSupplier->SeekToFrame(&frame);
		if (ret != B_OK)
			return ret;
		while (frame < (startFrame - 1)) {
			ret = fSupplier->ReadFrame(buffer, &performanceTime, format,
				wasCached);
			if (ret != B_OK)
				return ret;
			frame++;
		}
	}

	// TODO: cache into intermediate buffer!

	return fSupplier->ReadFrame(buffer, &performanceTime, format, wasCached);
}


void
ProxyVideoSupplier::DeleteCaches()
{
}


void
ProxyVideoSupplier::SetSupplier(VideoTrackSupplier* supplier)
{
	fSupplier = supplier;
}

