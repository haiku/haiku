/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "CacheScreenshotProcess.h"

#include <Catalog.h>

#include "Model.h"


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
	return fModel->GetPackageScreenshotRepository()->CacheScreenshot(fScreenshotCoordinate);
}