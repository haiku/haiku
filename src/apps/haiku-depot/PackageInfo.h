/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <Referenceable.h>
#include <String.h>

#include "List.h"


class BBitmap;


class SharedBitmap : public BReferenceable {
public:
		enum Size {
			SIZE_ANY = -1,
			SIZE_16 = 0,
			SIZE_32 = 1,
			SIZE_64 = 2
		};

								SharedBitmap(BBitmap* bitmap);
								SharedBitmap(int32 resourceID);
								SharedBitmap(const char* mimeType);
								~SharedBitmap();

			const BBitmap*		Bitmap(Size which);

private:
			BBitmap*			_CreateBitmapFromResource(int32 size) const;
			BBitmap*			_CreateBitmapFromMimeType(int32 size) const;

private:
			int32				fResourceID;
			BString				fMimeType;
			BBitmap*			fBitmap[3];
};


typedef BReference<SharedBitmap> BitmapRef;
typedef List<BitmapRef, false> BitmapList;


class UserInfo {
public:
								UserInfo();
								UserInfo(const BString& nickName);
								UserInfo(const BitmapRef& avatar,
									const BString& nickName);
								UserInfo(const UserInfo& other);

			UserInfo&			operator=(const UserInfo& other);
			bool				operator==(const UserInfo& other) const;
			bool				operator!=(const UserInfo& other) const;

			const BitmapRef&	Avatar() const
									{ return fAvatar; }
			const BString&		NickName() const
									{ return fNickName; }

private:
			BitmapRef			fAvatar;
			BString				fNickName;
};


class UserRating {
public:
								UserRating();
								UserRating(const UserInfo& userInfo,
									float rating,
									const BString& comment,
									const BString& language,
									const BString& packageVersion,
									int32 upVotes, int32 downVotes);
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

			int32				UpVotes() const
									{ return fUpVotes; }
			int32				DownVotes() const
									{ return fDownVotes; }
private:
			UserInfo			fUserInfo;
			float				fRating;
			BString				fComment;
			BString				fLanguage;
			BString				fPackageVersion;
			int32				fUpVotes;
			int32				fDownVotes;
};


typedef List<UserRating, false> UserRatingList;


class RatingSummary {
public:
			float				averageRating;
			int					ratingCount;

			int					ratingCountByStar[5];
};


class PublisherInfo {
public:
								PublisherInfo();
								PublisherInfo(const BitmapRef& logo,
									const BString& name,
									const BString& email,
									const BString& website);
								PublisherInfo(const PublisherInfo& other);

			PublisherInfo&		operator=(const PublisherInfo& other);
			bool				operator==(const PublisherInfo& other) const;
			bool				operator!=(const PublisherInfo& other) const;

			const BitmapRef&	Logo() const
									{ return fLogo; }
			const BString&		Name() const
									{ return fName; }
			const BString&		Email() const
									{ return fEmail; }
			const BString&		Website() const
									{ return fWebsite; }

private:
			BitmapRef			fLogo;
			BString				fName;
			BString				fEmail;
			BString				fWebsite;
};


class PackageCategory : public BReferenceable {
public:
								PackageCategory();
								PackageCategory(const BitmapRef& icon,
									const BString& label,
									const BString& name);
								PackageCategory(const PackageCategory& other);

			PackageCategory&	operator=(const PackageCategory& other);
			bool				operator==(const PackageCategory& other) const;
			bool				operator!=(const PackageCategory& other) const;

			const BitmapRef&	Icon() const
									{ return fIcon; }
			const BString&		Label() const
									{ return fLabel; }
			const BString&		Name() const
									{ return fName; }
private:
			BitmapRef			fIcon;
			BString				fLabel;
			BString				fName;
};


typedef BReference<PackageCategory> CategoryRef;
typedef List<CategoryRef, false> CategoryList;


class PackageInfo {
public:
								PackageInfo();
								PackageInfo(const BitmapRef& icon,
									const BString& title,
									const BString& version,
									const PublisherInfo& publisher,
									const BString& shortDescription,
									const BString& fullDescription,
									const BString& changelog);
								PackageInfo(const PackageInfo& other);

			PackageInfo&		operator=(const PackageInfo& other);
			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			const BitmapRef&	Icon() const
									{ return fIcon; }
			const BString&		Title() const
									{ return fTitle; }
			const BString&		Version() const
									{ return fVersion; }
			const BString&		ShortDescription() const
									{ return fShortDescription; }
			const BString&		FullDescription() const
									{ return fFullDescription; }
			const PublisherInfo& Publisher() const
									{ return fPublisher; }
			const BString&		Changelog() const
									{ return fChangelog; }

			bool				AddCategory(const CategoryRef& category);
			const CategoryList&	Categories() const
									{ return fCategories; }

			bool				AddUserRating(const UserRating& rating);
			const UserRatingList& UserRatings() const
									{ return fUserRatings; }
			RatingSummary		CalculateRatingSummary() const;

			bool				AddScreenshot(const BitmapRef& screenshot);
			const BitmapList&	Screenshots() const
									{ return fScreenshots; }

private:
			BitmapRef			fIcon;
			BString				fTitle;
			BString				fVersion;
			PublisherInfo		fPublisher;
			BString				fShortDescription;
			BString				fFullDescription;
			BString				fChangelog;
			CategoryList		fCategories;
			UserRatingList		fUserRatings;
			BitmapList			fScreenshots;
};


typedef List<PackageInfo, false> PackageList;


enum PackageState {
	NONE		= 0,
	INSTALLED	= 1,
	ACTIVATED	= 2,
	UNINSTALLED	= 3,
};


class DepotInfo {
public:
								DepotInfo();
								DepotInfo(const BString& name);
								DepotInfo(const DepotInfo& other);

			DepotInfo&			operator=(const DepotInfo& other);
			bool				operator==(const DepotInfo& other) const;
			bool				operator!=(const DepotInfo& other) const;

			const BString&		Name() const
									{ return fName; }

			const PackageList&	Packages() const
									{ return fPackages; }

			bool				AddPackage(const PackageInfo& package);

private:
			BString				fName;
			PackageList			fPackages;
};


typedef List<DepotInfo, false> DepotList;


#endif // PACKAGE_INFO_H
