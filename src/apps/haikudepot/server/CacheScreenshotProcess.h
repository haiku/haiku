/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef CACHE_SCREENSHOT_PROCESS__H
#define CACHE_SCREENSHOT_PROCESS__H


#include "AbstractProcess.h"
#include "PackageScreenshotRepository.h"


class Model;


class CacheScreenshotProcess : public AbstractProcess {
public:
								CacheScreenshotProcess(
									Model* model, ScreenshotCoordinate& screenshotCoordinate);
	virtual						~CacheScreenshotProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

private:
			Model*				fModel;
			ScreenshotCoordinate
								fScreenshotCoordinate;
};


#endif // CACHE_SCREENSHOT_PROCESS__H