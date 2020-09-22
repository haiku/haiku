/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <set>
#include <vector>

#include <Language.h>
#include <Referenceable.h>
#include <package/PackageInfo.h>

#include "List.h"
#include "PackageInfoListener.h"
#include "SharedBitmap.h"


class BPath;


/*! This class represents a language that is supported by the Haiku
    Depot Server system.  This may differ from the set of languages
    that are supported in the platform itself.
*/

class Language : public BReferenceable, public BLanguage {
public:
								Language(const char* language,
									const BString& serverName,
									bool isPopular);
								Language(const Language& other);

			status_t			GetName(BString& name,
									const BLanguage* displayLanguage = NULL
									) const;
			bool				IsPopular() const
									{ return fIsPopular; }

private:
			BString				fServerName;
			bool				fIsPopular;
};


class UserInfo {
public:
								UserInfo();
								UserInfo(const BString& nickName);
								UserInfo(const UserInfo& other);

			UserInfo&			operator=(const UserInfo& other);
			bool				operator==(const UserInfo& other) const;
			bool				operator!=(const UserInfo& other) const;

			const BString&		NickName() const
									{ return fNickName; }

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
									const BString& packageVersion,
									uint64 createTimestamp);
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
			const uint64		CreateTimestamp() const
									{ return fCreateTimestamp; }
private:
			UserInfo			fUserInfo;
			float				fRating;
			BString				fComment;
			BString				fLanguage;
			BString				fPackageVersion;
			uint64				fCreateTimestamp;
				// milliseconds since epoc
};


typedef List<UserRating, false> UserRatingList;


class RatingSummary {
public:
								RatingSummary();
								RatingSummary(const RatingSummary& other);

			RatingSummary&		operator=(const RatingSummary& other);
			bool				operator==(const RatingSummary& other) const;
			bool				operator!=(const RatingSummary& other) const;

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
								PackageCategory(const BString& code,
									const BString& name);
								PackageCategory(const PackageCategory& other);

			PackageCategory&	operator=(const PackageCategory& other);
			bool				operator==(const PackageCategory& other) const;
			bool				operator!=(const PackageCategory& other) const;

			const BString&		Code() const
									{ return fCode; }
			const BString&		Name() const
									{ return fName; }

			int					Compare(const PackageCategory& other) const;

private:
			BString				fCode;
			BString				fName;
};


typedef BReference<PackageCategory> CategoryRef;


extern bool IsPackageCategoryBefore(const CategoryRef& c1,
	const CategoryRef& c2);


class ScreenshotInfo {
public:
								ScreenshotInfo();
								ScreenshotInfo(const BString& code,
									int32 width, int32 height, int32 dataSize);
								ScreenshotInfo(const ScreenshotInfo& other);

			ScreenshotInfo&		operator=(const ScreenshotInfo& other);
			bool				operator==(const ScreenshotInfo& other) const;
			bool				operator!=(const ScreenshotInfo& other) const;

			const BString&		Code() const
									{ return fCode; }
			int32				Width() const
									{ return fWidth; }
			int32				Height() const
									{ return fHeight; }
			int32				DataSize() const
									{ return fDataSize; }

private:
			BString				fCode;
			int32				fWidth;
			int32				fHeight;
			int32				fDataSize;
};


typedef List<ScreenshotInfo, false, 2> ScreenshotInfoList;


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


using BPackageKit::BPackageInfo;
using BPackageKit::BPackageVersion;


class PackageInfo : public BReferenceable {
public:
								PackageInfo();
								PackageInfo(const BPackageInfo& info);
								PackageInfo(
									const BString& name,
									const BPackageVersion& version,
									const PublisherInfo& publisher,
									const BString& shortDescription,
									const BString& fullDescription,
									int32 packageFlags,
									const char* architecture);
								PackageInfo(const PackageInfo& other);

			PackageInfo&		operator=(const PackageInfo& other);
			bool				operator==(const PackageInfo& other) const;
			bool				operator!=(const PackageInfo& other) const;

			const BString&		Name() const
									{ return fName; }
			void				SetTitle(const BString& title);
			const BString&		Title() const;
			const BPackageVersion& Version() const
									{ return fVersion; }
			void				SetShortDescription(const BString& description);
			const BString&		ShortDescription() const
									{ return fShortDescription; }
			void				SetFullDescription(const BString& description);
			const BString&		FullDescription() const
									{ return fFullDescription; }
			const PublisherInfo& Publisher() const
									{ return fPublisher; }

