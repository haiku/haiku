/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <algorithm>

#include <Collator.h>
#include <FindDirectory.h>
#include <package/PackageDefs.h>
#include <package/PackageFlags.h>
#include <Path.h>

#include "LocaleUtils.h"
#include "Logger.h"

// #pragma mark - Language


Language::Language(const char* language, const BString& serverName,
	bool isPopular)
	:
	BLanguage(language),
	fServerName(serverName),
	fIsPopular(isPopular)
{
}


Language::Language(const Language& other)
	:
	BLanguage(other.Code()),
	fServerName(other.fServerName),
	fIsPopular(other.fIsPopular)
{
}


status_t
Language::GetName(BString& name,
	const BLanguage* displayLanguage) const
{
	status_t result = BLanguage::GetName(name, displayLanguage);

	if (result == B_OK && (name.IsEmpty() || name == Code()))
		name.SetTo(fServerName);

	return result;
}


// #pragma mark - UserInfo


UserInfo::UserInfo()
	:
	fNickName()
{
}


UserInfo::UserInfo(const BString& nickName)
	:
	fNickName(nickName)
{
}


UserInfo::UserInfo(const UserInfo& other)
	:
	fNickName(other.fNickName)
{
}


UserInfo&
UserInfo::operator=(const UserInfo& other)
{
	fNickName = other.fNickName;
	return *this;
}


bool
UserInfo::operator==(const UserInfo& other) const
{
	return fNickName == other.fNickName;
}


bool
UserInfo::operator!=(const UserInfo& other) const
{
	return !(*this == other);
}


// #pragma mark - UserRating


UserRating::UserRating()
	:
	fUserInfo(),
	fRating(0.0f),
	fComment(),
	fLanguage(),
	fPackageVersion(),
	fCreateTimestamp(0)
{
}


UserRating::UserRating(const UserInfo& userInfo, float rating,
		const BString& comment, const BString& language,
		const BString& packageVersion, uint64 createTimestamp)
	:
	fUserInfo(userInfo),
	fRating(rating),
	fComment(comment),
	fLanguage(language),
	fPackageVersion(packageVersion),
	fCreateTimestamp(createTimestamp)
{
}


UserRating::UserRating(const UserRating& other)
	:
	fUserInfo(other.fUserInfo),
	fRating(other.fRating),
	fComment(other.fComment),
	fLanguage(other.fLanguage),
	fPackageVersion(other.fPackageVersion),
	fCreateTimestamp(other.fCreateTimestamp)
{
}


UserRating&
UserRating::operator=(const UserRating& other)
{
	fUserInfo = other.fUserInfo;
	fRating = other.fRating;
	fComment = other.fComment;
	fLanguage = other.fLanguage;
	fPackageVersion = other.fPackageVersion;
	fCreateTimestamp = other.fCreateTimestamp;
	return *this;
}


bool
UserRating::operator==(const UserRating& other) const
{
	return fUserInfo == other.fUserInfo
		&& fRating == other.fRating
		&& fComment == other.fComment
		&& fLanguage == other.fLanguage
		&& fPackageVersion == other.fPackageVersion
		&& fCreateTimestamp == other.fCreateTimestamp;
}


bool
UserRating::operator!=(const UserRating& other) const
{
	return !(*this == other);
}


// #pragma mark - RatingSummary


RatingSummary::RatingSummary()
	:
	averageRating(0.0f),
	ratingCount(0)
{
	for (int i = 0; i < 5; i++)
		ratingCountByStar[i] = 0;
}


RatingSummary::RatingSummary(const RatingSummary& other)
{
	*this = other;
}


RatingSummary&
RatingSummary::operator=(const RatingSummary& other)
{
	averageRating = other.averageRating;
	ratingCount = other.ratingCount;

	for (int i = 0; i < 5; i++)
		ratingCountByStar[i] = other.ratingCountByStar[i];

	return *this;
}


bool
RatingSummary::operator==(const RatingSummary& other) const
{
	if (averageRating != other.averageRating
		|| ratingCount != other.ratingCount) {
		return false;
	}

	for (int i = 0; i < 5; i++) {
		if (ratingCountByStar[i] != other.ratingCountByStar[i])
			return false;
	}

	return true;
}


bool
RatingSummary::operator!=(const RatingSummary& other) const
{
	return !(*this == other);
}


// #pragma mark - PublisherInfo


PublisherInfo::PublisherInfo()
	:
	fLogo(),
	fName(),
	fEmail(),
	fWebsite()
{
}


