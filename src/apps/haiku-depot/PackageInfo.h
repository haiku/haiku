/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <String.h>

#include "List.h"


class UserInfo {
public:
								UserInfo();
								UserInfo(const BString& nickName);
								UserInfo(const UserInfo& other);

			UserInfo&			operator=(const UserInfo& other);
			bool				operator==(const UserInfo& other) const;
			bool				operator!=(const UserInfo& other) const;

			const BString&		NickName() const;

private:
			BString				fNickName;
};


class UserRating {
public:
								UserRating();
								UserRating(const UserInfo& userInfo,
									float rating,
									const BString& comment,
									const BString& language,
									const BString& packageVersion);
								UserRating(const UserRating& other);

			UserRating&			operator=(const UserRating& other);
			bool				operator==(const UserRating& other) const;
			bool				operator!=(const UserRating& other) const;

			const UserInfo&		User() const
									{ return fUserInfo; }
			const BString&		Comment() const
									{ return fComment; }
			const BString&		Language() const
									{ return fLanguage; }
			const float			Rating() const
									{ return fRating; }
			const BString&		PackageVersion() const
									{ return fPackageVersion; }

private:
			UserInfo			fUserInfo;
			float				fRating;
			BString				fComment;
			BString				fLanguage;
			BString				fPackageVersion;
};


typedef List<UserRating, false> UserRatingList;


class PackageInfo {
public:
								PackageInfo();
								PackageInfo(const BString& title,
									const BString& version,
									const BString& shortDescription,
									const BString& fullDescription,
									const BString& changelog);
								PackageInfo(const PackageInfo& other);

			PackageInfo&		operator=(const PackageInfo& other);
			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			const BString&		Title() const
									{ return fTitle; }
			const BString&		Version() const
									{ return fVersion; }
			const BString&		ShortDescription() const
									{ return fShortDescription; }
			const BString&		FullDescription() const
									{ return fFullDescription; }
			const BString&		Changelog() const
									{ return fChangelog; }

			bool				AddUserRating(const UserRating& rating);

private:
			BString				fTitle;
			BString				fVersion;
			BString				fShortDescription;
			BString				fFullDescription;
			BString				fChangelog;
			UserRatingList		fUserRatings;
};


typedef List<PackageInfo, false> PackageInfoList;


class DepotInfo {
public:
								DepotInfo();
								DepotInfo(const BString& title);
								DepotInfo(const DepotInfo& other);

			DepotInfo&			operator=(const DepotInfo& other);
			bool				operator==(const DepotInfo& other) const;
			bool				operator!=(const DepotInfo& other) const;

			const BString&		Title() const
									{ return fTitle; }

			const PackageInfoList& PackageList() const
									{ return fPackages; }

			bool				AddPackage(const PackageInfo& package);

private:
			BString				fTitle;
			PackageInfoList		fPackages;
};


typedef List<DepotInfo, false> DepotInfoList;


#endif // PACKAGE_INFO_H
