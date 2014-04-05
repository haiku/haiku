/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <set>

#include <Referenceable.h>
#include <String.h>

#include "List.h"
#include "PackageInfoListener.h"


class BBitmap;
class BPositionIO;


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
								SharedBitmap(BPositionIO& data);
								~SharedBitmap();

			const BBitmap*		Bitmap(Size which);

private:
			BBitmap*			_CreateBitmapFromResource(int32 size) const;
			BBitmap*			_CreateBitmapFromBuffer(int32 size) const;
			BBitmap*			_CreateBitmapFromMimeType(int32 size) const;

			BBitmap*			_LoadBitmapFromBuffer(const void* buffer,
									size_t dataSize) const;
			BBitmap*			_LoadIconFromBuffer(const void* buffer,
									size_t dataSize, int32 size) const;

private:
			int32				fResourceID;
			uint8*				fBuffer;
			off_t				fSize;
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


typedef List<PackageInfoListenerRef, false, 2> PackageListenerList;
typedef std::set<int32> PackageInstallationLocationSet;

enum PackageState {
	NONE		= 0,
	INSTALLED	= 1,
	DOWNLOADING	= 2,
	ACTIVATED	= 3,
	UNINSTALLED	= 4,
	PENDING		= 5,
};


class PackageInfo : public BReferenceable {
public:
								PackageInfo();
								PackageInfo(
									const BString& title,
									const BString& version,
									const PublisherInfo& publisher,
									const BString& shortDescription,
									const BString& fullDescription,
									int32 packageFlags);
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
			const PublisherInfo& Publisher() const
									{ return fPublisher; }

			void				SetIcon(const BitmapRef& icon);
			const BitmapRef&	Icon() const
									{ return fIcon; }
			void				SetChangelog(const BString& changelog);
			const BString&		Changelog() const
									{ return fChangelog; }

			int32				Flags() const
									{ return fFlags; }
			bool				IsSystemPackage() const;

			bool				IsSystemDependency() const
									{ return fSystemDependency; }
			void				SetSystemDependency(bool isDependency);

			PackageState		State() const
									{ return fState; }
			void				SetState(PackageState state);

			const PackageInstallationLocationSet&
								InstallationLocations() const
									{ return fInstallationLocations; }
			void				AddInstallationLocation(int32 location);

			float				DownloadProgress() const
									{ return fDownloadProgress; }
			void				SetDownloadProgress(float progress);

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

			bool				AddListener(
									const PackageInfoListenerRef& listener);
			void				RemoveListener(
									const PackageInfoListenerRef& listener);

private:
			void				_NotifyListeners(uint32 changes);

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
			PackageState		fState;
			PackageInstallationLocationSet
								fInstallationLocations;
			float				fDownloadProgress;
			PackageListenerList	fListeners;
			int32				fFlags;
			bool				fSystemDependency;
};


typedef BReference<PackageInfo> PackageInfoRef;


typedef List<PackageInfoRef, false> PackageList;


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

			bool				AddPackage(const PackageInfoRef& package);

private:
			BString				fName;
			PackageList			fPackages;
};


typedef List<DepotInfo, false> DepotList;


#endif // PACKAGE_INFO_H