PublisherInfo::PublisherInfo(const BitmapRef& logo, const BString& name,
		const BString& email, const BString& website)
	:
	fLogo(logo),
	fName(name),
	fEmail(email),
	fWebsite(website)
{
}


PublisherInfo::PublisherInfo(const PublisherInfo& other)
	:
	fLogo(other.fLogo),
	fName(other.fName),
	fEmail(other.fEmail),
	fWebsite(other.fWebsite)
{
}


PublisherInfo&
PublisherInfo::operator=(const PublisherInfo& other)
{
	fLogo = other.fLogo;
	fName = other.fName;
	fEmail = other.fEmail;
	fWebsite = other.fWebsite;
	return *this;
}


bool
PublisherInfo::operator==(const PublisherInfo& other) const
{
	return fLogo == other.fLogo
		&& fName == other.fName
		&& fEmail == other.fEmail
		&& fWebsite == other.fWebsite;
}


bool
PublisherInfo::operator!=(const PublisherInfo& other) const
{
	return !(*this == other);
}


// #pragma mark - PackageCategory


PackageCategory::PackageCategory()
	:
	BReferenceable(),
	fCode(),
	fName()
{
}


PackageCategory::PackageCategory(const BString& code, const BString& name)
	:
	BReferenceable(),
	fCode(code),
	fName(name)
{
}


PackageCategory::PackageCategory(const PackageCategory& other)
	:
	BReferenceable(),
	fCode(other.fCode),
	fName(other.fName)
{
}


PackageCategory&
PackageCategory::operator=(const PackageCategory& other)
{
	fCode = other.fCode;
	fName = other.fName;
	return *this;
}


bool
PackageCategory::operator==(const PackageCategory& other) const
{
	return fCode == other.fCode && fName == other.fName;
}


bool
PackageCategory::operator!=(const PackageCategory& other) const
{
	return !(*this == other);
}


int
PackageCategory::Compare(const PackageCategory& other) const
{
	BCollator* collator = LocaleUtils::GetSharedCollator();
	int32 result = collator->Compare(Name().String(),
		other.Name().String());
	if (result == 0)
		result = Code().Compare(other.Code());
	return result;
}


bool IsPackageCategoryBefore(const CategoryRef& c1,
	const CategoryRef& c2)
{
	if (c1.Get() == NULL || c2.Get() == NULL)
		HDFATAL("unexpected NULL reference in a referencable");
	return c1.Get()->Compare(*(c2.Get())) < 0;
}


// #pragma mark - ScreenshotInfo


ScreenshotInfo::ScreenshotInfo()
	:
	fCode(),
	fWidth(),
	fHeight(),
	fDataSize()
{
}


ScreenshotInfo::ScreenshotInfo(const BString& code,
		int32 width, int32 height, int32 dataSize)
	:
	fCode(code),
	fWidth(width),
	fHeight(height),
	fDataSize(dataSize)
{
}


ScreenshotInfo::ScreenshotInfo(const ScreenshotInfo& other)
	:
	fCode(other.fCode),
	fWidth(other.fWidth),
	fHeight(other.fHeight),
	fDataSize(other.fDataSize)
{
}


ScreenshotInfo&
ScreenshotInfo::operator=(const ScreenshotInfo& other)
{
	fCode = other.fCode;
	fWidth = other.fWidth;
	fHeight = other.fHeight;
	fDataSize = other.fDataSize;
	return *this;
}


bool
ScreenshotInfo::operator==(const ScreenshotInfo& other) const
{
	return fCode == other.fCode
		&& fWidth == other.fWidth
		&& fHeight == other.fHeight
		&& fDataSize == other.fDataSize;
}


