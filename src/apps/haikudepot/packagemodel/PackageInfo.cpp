/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageInfo.h"

#include <algorithm>

#include <package/PackageDefs.h>
#include <package/PackageFlags.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageInfoListener.h"


// #pragma mark - PackageInfo


PackageInfo::PackageInfo()
	:
	fName(),

	fCoreInfo(),
	fLocalizedText(),
	fClassificationInfo(),
	fScreenshotInfo(),
	fUserRatingInfo(),
	fLocalInfo()
{
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fName(other.fName),

	fCoreInfo(other.fCoreInfo),
	fLocalizedText(other.fLocalizedText),
	fClassificationInfo(other.fClassificationInfo),
	fScreenshotInfo(other.fScreenshotInfo),
	fUserRatingInfo(other.fUserRatingInfo),
	fLocalInfo(other.fLocalInfo)
{
}


bool
PackageInfo::operator==(const PackageInfo& other) const
{
	return fName == other.fName
		&& fCoreInfo == other.fCoreInfo
		&& fLocalInfo == other.fLocalInfo
		&& fLocalizedText == other.fLocalizedText
		&& fClassificationInfo == other.fClassificationInfo
		&& fScreenshotInfo == other.fScreenshotInfo
		&& fUserRatingInfo == fUserRatingInfo;
}


bool
PackageInfo::operator!=(const PackageInfo& other) const
{
	return !(*this == other);
}


const BString&
PackageInfo::Name() const
{
	return fName;
}


const PackageCoreInfoRef
PackageInfo::CoreInfo() const
{
	return fCoreInfo;
}


const PackageClassificationInfoRef
PackageInfo::PackageClassificationInfo() const
{
	return fClassificationInfo;
}


const PackageScreenshotInfoRef
PackageInfo::ScreenshotInfo() const
{
	return fScreenshotInfo;
}


void
PackageInfo::SetName(const BString& value)
{
	fName = value;
}


uint32
PackageInfo::ChangeMask(const PackageInfo& other) const
{
	uint32 result = 0;

	if (fLocalizedText != fLocalizedText)
		result |= PKG_CHANGED_LOCALIZED_TEXT;
	if (fScreenshotInfo != other.fScreenshotInfo)
		result |= PKG_CHANGED_SCREENSHOTS;
	if (fUserRatingInfo != other.fUserRatingInfo)
		result |= PKG_CHANGED_RATINGS;
	if (fLocalInfo != other.fLocalInfo)
		result |= PKG_CHANGED_LOCAL_INFO;
	if (fClassificationInfo != other.fClassificationInfo)
		result |= PKG_CHANGED_CLASSIFICATION;
	if (fCoreInfo != other.fCoreInfo)
		result |= PKG_CHANGED_CORE_INFO;

	return result;
}


void
PackageInfo::SetCoreInfo(PackageCoreInfoRef value)
{
	fCoreInfo = value;
}


const PackageLocalizedTextRef
PackageInfo::LocalizedText() const
{
	return fLocalizedText;
}


void
PackageInfo::SetLocalizedText(PackageLocalizedTextRef value)
{
	fLocalizedText = value;
}


const PackageUserRatingInfoRef
PackageInfo::UserRatingInfo() const
{
	return fUserRatingInfo;
}


void
PackageInfo::SetUserRatingInfo(PackageUserRatingInfoRef& value)
{
	fUserRatingInfo = value;
}


const PackageLocalInfoRef
PackageInfo::LocalInfo() const
{
	return fLocalInfo;
}


void
PackageInfo::SetLocalInfo(PackageLocalInfoRef& localInfo)
{
	fLocalInfo = localInfo;
}


void
PackageInfo::SetPackageClassificationInfo(PackageClassificationInfoRef value)
{
	fClassificationInfo = value;
}


void
PackageInfo::SetScreenshotInfo(PackageScreenshotInfoRef value)
{
	fScreenshotInfo = value;
}


// #pragma mark - PackageLocalInfoBuilder


PackageInfoBuilder::PackageInfoBuilder(const BString& name)
	:
	fName(name),
	fCoreInfo(),
	fLocalizedText(),
	fClassificationInfo(),
	fScreenshotInfo(),
	fUserRatingInfo(),
	fLocalInfo()
{
}


PackageInfoBuilder::PackageInfoBuilder(const PackageInfoRef& value)
	:
	fName(),
	fCoreInfo(),
	fLocalizedText(),
	fClassificationInfo(),
	fScreenshotInfo(),
	fUserRatingInfo(),
	fLocalInfo()
{
	if (value.IsSet())
		_Init(value.Get());
}


PackageInfoBuilder::PackageInfoBuilder(const PackageInfo& value)
{
	_Init(&value);
}


PackageInfoBuilder::~PackageInfoBuilder()
{
}


void
PackageInfoBuilder::_Init(const PackageInfo* value)
{
	fName = value->Name();
	fCoreInfo = value->CoreInfo();
	fLocalizedText = value->LocalizedText();
	fClassificationInfo = value->PackageClassificationInfo();
	fScreenshotInfo = value->ScreenshotInfo();
	fUserRatingInfo = value->UserRatingInfo();
	fLocalInfo = value->LocalInfo();
}


PackageInfoRef
PackageInfoBuilder::BuildRef()
{
	PackageInfo* info = new PackageInfo();
	info->SetName(fName);
	info->SetCoreInfo(fCoreInfo);
	info->SetLocalizedText(fLocalizedText);
	info->SetPackageClassificationInfo(fClassificationInfo);
	info->SetScreenshotInfo(fScreenshotInfo);
	info->SetUserRatingInfo(fUserRatingInfo);
	info->SetLocalInfo(fLocalInfo);
	return PackageInfoRef(info, true);
}


PackageInfoBuilder&
PackageInfoBuilder::WithCoreInfo(PackageCoreInfoRef value)
{
	fCoreInfo = value;
	return *this;
}


PackageInfoBuilder&
PackageInfoBuilder::WithLocalizedText(PackageLocalizedTextRef value)
{
	fLocalizedText = value;
	return *this;
}


PackageInfoBuilder&
PackageInfoBuilder::WithPackageClassificationInfo(PackageClassificationInfoRef value)
{
	fClassificationInfo = value;
	return *this;
}


PackageInfoBuilder&
PackageInfoBuilder::WithUserRatingInfo(PackageUserRatingInfoRef value)
{
	fUserRatingInfo = value;
	return *this;
}


PackageInfoBuilder&
PackageInfoBuilder::WithLocalInfo(PackageLocalInfoRef value)
{
	fLocalInfo = value;
	return *this;
}


PackageInfoBuilder&
PackageInfoBuilder::WithScreenshotInfo(PackageScreenshotInfoRef value)
{
	fScreenshotInfo = value;
	return *this;
}
