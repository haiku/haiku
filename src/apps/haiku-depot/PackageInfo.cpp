/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <IconUtils.h>
#include <Resources.h>

#include "support.h"


// #pragma mark - SharedBitmap


SharedBitmap::SharedBitmap(BBitmap* bitmap)
	:
	fResourceID(-1)
{
	fBitmap[0] = bitmap;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


SharedBitmap::SharedBitmap(int32 resourceID)
	:
	fResourceID(resourceID)
{
	fBitmap[0] = NULL;
	fBitmap[1] = NULL;
	fBitmap[2] = NULL;
}


const BBitmap*
SharedBitmap::Bitmap(Size which)
{
	if (fResourceID == -1)
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
	
	if (fBitmap[index] == NULL)
		fBitmap[index] = _CreateBitmap(size);
	
	return fBitmap[index];
}


BBitmap*
SharedBitmap::_CreateBitmap(int32 size) const
{
	BResources resources;
	status_t status = get_app_resources(resources);
	if (status != B_OK)
		return NULL;

	size_t dataSize;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, fResourceID,
		&dataSize);
	if (data == NULL)
		return NULL;

	BBitmap* bitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), 0, B_RGBA32);
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


SharedBitmap::~SharedBitmap()
{
	delete fBitmap[0];
	delete fBitmap[1];
	delete fBitmap[2];
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
	fPackageVersion()
{
}


UserRating::UserRating(const UserInfo& userInfo, float rating,
		const BString& comment, const BString& language,
		const BString& packageVersion)
	:
	fUserInfo(userInfo),
	fRating(rating),
	fComment(comment),
	fLanguage(language),
	fPackageVersion(packageVersion)
	
{
}


UserRating::UserRating(const UserRating& other)
	:
	fUserInfo(other.fUserInfo),
	fRating(other.fRating),
	fComment(other.fComment),
	fLanguage(other.fLanguage),
	fPackageVersion(other.fPackageVersion)
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
	return *this;
}


bool
UserRating::operator==(const UserRating& other) const
{
	return fUserInfo == other.fUserInfo
		&& fRating == other.fRating
		&& fComment == other.fComment
		&& fLanguage == other.fLanguage
		&& fPackageVersion == other.fPackageVersion;
}


bool
UserRating::operator!=(const UserRating& other) const
{
	return !(*this == other);
}


// #pragma mark - PackageInfo


PackageInfo::PackageInfo()
	:
	fIcon(),
	fTitle(),
	fVersion(),
	fShortDescription(),
	fFullDescription(),
	fChangelog(),
	fUserRatings()
{
}


PackageInfo::PackageInfo(const BitmapRef& icon, const BString& title,
		const BString& version, const BString& shortDescription,
		const BString& fullDescription, const BString& changelog)
	:
	fIcon(icon),
	fTitle(title),
	fVersion(version),
	fShortDescription(shortDescription),
	fFullDescription(fullDescription),
	fChangelog(changelog),
	fUserRatings()
{
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fIcon(other.fIcon),
	fTitle(other.fTitle),
	fVersion(other.fVersion),
	fShortDescription(other.fShortDescription),
	fFullDescription(other.fFullDescription),
	fChangelog(other.fChangelog),
	fUserRatings(other.fUserRatings)
{
}


PackageInfo&
PackageInfo::operator=(const PackageInfo& other)
{
	fIcon = other.fIcon;
	fTitle = other.fTitle;
	fVersion = other.fVersion;
	fShortDescription = other.fShortDescription;
	fFullDescription = other.fFullDescription;
	fChangelog = other.fChangelog;
	fUserRatings = other.fUserRatings;
	return *this;
}


bool
PackageInfo::operator==(const PackageInfo& other) const
{
	return fIcon == other.fIcon
		&& fTitle == other.fTitle
		&& fVersion == other.fVersion
		&& fShortDescription == other.fShortDescription
		&& fFullDescription == other.fFullDescription
		&& fChangelog == other.fChangelog
		&& fUserRatings == other.fUserRatings;
}


bool
PackageInfo::operator!=(const PackageInfo& other) const
{
	return !(*this == other);
}


bool
PackageInfo::AddUserRating(const UserRating& rating)
{
	return fUserRatings.Add(rating);
}


// #pragma mark -


DepotInfo::DepotInfo()
	:
	fTitle(),
	fPackages()
{
}


DepotInfo::DepotInfo(const BString& title)
	:
	fTitle(title),
	fPackages()
{
}


DepotInfo::DepotInfo(const DepotInfo& other)
	:
	fTitle(other.fTitle),
	fPackages(other.fPackages)
{
}


DepotInfo&
DepotInfo::operator=(const DepotInfo& other)
{
	fTitle = other.fTitle;
	fPackages = other.fPackages;
	return *this;
}


bool
DepotInfo::operator==(const DepotInfo& other) const
{
	return fTitle == other.fTitle
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
