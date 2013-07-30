/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <stdio.h>

#include <Bitmap.h>


// #pragma mark - SharedBitmap


SharedBitmap::SharedBitmap(BBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


SharedBitmap::~SharedBitmap()
{
	delete fBitmap;
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
