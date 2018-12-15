/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <set>

#include <Referenceable.h>
#include <package/PackageInfo.h>

#include "DateTime.h"
#include "List.h"
#include "PackageInfoListener.h"
#include "SharedBitmap.h"


class BPath;


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
									int32 upVotes, int32 downVotes,
									const BDateTime& createTimestamp);
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
			const BDateTime&	CreateTimestamp() const
									{ return fCreateTimestamp; }
private:
			UserInfo			fUserInfo;
			float				fRating;
			BString				fComment;
			BString				fLanguage;
			BString				fPackageVersion;
			int32				fUpVotes;
			int32				fDownVotes;
			BDateTime			fCreateTimestamp;
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


class StabilityRating {
public:
								StabilityRating();
								StabilityRating(
									const BString& label,
									const BString& name);
								StabilityRating(const StabilityRating& other);

			StabilityRating&	operator=(const StabilityRating& other);
			bool				operator==(const StabilityRating& other) const;
			bool				operator!=(const StabilityRating& other) const;

			const BString&		Label() const
									{ return fLabel; }
			const BString&		Name() const
									{ return fName; }
private:
			BString				fLabel;
			BString				fName;
};


typedef List<StabilityRating, false> StabilityRatingList;


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
			const CategoryList&	Categories() const
									{ return fCategories; }

			void				ClearUserRatings();
			bool				AddUserRating(const UserRating& rating);
			const UserRatingList& UserRatings() const
									{ return fUserRatings; }
			void				SetRatingSummary(const RatingSummary& summary);
			RatingSummary		CalculateRatingSummary() const;

			void				SetProminence(float prominence);
			float				Prominence() const
									{ return fProminence; }
			bool				HasProminence() const
									{ return fProminence != 0.0f; }
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

	static	void				CleanupDefaultIcon();

			void				StartCollatingChanges();
			void				EndCollatingChanges();

private:
			void				_NotifyListeners(uint32 changes);
			void				_NotifyListenersImmediate(uint32 changes);

private:
			BitmapRef			fIcon;
			BString				fName;
			BString				fTitle;
			BPackageVersion		fVersion;
			PublisherInfo		fPublisher;
			BString				fShortDescription;
			BString				fFullDescription;
			BString				fChangelog;
			CategoryList		fCategories;
			UserRatingList		fUserRatings;
			RatingSummary		fCachedRatingSummary;
			float				fProminence;
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

	static	BitmapRef			sDefaultIcon;
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

			int32				PackageIndexByName(const BString& packageName)
									const;

			void				SyncPackages(const PackageList& packages);

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


typedef List<DepotInfo, false> DepotList;


typedef List<BString, false> StringList;


#endif // PACKAGE_INFO_H