bool
ScreenshotInfo::operator!=(const ScreenshotInfo& other) const
{
	return !(*this == other);
}


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
	fCachedRatingSummary(),
	fProminence(0.0f),
	fScreenshotInfos(),
	fScreenshots(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(0),
	fSystemDependency(false),
	fArchitecture(),
	fLocalFilePath(),
	fFileName(),
	fSize(0),
	fDepotName(""),
	fIsCollatingChanges(false),
	fCollatedChanges(0)
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
	fCachedRatingSummary(),
	fProminence(0.0f),
	fScreenshotInfos(),
	fScreenshots(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(info.Flags()),
	fSystemDependency(false),
	fArchitecture(info.ArchitectureName()),
	fLocalFilePath(),
	fFileName(info.FileName()),
	fSize(0), // TODO: Retrieve local file size
	fDepotName(""),
	fIsCollatingChanges(false),
	fCollatedChanges(0)

{
	BString publisherURL;
	if (info.URLList().CountStrings() > 0)
		publisherURL = info.URLList().StringAt(0);

	BString publisherName = info.Vendor();
	const BStringList& rightsList = info.CopyrightList();
	if (rightsList.CountStrings() > 0)
		publisherName = rightsList.Last();
	if (!publisherName.IsEmpty())
		publisherName.Prepend("© ");

	fPublisher = PublisherInfo(BitmapRef(), publisherName, "", publisherURL);
}


PackageInfo::PackageInfo(const BString& name,
		const BPackageVersion& version, const PublisherInfo& publisher,
		const BString& shortDescription, const BString& fullDescription,
		int32 flags, const char* architecture)
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
	fCachedRatingSummary(),
	fProminence(0.0f),
	fScreenshotInfos(),
	fScreenshots(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(flags),
	fSystemDependency(false),
	fArchitecture(architecture),
	fLocalFilePath(),
	fFileName(),
	fSize(0),
	fDepotName(""),
	fIsCollatingChanges(false),
	fCollatedChanges(0)
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
	fCachedRatingSummary(other.fCachedRatingSummary),
	fProminence(other.fProminence),
	fScreenshotInfos(other.fScreenshotInfos),
	fScreenshots(other.fScreenshots),
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
	fIsCollatingChanges(false),
	fCollatedChanges(0)
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
	fCachedRatingSummary = other.fCachedRatingSummary;
	fProminence = other.fProminence;
	fScreenshotInfos = other.fScreenshotInfos;
	fScreenshots = other.fScreenshots;
	fState = other.fState;
	fInstallationLocations = other.fInstallationLocations;
	fDownloadProgress = other.fDownloadProgress;
	fFlags = other.fFlags;
	fSystemDependency = other.fSystemDependency;
	fArchitecture = other.fArchitecture;
	fLocalFilePath = other.fLocalFilePath;
	fFileName = other.fFileName;
	fSize = other.fSize;

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
		&& fScreenshots == other.fScreenshots
		&& fState == other.fState
		&& fFlags == other.fFlags
		&& fDownloadProgress == other.fDownloadProgress
		&& fSystemDependency == other.fSystemDependency
		&& fArchitecture == other.fArchitecture
		&& fLocalFilePath == other.fLocalFilePath
		&& fFileName == other.fFileName
		&& fSize == other.fSize;
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
	if (!fUserRatings.IsEmpty()) {
		fUserRatings.Clear();
		_NotifyListeners(PKG_CHANGED_RATINGS);
	}
}


bool
PackageInfo::AddUserRating(const UserRating& rating)
{
	if (!fUserRatings.Add(rating))
		return false;

	_NotifyListeners(PKG_CHANGED_RATINGS);

	return true;
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
	if (fUserRatings.CountItems() == 0)
		return fCachedRatingSummary;

	RatingSummary summary;
	summary.ratingCount = fUserRatings.CountItems();
	summary.averageRating = 0.0f;
	int starRatingCount = sizeof(summary.ratingCountByStar) / sizeof(int);
	for (int i = 0; i < starRatingCount; i++)
		summary.ratingCountByStar[i] = 0;

	if (summary.ratingCount <= 0)
		return summary;

	float ratingSum = 0.0f;

	int ratingsSpecified = summary.ratingCount;
	for (int i = 0; i < summary.ratingCount; i++) {
		float rating = fUserRatings.ItemAtFast(i).Rating();

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
	fScreenshotInfos.Clear();
}


bool
PackageInfo::AddScreenshotInfo(const ScreenshotInfo& info)
{
	return fScreenshotInfos.Add(info);
}


void
PackageInfo::ClearScreenshots()
{
	if (!fScreenshots.IsEmpty()) {
		fScreenshots.Clear();
		_NotifyListeners(PKG_CHANGED_SCREENSHOTS);
	}
}


bool
PackageInfo::AddScreenshot(const BitmapRef& screenshot)
{
	if (!fScreenshots.Add(screenshot))
		return false;

	_NotifyListeners(PKG_CHANGED_SCREENSHOTS);

	return true;
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
	return fListeners.Add(listener);
}


void
PackageInfo::RemoveListener(const PackageInfoListenerRef& listener)
{
	fListeners.Remove(listener);
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
	int count = fListeners.CountItems();
	if (count == 0)
		return;

	// Clone list to avoid listeners detaching themselves in notifications
	// to screw up the list while iterating it.
	PackageListenerList listeners(fListeners);
	// Check if it worked:
	if (listeners.CountItems() != count)
		return;

	PackageInfoEvent event(PackageInfoRef(this), changes);

	for (int i = 0; i < count; i++) {
		const PackageInfoListenerRef& listener = listeners.ItemAtFast(i);
		if (listener.Get() != NULL)
			listener->PackageChanged(event);
	}
}


// #pragma mark - Sorting Functions


/*! This function is used with the List class in order to facilitate fast
    ordered inserting of packages.
 */

static int32
PackageCompare(const PackageInfoRef& p1, const PackageInfoRef& p2)
{
	return p1->Name().Compare(p2->Name());
}


/*! This function is used with the List class in order to facilitate fast
    searching of packages.
 */

static int32
PackageFixedNameCompare(const void* context,
	const PackageInfoRef& package)
{
	const BString* packageName = static_cast<const BString*>(context);
	return packageName->Compare(package->Name());
}


// #pragma mark -


DepotInfo::DepotInfo()
	:
	fName(),
	fPackages(&PackageCompare, &PackageFixedNameCompare),
	fWebAppRepositoryCode()
{
}


DepotInfo::DepotInfo(const BString& name)
	:
	fName(name),
	fPackages(&PackageCompare, &PackageFixedNameCompare),
	fWebAppRepositoryCode(),
	fWebAppRepositorySourceCode()
{
}


DepotInfo::DepotInfo(const DepotInfo& other)
	:
	fName(other.fName),
	fPackages(other.fPackages),
	fWebAppRepositoryCode(other.fWebAppRepositoryCode),
	fWebAppRepositorySourceCode(other.fWebAppRepositorySourceCode),
	fURL(other.fURL)
{
}


DepotInfo&
DepotInfo::operator=(const DepotInfo& other)
{
	fName = other.fName;
	fPackages = other.fPackages;
	fURL = other.fURL;
	fWebAppRepositoryCode = other.fWebAppRepositoryCode;
	fWebAppRepositorySourceCode = other.fWebAppRepositorySourceCode;
	return *this;
}


bool
DepotInfo::operator==(const DepotInfo& other) const
{
	return fName == other.fName
		&& fPackages == other.fPackages;
}


bool
DepotInfo::operator!=(const DepotInfo& other) const
{
	return !(*this == other);
}


/*! This method will insert the package into the list of packages
    in order so that the list of packages remains in order.
 */

bool
DepotInfo::AddPackage(const PackageInfoRef& package)
{
 	return fPackages.Add(package);
}


int32
DepotInfo::PackageIndexByName(const BString& packageName) const
{
	return fPackages.Search(&packageName);
}


void
DepotInfo::SyncPackages(const PackageList& otherPackages)
{
	PackageList packages(fPackages);

	for (int32 i = otherPackages.CountItems() - 1; i >= 0; i--) {
		const PackageInfoRef& otherPackage = otherPackages.ItemAtFast(i);
		bool found = false;
		for (int32 j = packages.CountItems() - 1; j >= 0; j--) {
			const PackageInfoRef& package = packages.ItemAtFast(j);
			if (package->Name() == otherPackage->Name()) {
				package->SetState(otherPackage->State());
				package->SetLocalFilePath(otherPackage->LocalFilePath());
				package->SetSystemDependency(
					otherPackage->IsSystemDependency());
				found = true;
				packages.Remove(j);
				break;
			}
		}
		if (!found) {
			HDINFO("%s: new package: '%s'", fName.String(),
				otherPackage->Name().String());
			fPackages.Add(otherPackage);
		}
	}

	for (int32 i = packages.CountItems() - 1; i >= 0; i--) {
		const PackageInfoRef& package = packages.ItemAtFast(i);
		HDINFO("%s: removing package: '%s'", fName.String(),
			package->Name().String());
		fPackages.Remove(package);
	}
}


bool
DepotInfo::HasAnyProminentPackages() const
{
	int32 count = fPackages.CountItems();
	for (int32 i = 0; i < count; i++) {
		const PackageInfoRef& package = fPackages.ItemAtFast(i);
		if (package->IsProminent())
			return true;
	}
	return false;
}


void
DepotInfo::SetURL(const BString& URL)
{
	fURL = URL;
}


void
DepotInfo::SetWebAppRepositoryCode(const BString& code)
{
	fWebAppRepositoryCode = code;
}


void
DepotInfo::SetWebAppRepositorySourceCode(const BString& code)
{
	fWebAppRepositorySourceCode = code;
}
