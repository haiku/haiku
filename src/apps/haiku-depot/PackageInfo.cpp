/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "PackageInfo.h"

#include <stdio.h>


// #pragma mark - UserInfo


UserInfo::UserInfo()
	:
	fNickName()
{
}


UserInfo::UserInfo(const BString& fNickName)
	:
	fNickName(fNickName)
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
	fComment(),
	fRating(0.0f),
	fPackageVersion()
{
}


UserRating::UserRating(const UserInfo& userInfo, const BString& comment,
		float rating, const BString& packageVersion)
	:
	fUserInfo(userInfo),
	fComment(comment),
	fRating(rating),
	fPackageVersion(packageVersion)
	
{
}


UserRating::UserRating(const UserRating& other)
	:
	fUserInfo(other.fUserInfo),
	fComment(other.fComment),
	fRating(other.fRating),
	fPackageVersion(other.fPackageVersion)
{
}


UserRating&
UserRating::operator=(const UserRating& other)
{
	fUserInfo = other.fUserInfo;
	fComment = other.fComment;
	fRating = other.fRating;
	fPackageVersion = other.fPackageVersion;
	return *this;
}


bool
UserRating::operator==(const UserRating& other) const
{
	return fUserInfo == other.fUserInfo
		&& fComment == other.fComment
		&& fRating == other.fRating
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
	fTitle(),
	fVersion(),
	fDescription(),
	fChangelog(),
	fUserRatings()
{
}


PackageInfo::PackageInfo(const BString& title, const BString& version,
		const BString& description, const BString& changelog)
	:
	fTitle(title),
	fVersion(version),
	fDescription(description),
	fChangelog(changelog),
	fUserRatings()
{
}


PackageInfo::PackageInfo(const PackageInfo& other)
	:
	fTitle(other.fTitle),
	fVersion(other.fVersion),
	fDescription(other.fDescription),
	fChangelog(other.fChangelog),
	fUserRatings(other.fUserRatings)
{
}


PackageInfo&
PackageInfo::operator=(const PackageInfo& other)
{
	fTitle = other.fTitle;
	fVersion = other.fVersion;
	fDescription = other.fDescription;
	fChangelog = other.fChangelog;
	fUserRatings = other.fUserRatings;
	return *this;
}


bool
PackageInfo::operator==(const PackageInfo& other) const
{
	return fTitle == other.fTitle
		&& fVersion == other.fVersion
		&& fDescription == other.fDescription
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
