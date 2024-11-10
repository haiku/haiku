/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageScreenshotInfo.h"


PackageScreenshotInfo::PackageScreenshotInfo()
{
}


PackageScreenshotInfo::PackageScreenshotInfo(const PackageScreenshotInfo& other)
	:
	fScreenshotInfos(other.fScreenshotInfos)
{
}


PackageScreenshotInfo&
PackageScreenshotInfo::operator=(const PackageScreenshotInfo& other)
{
	fScreenshotInfos = other.fScreenshotInfos;
	return *this;
}


bool
PackageScreenshotInfo::operator==(const PackageScreenshotInfo& other) const
{
	return fScreenshotInfos == other.fScreenshotInfos;
}


bool
PackageScreenshotInfo::operator!=(const PackageScreenshotInfo& other) const
{
	return !(*this == other);
}


void
PackageScreenshotInfo::Clear()
{
	fScreenshotInfos.clear();
}


int32
PackageScreenshotInfo::Count() const
{
	return fScreenshotInfos.size();
}


ScreenshotInfoRef
PackageScreenshotInfo::ScreenshotAtIndex(int32 index) const
{
	return fScreenshotInfos[index];
}


void
PackageScreenshotInfo::AddScreenshot(const ScreenshotInfoRef& info)
{
	fScreenshotInfos.push_back(info);
}