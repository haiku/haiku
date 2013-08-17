/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <DataIO.h>
#include <IconUtils.h>
#include <MimeType.h>
#include <Resources.h>

#include "support.h"


// #pragma mark - SharedBitmap


SharedBitmap::SharedBitmap(BBitmap* bitmap)
	:
	BReferenceable(),
	fResourceID(-1),
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
	fMimeType(mimeType)
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::~SharedBitmap()
{
	delete fBitmap[0];
	delete fBitmap[1];
	delete fBitmap[2];
}


const BBitmap*
SharedBitmap::Bitmap(Size which)
{
	if (fResourceID == -1 && fMimeType.Length() == 0)
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
	if (data != NULL) {
		BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0,
			B_RGBA32);
		status = bitmap->InitCheck();
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
	
	data = resources.LoadResource(B_MESSAGE_TYPE, fResourceID, &dataSize);
	if (data != NULL) {
		BMemoryIO stream(data, dataSize);
	
		// Try to read as an archived bitmap.
		BMessage archive;
		status = archive.Unflatten(&stream);
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
	
	return NULL;
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
	fScreenshots()
{
}


PackageInfo::PackageInfo(const BitmapRef& icon, const BString& title,
		const BString& version, const PublisherInfo& publisher,
		const BString& shortDescription, const BString& fullDescription,
		const BString& changelog)
	:
	fIcon(icon),
	fTitle(title),
	fVersion(version),
	fPublisher(publisher),
	fShortDescription(shortDescription),
	fFullDescription(fullDescription),
	fChangelog(changelog),
	fCategories(),
	fUserRatings(),
	fScreenshots()
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
	fScreenshots(other.fScreenshots)
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
		&& fScreenshots == other.fScreenshots;
}


bool
PackageInfo::operator!=(const PackageInfo& other) const
{
	return !(*this == other);
}


bool
PackageInfo::AddCategory(const CategoryRef& category)
{
	return fCategories.Add(category);
}


bool
PackageInfo::AddUserRating(const UserRating& rating)
{
	return fUserRatings.Add(rating);
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
	return fScreenshots.Add(screenshot);
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
DepotInfo::AddPackage(const PackageInfo& package)
{
	return fPackages.Add(package);
}
