/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <IconUtils.h>
#include <MimeType.h>
#include <package/PackageFlags.h>
#include <Resources.h>

#include "support.h"


// #pragma mark - SharedBitmap


SharedBitmap::SharedBitmap(BBitmap* bitmap)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	fBitmap[0] = bitmap;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::SharedBitmap(int32 resourceID)
	:
	BReferenceable(),
	fResourceID(resourceID),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::SharedBitmap(const char* mimeType)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType(mimeType)
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::SharedBitmap(BPositionIO& data)
	:
	BReferenceable(),
	fResourceID(-1),
	fBuffer(NULL),
	fSize(0),
	fMimeType()
{
	status_t status = data.GetSize(&fSize);
	if (status == B_OK && fSize > 0 && fSize <= 64 * 1024) {
		fBuffer = new(std::nothrow) uint8[fSize];

		data.Seek(0, SEEK_SET);
		ssize_t read = data.Read(fBuffer, fSize);
		if (read != fSize) {
			delete[] fBuffer;
			fBuffer = NULL;
			fSize = 0;
		}
	}

	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::~SharedBitmap()
{
	delete fBitmap[0];
	delete fBitmap[1];
	delete fBitmap[2];
	delete[] fBuffer;
}


const BBitmap*
SharedBitmap::Bitmap(Size which)
{
	if (fResourceID == -1 && fMimeType.Length() == 0 && fBuffer == NULL)
		return fBitmap[0];

	int32 index = 0;
	int32 size = 16;

	switch (which) {
		default:
		case SIZE_16:
			break;

		case SIZE_32:
			index = 1;
			size = 32;
			break;
		case SIZE_64:
			index = 2;
			size = 64;
			break;
	}

	if (fBitmap[index] == NULL) {
		if (fResourceID >= 0)
			fBitmap[index] = _CreateBitmapFromResource(size);
		else if (fBuffer != NULL)
			fBitmap[index] = _CreateBitmapFromBuffer(size);
		else if (fMimeType.Length() > 0)
			fBitmap[index] = _CreateBitmapFromMimeType(size);
	}

	return fBitmap[index];
}


BBitmap*
SharedBitmap::_CreateBitmapFromResource(int32 size) const
{
	BResources resources;
	status_t status = get_app_resources(resources);
	if (status != B_OK)
		return NULL;

	size_t dataSize;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, fResourceID,
		&dataSize);
	if (data != NULL)
		return _LoadIconFromBuffer(data, dataSize, size);

	data = resources.LoadResource(B_MESSAGE_TYPE, fResourceID, &dataSize);
	if (data != NULL)
		return _LoadBitmapFromBuffer(data, dataSize);

	return NULL;
}


BBitmap*
SharedBitmap::_CreateBitmapFromBuffer(int32 size) const
{
	BBitmap* bitmap = _LoadIconFromBuffer(fBuffer, fSize, size);
	
	if (bitmap == NULL)
		bitmap = _LoadBitmapFromBuffer(fBuffer, fSize);

	return bitmap;
}


BBitmap*
SharedBitmap::_CreateBitmapFromMimeType(int32 size) const
{
	BMimeType mimeType(fMimeType.String());
	status_t status = mimeType.InitCheck();
	if (status != B_OK)
		return NULL;

	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0, B_RGBA32);
	status = bitmap->InitCheck();
	if (status == B_OK)
		status = mimeType.GetIcon(bitmap, B_MINI_ICON);

	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


BBitmap*
SharedBitmap::_LoadBitmapFromBuffer(const void* buffer, size_t size) const
{
	BMemoryIO stream(buffer, size);

	// Try to read as an archived bitmap.
	BMessage archive;
	status_t status = archive.Unflatten(&stream);
	if (status != B_OK)
		return NULL;

	BBitmap* bitmap = new BBitmap(&archive);

	status = bitmap->InitCheck();
	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


BBitmap*
SharedBitmap::_LoadIconFromBuffer(const void* data, size_t dataSize,
	int32 size) const
{
	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0,
		B_RGBA32);
	status_t status = bitmap->InitCheck();
	if (status == B_OK) {
		status = BIconUtils::GetVectorIcon(
			reinterpret_cast<const uint8*>(data), dataSize, bitmap);
	};

	if (status != B_OK) {
		delete bitmap;
		bitmap = NULL;
	}

	return bitmap;
}


// #pragma mark - UserInfo