			void				SetHasChangelog(bool value);
			bool				HasChangelog() const
									{ return fHasChangelog; }
			void				SetChangelog(const BString& changelog);
			const BString&		Changelog() const
									{ return fChangelog; }

			int32				Flags() const
									{ return fFlags; }
			bool				IsSystemPackage() const;

			bool				IsSystemDependency() const
									{ return fSystemDependency; }
			void				SetSystemDependency(bool isDependency);

			const BString		Architecture() const
									{ return fArchitecture; }

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

			void				SetLocalFilePath(const char* path);
			const BString&		LocalFilePath() const
									{ return fLocalFilePath; }
			bool				IsLocalFile() const;
			const BString&		FileName() const
									{ return fFileName; }

			void				ClearCategories();
			bool				AddCategory(const CategoryRef& category);
			int32				CountCategories() const;
			CategoryRef			CategoryAtIndex(int32 index) const;

			void				ClearUserRatings();
			bool				AddUserRating(const UserRating& rating);
			const UserRatingList& UserRatings() const
									{ return fUserRatings; }
			void				SetRatingSummary(const RatingSummary& summary);
			RatingSummary		CalculateRatingSummary() const;

			void				SetProminence(int64 prominence);
			int64				Prominence() const
									{ return fProminence; }
			bool				HasProminence() const
									{ return fProminence != 0; }
			bool				IsProminent() const;

			void				ClearScreenshotInfos();
			bool				AddScreenshotInfo(const ScreenshotInfo& info);
			const ScreenshotInfoList& ScreenshotInfos() const
									{ return fScreenshotInfos; }

			void				ClearScreenshots();
			bool				AddScreenshot(const BitmapRef& screenshot);
			const BitmapList&	Screenshots() const
									{ return fScreenshots; }

			void				SetSize(int64 size);
			int64				Size() const
									{ return fSize; }

			void				SetDepotName(const BString& depotName);
			const BString&		DepotName() const
									{ return fDepotName; }

			bool				AddListener(
									const PackageInfoListenerRef& listener);
			void				RemoveListener(
									const PackageInfoListenerRef& listener);

			void				StartCollatingChanges();
			void				EndCollatingChanges();
			void				NotifyChangedIcon();

private:
			void				_NotifyListeners(uint32 changes);
			void				_NotifyListenersImmediate(uint32 changes);

private:
			BString				fName;
			BString				fTitle;
			BPackageVersion		fVersion;
			PublisherInfo		fPublisher;
			BString				fShortDescription;
			BString				fFullDescription;
			bool				fHasChangelog;
			BString				fChangelog;
			std::vector<CategoryRef>
								fCategories;
			UserRatingList		fUserRatings;
			RatingSummary		fCachedRatingSummary;
			int64				fProminence;
			ScreenshotInfoList	fScreenshotInfos;
			BitmapList			fScreenshots;
			PackageState		fState;
			PackageInstallationLocationSet
								fInstallationLocations;
			float				fDownloadProgress;
			PackageListenerList	fListeners;
			int32				fFlags;
			bool				fSystemDependency;
			BString				fArchitecture;
			BString				fLocalFilePath;
			BString				fFileName;
			int64				fSize;
			BString				fDepotName;

			bool				fIsCollatingChanges;
			uint32				fCollatedChanges;
};


typedef BReference<PackageInfo> PackageInfoRef;


typedef List<PackageInfoRef, false> PackageList;


class DepotInfo : public BReferenceable {
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

			int32				PackageIndexByName(const BString& packageName)
									const;

			void				SyncPackages(const PackageList& packages);

			bool				HasAnyProminentPackages() const;

			void				SetURL(const BString& URL);
			const BString&		URL() const
									{ return fURL; }

			void				SetWebAppRepositoryCode(const BString& code);
			const BString&		WebAppRepositoryCode() const
									{ return fWebAppRepositoryCode; }

			void				SetWebAppRepositorySourceCode(
									const BString& code);
			const BString&		WebAppRepositorySourceCode() const
									{ return fWebAppRepositorySourceCode; }

private:
			BString				fName;
			PackageList			fPackages;
			BString				fWebAppRepositoryCode;
			BString				fWebAppRepositorySourceCode;
			BString				fURL;
				// this is actually a unique identifier for the repository.
};


typedef BReference<DepotInfo> DepotInfoRef;


#endif // PACKAGE_INFO_H
