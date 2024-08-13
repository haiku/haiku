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
	fTitle(),
	fVersion(),
	fPublisher(),
	fShortDescription(),
	fFullDescription(),
	fHasChangelog(false),
	fChangelog(),
	fUserRatings(),
	fDidPopulateUserRatings(false),
	fCachedRatingSummary(),
	fProminence(0),
	fScreenshotInfos(),
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
	fTitle(),
	fVersion(info.Version()),
	fPublisher(),
	fShortDescription(info.Summary()),
	fFullDescription(info.Description()),
	fHasChangelog(false),
	fChangelog(),
	fUserRatings(),
	fDidPopulateUserRatings(false),
	fCachedRatingSummary(),
	fProminence(0),
	fScreenshotInfos(),
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
}


PackageInfo::PackageInfo(const BString& name, const BPackageVersion& version,
		const PublisherInfo& publisher, const BString& shortDescription,
		const BString& fullDescription, int32 flags, const char* architecture)
	:
	fName(name),
	fTitle(),
	fVersion(version),
	fPublisher(publisher),
	fShortDescription(shortDescription),
	fFullDescription(fullDescription),
	fHasChangelog(false),
	fChangelog(),
	fCategories(),
	fUserRatings(),
	fDidPopulateUserRatings(false),
	fCachedRatingSummary(),
	fProminence(0),
	fScreenshotInfos(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(flags),
	fSystemDependency(false),
	fArchitecture(architecture),
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


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fName(other.fName),
	fTitle(other.fTitle),
	fVersion(other.fVersion),
	fPublisher(other.fPublisher),
	fShortDescription(other.fShortDescription),
	fFullDescription(other.fFullDescription),
	fHasChangelog(other.fHasChangelog),
	fChangelog(other.fChangelog),
	fCategories(other.fCategories),
	fUserRatings(other.fUserRatings),
	fDidPopulateUserRatings(other.fDidPopulateUserRatings),
	fCachedRatingSummary(other.fCachedRatingSummary),
	fProminence(other.fProminence),
	fScreenshotInfos(other.fScreenshotInfos),
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
	fTitle = other.fTitle;
	fVersion = other.fVersion;
	fPublisher = other.fPublisher;
	fShortDescription = other.fShortDescription;
	fFullDescription = other.fFullDescription;
	fHasChangelog = other.fHasChangelog;
	fChangelog = other.fChangelog;
	fCategories = other.fCategories;
	fUserRatings = other.fUserRatings;
	fDidPopulateUserRatings = fDidPopulateUserRatings;
	fCachedRatingSummary = other.fCachedRatingSummary;
	fProminence = other.fProminence;
	fScreenshotInfos = other.fScreenshotInfos;
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
		&& fTitle == other.fTitle
		&& fVersion == other.fVersion
		&& fPublisher == other.fPublisher
		&& fShortDescription == other.fShortDescription
		&& fFullDescription == other.fFullDescription
		&& fHasChangelog == other.fHasChangelog
		&& fChangelog == other.fChangelog
		&& fCategories == other.fCategories
		&& fUserRatings == other.fUserRatings
		&& fCachedRatingSummary == other.fCachedRatingSummary
		&& fProminence == other.fProminence
		&& fScreenshotInfos == other.fScreenshotInfos
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


void
PackageInfo::SetTitle(const BString& title)
{
	if (fTitle != title) {
		fTitle = title;
		_NotifyListeners(PKG_CHANGED_TITLE);
	}
}


const BString&
PackageInfo::Title() const
{
	return fTitle.Length() > 0 ? fTitle : fName;
}


void
PackageInfo::SetShortDescription(const BString& description)
{
	if (fShortDescription != description) {
		fShortDescription = description;
		_NotifyListeners(PKG_CHANGED_SUMMARY);
	}
}


void
PackageInfo::SetFullDescription(const BString& description)
{
	if (fFullDescription != description) {
		fFullDescription = description;
		_NotifyListeners(PKG_CHANGED_DESCRIPTION);
	}
}


void
PackageInfo::SetHasChangelog(bool value)
{
	fHasChangelog = value;
}


void
PackageInfo::SetChangelog(const BString& changelog)
{
	if (fChangelog != changelog) {
		fChangelog = changelog;
		_NotifyListeners(PKG_CHANGED_CHANGELOG);
	}
}


bool
PackageInfo::IsSystemPackage() const
{
	return (fFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0;
}


int32
PackageInfo::CountCategories() const
{
	return fCategories.size();
}


CategoryRef
PackageInfo::CategoryAtIndex(int32 index) const
{
	return fCategories[index];
}


void
PackageInfo::ClearCategories()
{
	if (!fCategories.empty()) {
		fCategories.clear();
		_NotifyListeners(PKG_CHANGED_CATEGORIES);
	}
}


bool
PackageInfo::AddCategory(const CategoryRef& category)
{
	std::vector<CategoryRef>::const_iterator itInsertionPt
		= std::lower_bound(
			fCategories.begin(),
			fCategories.end(),
			category,
			&IsPackageCategoryBefore);

	if (itInsertionPt == fCategories.end()) {
		fCategories.push_back(category);
		_NotifyListeners(PKG_CHANGED_CATEGORIES);
		return true;
	}
	return false;
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


void
PackageInfo::ClearUserRatings()
{
	if (!fUserRatings.empty()) {
		fUserRatings.clear();
		_NotifyListeners(PKG_CHANGED_RATINGS);
	}
}


bool
PackageInfo::DidPopulateUserRatings() const
{
	return fDidPopulateUserRatings;
}


void
PackageInfo::SetDidPopulateUserRatings()
{
	fDidPopulateUserRatings = true;
}


int32
PackageInfo::CountUserRatings() const
{
	return fUserRatings.size();
}


UserRatingRef
PackageInfo::UserRatingAtIndex(int32 index) const
{
	return fUserRatings[index];
}


void
PackageInfo::AddUserRating(const UserRatingRef& rating)
{
	fUserRatings.push_back(rating);
	_NotifyListeners(PKG_CHANGED_RATINGS);
}


void
PackageInfo::SetRatingSummary(const RatingSummary& summary)
{
	if (fCachedRatingSummary == summary)
		return;

	fCachedRatingSummary = summary;

	_NotifyListeners(PKG_CHANGED_RATINGS);
}


RatingSummary
PackageInfo::CalculateRatingSummary() const
{
	if (fUserRatings.empty())
		return fCachedRatingSummary;

	RatingSummary summary;
	summary.ratingCount = fUserRatings.size();
	summary.averageRating = 0.0f;
	int starRatingCount = sizeof(summary.ratingCountByStar) / sizeof(int);
	for (int i = 0; i < starRatingCount; i++)
		summary.ratingCountByStar[i] = 0;

	if (summary.ratingCount <= 0)
		return summary;

	float ratingSum = 0.0f;

	int ratingsSpecified = summary.ratingCount;
	for (int i = 0; i < summary.ratingCount; i++) {
		float rating = fUserRatings[i]->Rating();

		if (rating < 0.0f)
			rating = -1.0f;
		else if (rating > 5.0f)
			rating = 5.0f;

		if (rating >= 0.0f)
			ratingSum += rating;

		if (rating <= 0.0f)
			ratingsSpecified--; // No rating specified by user
		else if (rating <= 1.0f)
			summary.ratingCountByStar[0]++;
		else if (rating <= 2.0f)
			summary.ratingCountByStar[1]++;
		else if (rating <= 3.0f)
			summary.ratingCountByStar[2]++;
		else if (rating <= 4.0f)
			summary.ratingCountByStar[3]++;
		else if (rating <= 5.0f)
			summary.ratingCountByStar[4]++;
	}

	if (ratingsSpecified > 1)
		ratingSum /= ratingsSpecified;

	summary.averageRating = ratingSum;
	summary.ratingCount = ratingsSpecified;

	return summary;
}


void
PackageInfo::SetProminence(int64 prominence)
{
	if (fProminence != prominence) {
		fProminence = prominence;
		_NotifyListeners(PKG_CHANGED_PROMINENCE);
	}
}


bool
PackageInfo::IsProminent() const
{
	return HasProminence() && Prominence() <= PROMINANCE_ORDERING_PROMINENT_MAX;
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
