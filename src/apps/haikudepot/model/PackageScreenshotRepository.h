/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SCREENSHOT_REPOSITORY_H
#define SCREENSHOT_REPOSITORY_H


#include <Bitmap.h>
#include <Path.h>
#include <String.h>

#include "BitmapHolder.h"
#include "ScreenshotCoordinate.h"
#include "WebAppInterface.h"


class Model;


class PackageScreenshotRepositoryListener : public BReferenceable {
public:
    virtual	void				ScreenshotCached(const ScreenshotCoordinate& coord) = 0;
};


typedef BReference<PackageScreenshotRepositoryListener> PackageScreenshotRepositoryListenerRef;


/*! This object manages a disk and in-memory cache of screenshots for the
    system. It will keep a few screenshots in memory, but will generally load
    them as required from local disk.
*/

class PackageScreenshotRepository {
public:

								PackageScreenshotRepository(
									PackageScreenshotRepositoryListenerRef listener,
									Model* model);
								~PackageScreenshotRepository();

			status_t			LoadScreenshot(const ScreenshotCoordinate& coord,
									BitmapHolderRef& bitmapHolderRef);
			status_t			CacheAndLoadScreenshot(const ScreenshotCoordinate& coord,
									BitmapHolderRef& bitmapHolderRef);

			status_t			HasCachedScreenshot(const ScreenshotCoordinate& coord, bool* value);
			status_t			CacheScreenshot(const ScreenshotCoordinate& coord);

private:
			WebAppInterfaceRef	_WebApp();

			status_t			_Init();

			status_t			_CleanCache();

			status_t			_DownloadToLocalFile(const ScreenshotCoordinate& coord,
									const BPath& path);
			BPath				_DeriveCachePath(const ScreenshotCoordinate& coord) const;
			status_t			_CreateCachedData(const ScreenshotCoordinate& coord,
									BPositionIO** data);

private:
			PackageScreenshotRepositoryListenerRef
								fListener;
			Model*				fModel;
			BPath				fBaseDirectory;
};


#endif // SCREENSHOT_REPOSITORY_H
