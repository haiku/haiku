/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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


int32
PackageScreenshotInfo::Count() const
{
	return fScreenshotInfos.size();
}


const ScreenshotInfoRef
PackageScreenshotInfo::ScreenshotAtIndex(int32 index) const
{
	return fScreenshotInfos[index];
}


void
PackageScreenshotInfo::AddScreenshot(const ScreenshotInfoRef& info)
{
	fScreenshotInfos.push_back(info);
}


// #pragma mark - PackageScreenshotInfoBuilder


PackageScreenshotInfoBuilder::PackageScreenshotInfoBuilder()
	:
	fSource(),
	fScreenshotInfos()
{
}


PackageScreenshotInfoBuilder::PackageScreenshotInfoBuilder(const PackageScreenshotInfoRef& other)
	:
	fScreenshotInfos()
{
	fSource = other;
}


PackageScreenshotInfoBuilder::~PackageScreenshotInfoBuilder()
{
}


void
PackageScreenshotInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
PackageScreenshotInfoBuilder::_Init(const PackageScreenshotInfo* value)
{
	int32 count = value->Count();
	for (int32 i = 0; i < count; i++)
		fScreenshotInfos.push_back(value->ScreenshotAtIndex(i));
}


PackageScreenshotInfoRef
PackageScreenshotInfoBuilder::BuildRef()
{
	if (fSource.IsSet())
		return fSource;

	PackageScreenshotInfo* screenshotInfo = new PackageScreenshotInfo();

	std::vector<ScreenshotInfoRef>::const_iterator it;
	for (it = fScreenshotInfos.begin(); it != fScreenshotInfos.end(); it++)
		screenshotInfo->AddScreenshot(*it);

	return PackageScreenshotInfoRef(screenshotInfo, true);
}


PackageScreenshotInfoBuilder&
PackageScreenshotInfoBuilder::AddScreenshot(const ScreenshotInfoRef& value)
{
	if (fSource.IsSet())
		_InitFromSource();
	fScreenshotInfos.push_back(value);
	return *this;
}
