/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageInfo.h"

#include <algorithm>

#include <package/PackageDefs.h>
#include <package/PackageFlags.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"


// #pragma mark - PackageInfo


PackageInfo::PackageInfo()
	:
	fName(),

	fCoreInfo(),
	fLocalizedText(),
	fClassificationInfo(),
	fScreenshotInfo(),
	fUserRatingInfo(),
	fLocalInfo(),

	fListeners(),
	fIsCollatingChanges(false),
	fCollatedChanges(0)
{
}


PackageInfo::PackageInfo(const BPackageInfo& info)
	:
	fName(info.Name()),

	fClassificationInfo(),
	fScreenshotInfo(),
	fUserRatingInfo(),

	fListeners(),
	fIsCollatingChanges(false),
	fCollatedChanges(0)
{

	// TODO; factor this material out.

	BString publisherURL;
	if (info.URLList().CountStrings() > 0)
		publisherURL = info.URLList().StringAt(0);

	BString publisherName = info.Vendor();
	const BStringList& copyrightList = info.CopyrightList();
	if (!copyrightList.IsEmpty()) {
		publisherName = "";

		for (int32 i = 0; i < copyrightList.CountStrings(); i++) {
			if (!publisherName.IsEmpty())
				publisherName << ", ";
			publisherName << copyrightList.StringAt(i);
		}
	}
	if (!publisherName.IsEmpty())
		publisherName.Prepend("© ");

	fCoreInfo = PackageCoreInfoRef(new PackageCoreInfo(), true);
	fCoreInfo->SetArchitecture(info.ArchitectureName());
	fCoreInfo->SetVersion(new PackageVersion(info.Version()));
	fCoreInfo->SetPublisher(
		PackagePublisherInfoRef(new PackagePublisherInfo(publisherName, publisherURL), true));

	fLocalizedText = PackageLocalizedTextRef(new PackageLocalizedText(), true);
	fLocalizedText->SetTitle(info.Name());
	fLocalizedText->SetSummary(info.Summary());
	fLocalizedText->SetDescription(info.Description());

	// TODO: Retrieve local file size
	fLocalInfo = PackageLocalInfoRef(new PackageLocalInfo(), true);
	fLocalInfo->SetFlags(info.Flags());
	fLocalInfo->SetFileName(info.FileName());
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fName(other.fName),

	fCoreInfo(other.fCoreInfo),
	fLocalizedText(other.fLocalizedText),
	fClassificationInfo(other.fClassificationInfo),
	fScreenshotInfo(other.fScreenshotInfo),
	fUserRatingInfo(other.fUserRatingInfo),
	fLocalInfo(other.fLocalInfo),

	fListeners(),
	fIsCollatingChanges(false),
	fCollatedChanges(0)
{
}


PackageInfo&
PackageInfo::operator=(const PackageInfo& other)
{
	fName = other.fName;
	fCoreInfo = other.fCoreInfo;
	fLocalizedText = other.fLocalizedText;
	fClassificationInfo = other.fClassificationInfo;
	fScreenshotInfo = other.fScreenshotInfo;
	fUserRatingInfo = other.fUserRatingInfo;
	fLocalInfo = other.fLocalInfo;

	return *this;
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


uint32
PackageInfo::DiffMask(const PackageInfo& other) const
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
	if (value != fCoreInfo) {
		fCoreInfo = value;
		_NotifyListeners(PKG_CHANGED_CORE_INFO);
	}
}


PackageLocalizedTextRef
PackageInfo::LocalizedText() const
{
	return fLocalizedText;
}


void
PackageInfo::SetLocalizedText(PackageLocalizedTextRef value)
{
	if (fLocalizedText != value) {
		fLocalizedText = value;
		_NotifyListeners(PKG_CHANGED_LOCALIZED_TEXT);
			// TODO; separate out these later - they are bundled for now to keep the existing
			// logic working.
	}
}


UserRatingInfoRef
PackageInfo::UserRatingInfo() const
{
	return fUserRatingInfo;
}


void
PackageInfo::SetUserRatingInfo(UserRatingInfoRef value)
{
	if (fUserRatingInfo != value) {
		fUserRatingInfo = value;
		_NotifyListeners(PKG_CHANGED_RATINGS);
	}
}


PackageLocalInfoRef
PackageInfo::LocalInfo() const
{
	return fLocalInfo;
}


void
PackageInfo::SetLocalInfo(PackageLocalInfoRef& localInfo)
{
	if (fLocalInfo != localInfo) {
		fLocalInfo = localInfo;
		_NotifyListeners(PKG_CHANGED_LOCAL_INFO);
	}
}


void
PackageInfo::SetPackageClassificationInfo(PackageClassificationInfoRef value)
{
	if (value != fClassificationInfo) {
		fClassificationInfo = value;
		_NotifyListeners(PKG_CHANGED_CLASSIFICATION);
	}
}


void
PackageInfo::SetScreenshotInfo(PackageScreenshotInfoRef value)
{
	if (value != fScreenshotInfo) {
		fScreenshotInfo = value;
		_NotifyListeners(PKG_CHANGED_SCREENSHOTS);
	}
}


bool
PackageInfo::AddListener(const PackageInfoListenerRef& listener)
{
	fListeners.push_back(listener);
	return true;
}


void
PackageInfo::RemoveListener(const PackageInfoListenerRef& listener)
{
	fListeners.erase(std::remove(fListeners.begin(), fListeners.end(),
		listener), fListeners.end());
}


void
PackageInfo::NotifyChangedIcon()
{
	_NotifyListeners(PKG_CHANGED_ICON);
}


void
PackageInfo::StartCollatingChanges()
{
	fIsCollatingChanges = true;
	fCollatedChanges = 0;
}


void
PackageInfo::EndCollatingChanges()
{
	if (fIsCollatingChanges && fCollatedChanges != 0)
		_NotifyListenersImmediate(fCollatedChanges);
	fIsCollatingChanges = false;
	fCollatedChanges = 0;
}


void
PackageInfo::_NotifyListeners(uint32 changes)
{
	if (fIsCollatingChanges)
		fCollatedChanges |= changes;
	else
		_NotifyListenersImmediate(changes);
}


void
PackageInfo::_NotifyListenersImmediate(uint32 changes)
{
	if (fListeners.empty())
		return;

	// Clone list to avoid listeners detaching themselves in notifications
	// to screw up the list while iterating it.
	std::vector<PackageInfoListenerRef> listeners(fListeners);
	PackageInfoEvent event(PackageInfoRef(this), changes);

	std::vector<PackageInfoListenerRef>::iterator it;
	for (it = listeners.begin(); it != listeners.end(); it++) {
		const PackageInfoListenerRef listener = *it;
		if (listener.IsSet())
			listener->PackageChanged(event);
	}
}
