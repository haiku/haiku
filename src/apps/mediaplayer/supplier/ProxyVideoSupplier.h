/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT licensce.
 */
#ifndef PROXY_VIDEO_SUPPLIER_H
#define PROXY_VIDEO_SUPPLIER_H

#include <Locker.h>

#include "VideoSupplier.h"


class VideoTrackSupplier;


class ProxyVideoSupplier : public VideoSupplier {
public:
								ProxyVideoSupplier();
	virtual						~ProxyVideoSupplier();

	virtual	const media_format&	Format() const;
	virtual	status_t			FillBuffer(int64 startFrame, void* buffer,
									const media_raw_video_format& format,
									bool forceGeneration, bool& wasCached);

	virtual	void				DeleteCaches();

			void				SetSupplier(VideoTrackSupplier* supplier);

private:
			BLocker				fSupplierLock;

			VideoTrackSupplier*	fSupplier;
};

#endif	// PROXY_VIDEO_SUPPLIER_H