UserInfo::UserInfo()
	:
	fAvatar(),
	fNickName()
{
}


UserInfo::UserInfo(const BString& nickName)
	:
	fAvatar(),
	fNickName(nickName)
{
}


UserInfo::UserInfo(const BitmapRef& avatar, const BString& nickName)
	:
	fAvatar(avatar),
	fNickName(nickName)
{
}


UserInfo::UserInfo(const UserInfo& other)
	:
	fAvatar(other.fAvatar),
	fNickName(other.fNickName)
{
}


UserInfo&
UserInfo::operator=(const UserInfo& other)
{
	fAvatar = other.fAvatar;
	fNickName = other.fNickName;
	return *this;
}


bool
UserInfo::operator==(const UserInfo& other) const
{
	return fAvatar == other.fAvatar
		&& fNickName == other.fNickName;
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
	fUpVotes(0),
	fDownVotes(0)
{
}


UserRating::UserRating(const UserInfo& userInfo, float rating,
		const BString& comment, const BString& language,
		const BString& packageVersion, int32 upVotes, int32 downVotes)
	:
	fUserInfo(userInfo),
	fRating(rating),
	fComment(comment),
	fLanguage(language),
	fPackageVersion(packageVersion),
	fUpVotes(upVotes),
	fDownVotes(downVotes)
{
}


UserRating::UserRating(const UserRating& other)
	:
	fUserInfo(other.fUserInfo),
	fRating(other.fRating),
	fComment(other.fComment),
	fLanguage(other.fLanguage),
	fPackageVersion(other.fPackageVersion),
	fUpVotes(other.fUpVotes),
	fDownVotes(other.fDownVotes)
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
	fUpVotes = other.fUpVotes;
	fDownVotes = other.fDownVotes;
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
		&& fUpVotes == other.fUpVotes
		&& fDownVotes == other.fDownVotes;
}


bool
UserRating::operator!=(const UserRating& other) const
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
	fIcon(),
	fName()
{
}


PackageCategory::PackageCategory(const BitmapRef& icon, const BString& label,
		const BString& name)
	:
	BReferenceable(),
	fIcon(icon),
	fLabel(label),
	fName(name)
{
}


PackageCategory::PackageCategory(const PackageCategory& other)
	:
	BReferenceable(),
	fIcon(other.fIcon),
	fLabel(other.fLabel),
	fName(other.fName)
{
}


PackageCategory&
PackageCategory::operator=(const PackageCategory& other)
{
	fIcon = other.fIcon;
	fName = other.fName;
	fLabel = other.fLabel;
	return *this;
}


bool
PackageCategory::operator==(const PackageCategory& other) const
{
	return fIcon == other.fIcon
		&& fLabel == other.fLabel
		&& fName == other.fName;
}


bool
PackageCategory::operator!=(const PackageCategory& other) const
{
	return !(*this == other);
}


// #pragma mark - PackageInfo


