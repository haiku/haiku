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
	fVersion(),
	fPublisher(),
	fLocalizedText(),
	fPackageClassificationInfo(),
	fScreenshotInfos(),
	fUserRatingInfo(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(0),
	fSystemDependency(false),
	fArchitecture(),
	fLocalFilePath(),
	fFileName(),
	fSize(0),
	fDepotName(""),
	fViewed(false),
	fIsCollatingChanges(false),
	fCollatedChanges(0),
	fVersionCreateTimestamp(0)
{
}


PackageInfo::PackageInfo(const BPackageInfo& info)
	:
	fName(info.Name()),
	fVersion(info.Version()),
	fPublisher(),
	fPackageClassificationInfo(),
	fScreenshotInfos(),
	fUserRatingInfo(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(info.Flags()),
	fSystemDependency(false),
	fArchitecture(info.ArchitectureName()),
	fLocalFilePath(),
	fFileName(info.FileName()),
	fSize(0), // TODO: Retrieve local file size
	fDepotName(""),
	fViewed(false),
	fIsCollatingChanges(false),
	fCollatedChanges(0),
	fVersionCreateTimestamp(0)
{
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

	fPublisher = PublisherInfo(publisherName, publisherURL);

	fLocalizedText = PackageLocalizedTextRef(new PackageLocalizedText(), true);
	fLocalizedText->SetTitle(info.Name());
	fLocalizedText->SetSummary(info.Summary());
	fLocalizedText->SetDescription(info.Description());
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fName(other.fName),
	fVersion(other.fVersion),
	fPublisher(other.fPublisher),
	fLocalizedText(other.fLocalizedText),
	fPackageClassificationInfo(other.fPackageClassificationInfo),
	fScreenshotInfos(other.fScreenshotInfos),
	fUserRatingInfo(other.fUserRatingInfo),
	fState(other.fState),
	fInstallationLocations(other.fInstallationLocations),
	fDownloadProgress(other.fDownloadProgress),
	fFlags(other.fFlags),
	fSystemDependency(other.fSystemDependency),
	fArchitecture(other.fArchitecture),
	fLocalFilePath(other.fLocalFilePath),
	fFileName(other.fFileName),
	fSize(other.fSize),
	fDepotName(other.fDepotName),
	fViewed(other.fViewed),
	fIsCollatingChanges(false),
	fCollatedChanges(0),
	fVersionCreateTimestamp(other.fVersionCreateTimestamp)
{
}


PackageInfo&
PackageInfo::operator=(const PackageInfo& other)
{
	fName = other.fName;
	fVersion = other.fVersion;
	fPublisher = other.fPublisher;
	fLocalizedText = other.fLocalizedText;
	fPackageClassificationInfo = other.fPackageClassificationInfo;
	fScreenshotInfos = other.fScreenshotInfos;
	fUserRatingInfo = other.fUserRatingInfo;
	fState = other.fState;
	fInstallationLocations = other.fInstallationLocations;
	fDownloadProgress = other.fDownloadProgress;
	fFlags = other.fFlags;
	fSystemDependency = other.fSystemDependency;
	fArchitecture = other.fArchitecture;
	fLocalFilePath = other.fLocalFilePath;
	fFileName = other.fFileName;
	fSize = other.fSize;
	fDepotName = other.fDepotName;
	fViewed = other.fViewed;
	fVersionCreateTimestamp = other.fVersionCreateTimestamp;

	return *this;
}


bool
PackageInfo::operator==(const PackageInfo& other) const
{
	return fName == other.fName
		&& fVersion == other.fVersion
		&& fPublisher == other.fPublisher
		&& fLocalizedText == other.fLocalizedText
		&& fPackageClassificationInfo == other.fPackageClassificationInfo
		&& fScreenshotInfos == other.fScreenshotInfos
		&& fUserRatingInfo == fUserRatingInfo
		&& fState == other.fState
		&& fFlags == other.fFlags
		&& fDownloadProgress == other.fDownloadProgress
		&& fSystemDependency == other.fSystemDependency
		&& fArchitecture == other.fArchitecture
		&& fLocalFilePath == other.fLocalFilePath
		&& fFileName == other.fFileName
		&& fSize == other.fSize
		&& fVersionCreateTimestamp == other.fVersionCreateTimestamp;
}


bool
PackageInfo::operator!=(const PackageInfo& other) const
{
	return !(*this == other);
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
		_NotifyListeners(PKG_CHANGED_CHANGELOG);
			// TODO; separate out these later - they are bundled for now to keep the existing
			// logic working.
	}
}


bool
PackageInfo::IsSystemPackage() const
{
	return (fFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0;
}


void
PackageInfo::SetPackageClassificationInfo(PackageClassificationInfoRef value)
{
	if (value != fPackageClassificationInfo) {
		fPackageClassificationInfo = value;
		_NotifyListeners(PKG_CHANGED_CLASSIFICATION);
	}
}


void
PackageInfo::SetSystemDependency(bool isDependency)
{
	fSystemDependency = isDependency;
}


void
PackageInfo::SetState(PackageState state)
{
	if (fState != state) {
		fState = state;
		if (fState != DOWNLOADING)
			fDownloadProgress = 0.0;
		_NotifyListeners(PKG_CHANGED_STATE);
	}
}


void
PackageInfo::AddInstallationLocation(int32 location)
{
	fInstallationLocations.insert(location);
	SetState(ACTIVATED);
		// TODO: determine how to differentiate between installed and active.
}


void
PackageInfo::ClearInstallationLocations()
{
	fInstallationLocations.clear();
}


void
PackageInfo::SetDownloadProgress(float progress)
{
	fState = DOWNLOADING;
	fDownloadProgress = progress;
	_NotifyListeners(PKG_CHANGED_STATE);
}


void
PackageInfo::SetLocalFilePath(const char* path)
{
	fLocalFilePath = path;
}


bool
PackageInfo::IsLocalFile() const
{
	return !fLocalFilePath.IsEmpty() && fInstallationLocations.empty();
}


UserRatingInfoRef
PackageInfo::UserRatingInfo()
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


void
PackageInfo::ClearScreenshotInfos()
{
	if (!fScreenshotInfos.empty()) {
		fScreenshotInfos.clear();
		_NotifyListeners(PKG_CHANGED_SCREENSHOTS);
	}
}


int32
PackageInfo::CountScreenshotInfos() const
{
	return fScreenshotInfos.size();
}


ScreenshotInfoRef
PackageInfo::ScreenshotInfoAtIndex(int32 index) const
{
	return fScreenshotInfos[index];
}


void
PackageInfo::AddScreenshotInfo(const ScreenshotInfoRef& info)
{
	fScreenshotInfos.push_back(info);
	_NotifyListeners(PKG_CHANGED_SCREENSHOTS);
}


void
PackageInfo::SetSize(int64 size)
{
	if (fSize != size) {
		fSize = size;
		_NotifyListeners(PKG_CHANGED_SIZE);
	}
}


void
PackageInfo::SetViewed()
{
	fViewed = true;
}


void
PackageInfo::SetVersionCreateTimestamp(uint64 value)
{
	if (fVersionCreateTimestamp != value) {
		fVersionCreateTimestamp = value;
		_NotifyListeners(PKG_CHANGED_VERSION_CREATE_TIMESTAMP);
	}
}


void
PackageInfo::SetDepotName(const BString& depotName)
{
	if (fDepotName != depotName) {
		fDepotName = depotName;
		_NotifyListeners(PKG_CHANGED_DEPOT);
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


const char* package_state_to_string(PackageState state)
{
	switch (state) {
		case NONE:
			return "NONE";
		case INSTALLED:
			return "INSTALLED";
		case DOWNLOADING:
			return "DOWNLOADING";
		case ACTIVATED:
			return "ACTIVATED";
		case UNINSTALLED:
			return "UNINSTALLED";
		case PENDING:
			return "PENDING";
		default:
			debugger("unknown package state");
			return "???";
	}
}
