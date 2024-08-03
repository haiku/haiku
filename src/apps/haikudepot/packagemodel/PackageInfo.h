/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_INFO_H
#define PACKAGE_INFO_H


#include <set>
#include <vector>

#include <Referenceable.h>
#include <package/PackageInfo.h>

#include "Language.h"
#include "List.h"
#include "PackageCategory.h"
#include "PackageInfoListener.h"
#include "PublisherInfo.h"
#include "RatingSummary.h"
#include "ScreenshotInfo.h"
#include "UserRating.h"


typedef std::set<int32> PackageInstallationLocationSet;


enum PackageState {
	NONE		= 0,
	INSTALLED	= 1,
	DOWNLOADING	= 2,
	ACTIVATED	= 3,
	UNINSTALLED	= 4,
	PENDING		= 5,
};


const char* package_state_to_string(PackageState state);


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
			const BPackageVersion&
								Version() const
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
			void				ClearInstallationLocations();

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

			bool				DidPopulateUserRatings() const;
			void				SetDidPopulateUserRatings();

			void				ClearUserRatings();
			void				AddUserRating(const UserRatingRef& rating);
			int32				CountUserRatings() const;
			UserRatingRef		UserRatingAtIndex(int32 index) const;
			void				SetRatingSummary(const RatingSummary& summary);
			RatingSummary		CalculateRatingSummary() const;

			void				SetProminence(int64 prominence);
			int64				Prominence() const
									{ return fProminence; }
			bool				HasProminence() const
									{ return fProminence != 0; }
			bool				IsProminent() const;

			void				ClearScreenshotInfos();
			void				AddScreenshotInfo(
									const ScreenshotInfoRef& info);
			int32				CountScreenshotInfos() const;
			ScreenshotInfoRef	ScreenshotInfoAtIndex(int32 index) const;

			void				SetSize(off_t size);
			off_t				Size() const
									{ return fSize; }

			void				SetViewed();
			bool				Viewed() const
									{ return fViewed; }

			void				SetVersionCreateTimestamp(uint64 value);
			uint64				VersionCreateTimestamp() const
									{ return fVersionCreateTimestamp; }

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
			std::vector<UserRatingRef>
								fUserRatings;
			bool				fDidPopulateUserRatings;
			RatingSummary		fCachedRatingSummary;
			int64				fProminence;
			std::vector<ScreenshotInfoRef>
								fScreenshotInfos;

			PackageState		fState;
			PackageInstallationLocationSet
								fInstallationLocations;
			float				fDownloadProgress;
			std::vector<PackageInfoListenerRef>
								fListeners;
			int32				fFlags;
			bool				fSystemDependency;
			BString				fArchitecture;
			BString				fLocalFilePath;
			BString				fFileName;
			off_t				fSize;
			BString				fDepotName;
			bool				fViewed;

			bool				fIsCollatingChanges;
			uint32				fCollatedChanges;

			uint64				fVersionCreateTimestamp;
				// milliseconds since epoch
};


typedef BReference<PackageInfo> PackageInfoRef;


#endif // PACKAGE_INFO_H
