/*
 * Copyright 2023-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "CacheScreenshotProcess.h"

#include <Catalog.h>

#include "Logger.h"
#include "Model.h"
#include "ServerHelper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "CacheScreenshotProcess"


CacheScreenshotProcess::CacheScreenshotProcess(Model *model,
	ScreenshotCoordinate& screenshotCoordinate)
	:
	fModel(model),
	fScreenshotCoordinate(screenshotCoordinate)
{
}


CacheScreenshotProcess::~CacheScreenshotProcess()
{
}


const char*
CacheScreenshotProcess::Name() const
{
	return "CacheScreenshotProcess";
}


const char*
CacheScreenshotProcess::Description() const
{
	return B_TRANSLATE("Fetching screenshot");
}


status_t
CacheScreenshotProcess::RunInternal()
{
	if (!ServerHelper::IsNetworkAvailable()) {
		HDINFO("no network so will not cache screenshot");
		return B_OK;
	}

	return fModel->GetPackageScreenshotRepository()->CacheScreenshot(fScreenshotCoordinate);
}