PackageInfo::PackageInfo()
	:
	fIcon(),
	fTitle(),
	fVersion(),
	fPublisher(),
	fShortDescription(),
	fFullDescription(),
	fChangelog(),
	fUserRatings(),
	fScreenshots(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(0),
	fSystemDependency(false)
{
}


PackageInfo::PackageInfo(const BString& title,
		const BString& version, const PublisherInfo& publisher,
		const BString& shortDescription, const BString& fullDescription,
		int32 flags)
	:
	fIcon(),
	fTitle(title),
	fVersion(version),
	fPublisher(publisher),
	fShortDescription(shortDescription),
	fFullDescription(fullDescription),
	fChangelog(),
	fCategories(),
	fUserRatings(),
	fScreenshots(),
	fState(NONE),
	fDownloadProgress(0.0),
	fFlags(flags),
	fSystemDependency(false)
{
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fIcon(other.fIcon),
	fTitle(other.fTitle),
	fVersion(other.fVersion),
	fPublisher(other.fPublisher),
	fShortDescription(other.fShortDescription),
	fFullDescription(other.fFullDescription),
	fChangelog(other.fChangelog),
	fCategories(other.fCategories),
	fUserRatings(other.fUserRatings),
	fScreenshots(other.fScreenshots),
	fState(other.fState),
	fInstallationLocations(other.fInstallationLocations),
	fDownloadProgress(other.fDownloadProgress),
	fFlags(other.fFlags),
	fSystemDependency(other.fSystemDependency)
{
}


PackageInfo&
PackageInfo::operator=(const PackageInfo& other)
{
	fIcon = other.fIcon;
	fTitle = other.fTitle;
	fVersion = other.fVersion;
	fPublisher = other.fPublisher;
	fShortDescription = other.fShortDescription;
	fFullDescription = other.fFullDescription;
	fChangelog = other.fChangelog;
	fCategories = other.fCategories;
	fUserRatings = other.fUserRatings;
	fScreenshots = other.fScreenshots;
	fState = other.fState;
	fInstallationLocations = other.fInstallationLocations;
	fDownloadProgress = other.fDownloadProgress;
	fFlags = other.fFlags;
	fSystemDependency = other.fSystemDependency;
	return *this;
}


bool
PackageInfo::operator==(const PackageInfo& other) const
{
	return fIcon == other.fIcon
		&& fTitle == other.fTitle
		&& fVersion == other.fVersion
		&& fPublisher == other.fPublisher
		&& fShortDescription == other.fShortDescription
		&& fFullDescription == other.fFullDescription
		&& fChangelog == other.fChangelog
		&& fCategories == other.fCategories
		&& fUserRatings == other.fUserRatings
		&& fScreenshots == other.fScreenshots
		&& fState == other.fState
		&& fFlags == other.fFlags
		&& fDownloadProgress == other.fDownloadProgress
		&& fSystemDependency == other.fSystemDependency;
}


bool
PackageInfo::operator!=(const PackageInfo& other) const
{
	return !(*this == other);
}


void
PackageInfo::SetIcon(const BitmapRef& icon)
{
	if (fIcon != icon) {
		fIcon = icon;
		_NotifyListeners(PKG_CHANGED_ICON);
	}
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


bool
PackageInfo::AddCategory(const CategoryRef& category)
{
	if (fCategories.Add(category)) {
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


bool
PackageInfo::AddUserRating(const UserRating& rating)
{
	if (!fUserRatings.Add(rating))
		return false;

	_NotifyListeners(PKG_CHANGED_RATINGS);

	return true;
}


RatingSummary
PackageInfo::CalculateRatingSummary() const
{
	RatingSummary summary;
	summary.ratingCount = fUserRatings.CountItems();
	summary.averageRating = 0.0f;
	int starRatingCount = sizeof(summary.ratingCountByStar) / sizeof(int);
	for (int i = 0; i < starRatingCount; i++)
		summary.ratingCountByStar[i] = 0;

	if (summary.ratingCount <= 0)
		return summary;

	float ratingSum = 0.0f;

	for (int i = 0; i < summary.ratingCount; i++) {
		float rating = fUserRatings.ItemAtFast(i).Rating();

		if (rating < 0.0f)
			rating = 0.0f;
		else if (rating > 5.0f)
			rating = 5.0f;

		ratingSum += rating;

		if (rating <= 1.0f)
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

	summary.averageRating = ratingSum / summary.ratingCount;
	return summary;
}


bool
PackageInfo::AddScreenshot(const BitmapRef& screenshot)
{
	if (!fScreenshots.Add(screenshot))
		return false;

	_NotifyListeners(PKG_CHANGED_SCREENSHOTS);

	return true;
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
PackageInfo::_NotifyListeners(uint32 changes)
{
	int count = fListeners.CountItems();
	if (count == 0)
		return;

	// Clone list to avoid listeners detaching themselves in notifications
	// to screw up the list while iterating it.
	PackageListenerList listeners(fListeners);
	// Check if it worked:
	if (listeners.CountItems () != count)
		return;

	PackageInfoEvent event(PackageInfoRef(this), changes);

	for (int i = 0; i < count; i++) {
		const PackageInfoListenerRef& listener = listeners.ItemAtFast(i);
		if (listener.Get() != NULL)
			listener->PackageChanged(event);
	}
}


// #pragma mark -


DepotInfo::DepotInfo()
	:
	fName(),
	fPackages()
{
}


DepotInfo::DepotInfo(const BString& name)
	:
	fName(name),
	fPackages()
{
}


DepotInfo::DepotInfo(const DepotInfo& other)
	:
	fName(other.fName),
	fPackages(other.fPackages)
{
}


DepotInfo&
DepotInfo::operator=(const DepotInfo& other)
{
	fName = other.fName;
	fPackages = other.fPackages;
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


bool
DepotInfo::AddPackage(const PackageInfoRef& package)
{
	return fPackages.Add(package);
}
