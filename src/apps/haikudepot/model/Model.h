/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <vector>

#include <Locker.h>

#include "AbstractProcess.h"
#include "PackageIconTarRepository.h"
#include "LanguageModel.h"
#include "PackageInfo.h"
#include "RatingStability.h"
#include "WebAppInterface.h"


class BFile;
class BMessage;
class BPath;


typedef enum package_list_view_mode {
	PROMINENT,
	ALL
} package_list_view_mode;


class PackageFilter : public BReferenceable {
public:
	virtual						~PackageFilter();

	virtual	bool				AcceptsPackage(
									const PackageInfoRef& package) const = 0;
};

typedef BReference<PackageFilter> PackageFilterRef;


class ModelListener : public BReferenceable {
public:
	virtual						~ModelListener();

	virtual	void				AuthorizationChanged() = 0;
	virtual void				CategoryListChanged() = 0;
};


class PackageConsumer {
public:
	virtual	bool				ConsumePackage(
									const PackageInfoRef& packageInfoRef,
									void* context) = 0;
};


typedef BReference<ModelListener> ModelListenerRef;
typedef List<ModelListenerRef, false> ModelListenerList;


class Model {
public:
								Model();
	virtual						~Model();

			LanguageModel*		Language();
			PackageIconRepository&
								GetPackageIconRepository();
			status_t			InitPackageIconRepository();

			BLocker*			Lock()
									{ return &fLock; }

			bool				AddListener(const ModelListenerRef& listener);

			PackageInfoRef		PackageForName(const BString& name);
			bool				MatchesFilter(
									const PackageInfoRef& package) const;

			void				MergeOrAddDepot(const DepotInfoRef depot);
			bool				HasDepot(const BString& name) const;
			int32				CountDepots() const;
			DepotInfoRef		DepotAtIndex(int32 index) const;
			const DepotInfoRef	DepotForName(const BString& name) const;
			bool				HasAnyProminentPackages();

			void				Clear();

			int32				CountCategories() const;
			CategoryRef			CategoryByCode(BString& code) const;
			CategoryRef			CategoryAtIndex(int32 index) const;
			void				AddCategories(
									std::vector<CategoryRef>& values);

			int32				CountRatingStabilities() const;
			RatingStabilityRef	RatingStabilityByCode(BString& code) const;
			RatingStabilityRef	RatingStabilityAtIndex(int32 index) const;
			void				AddRatingStabilities(
									std::vector<RatingStabilityRef>& values);

			void				SetPackageState(
									const PackageInfoRef& package,
									PackageState state);

			// Configure PackageFilters
			void				SetCategory(const BString& category);
			BString				Category() const;
			void				SetDepot(const BString& depot);
			BString				Depot() const;
			void				SetSearchTerms(const BString& searchTerms);
			BString				SearchTerms() const;

			void				SetPackageListViewMode(
									package_list_view_mode mode);
			package_list_view_mode
								PackageListViewMode() const
									{ return fPackageListViewMode; }
			void				SetShowAvailablePackages(bool show);
			bool				ShowAvailablePackages() const
									{ return fShowAvailablePackages; }
			void				SetShowInstalledPackages(bool show);
			bool				ShowInstalledPackages() const
									{ return fShowInstalledPackages; }
			void				SetShowSourcePackages(bool show);
			bool				ShowSourcePackages() const
									{ return fShowSourcePackages; }
			void				SetShowDevelopPackages(bool show);
			bool				ShowDevelopPackages() const
									{ return fShowDevelopPackages; }

			// Retrieve package information
	static	const uint32		POPULATE_CACHED_RATING	= 1 << 0;
	static	const uint32		POPULATE_CACHED_ICON	= 1 << 1;
	static	const uint32		POPULATE_USER_RATINGS	= 1 << 2;
	static	const uint32		POPULATE_SCREEN_SHOTS	= 1 << 3;
	static	const uint32		POPULATE_CHANGELOG		= 1 << 4;
	static	const uint32		POPULATE_CATEGORIES		= 1 << 5;
	static	const uint32		POPULATE_FORCE			= 1 << 6;

			void				PopulatePackage(const PackageInfoRef& package,
									uint32 flags);

			void				SetNickname(BString nickname);
			const BString&		Nickname() const;
			void				SetAuthorization(const BString& nickname,
									const BString& passwordClear,
									bool storePassword);

			const WebAppInterface&
								GetWebAppInterface() const
									{ return fWebAppInterface; }

			status_t			IconTarPath(BPath& path) const;
			status_t			DumpExportReferenceDataPath(BPath& path);
			status_t			DumpExportRepositoryDataPath(BPath& path);
			status_t			DumpExportPkgDataPath(BPath& path,
									const BString& repositorySourceCode);

private:
			void				_AddCategory(const CategoryRef& category);

			void				_AddRatingStability(
									const RatingStabilityRef& value);

			void				_MaybeLogJsonRpcError(
									const BMessage &responsePayload,
									const char *sourceDescription) const;

	static	int32				_PopulateAllPackagesEntry(void* cookie);

			void				_PopulatePackageChangelog(
									const PackageInfoRef& package);

			void				_PopulatePackageScreenshot(
									const PackageInfoRef& package,
									const ScreenshotInfo& info,
									int32 scaledWidth, bool fromCacheOnly);

			void				_NotifyAuthorizationChanged();
			void				_NotifyCategoryListChanged();

private:
			BLocker				fLock;

			std::vector<DepotInfoRef>
								fDepots;
			std::vector<CategoryRef>
								fCategories;
			std::vector<RatingStabilityRef>
								fRatingStabilities;

			PackageList			fInstalledPackages;
			PackageList			fActivatedPackages;
			PackageList			fUninstalledPackages;
			PackageList			fDownloadingPackages;
			PackageList			fUpdateablePackages;
			PackageList			fPopulatedPackages;

			PackageFilterRef	fCategoryFilter;
			BString				fDepotFilter;
			PackageFilterRef	fSearchTermsFilter;

			package_list_view_mode
								fPackageListViewMode;
			bool				fShowAvailablePackages;
			bool				fShowInstalledPackages;
			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;

			LanguageModel		fLanguageModel;
			PackageIconTarRepository
								fPackageIconRepository;
			WebAppInterface		fWebAppInterface;

			ModelListenerList	fListeners;
};


#endif // PACKAGE_INFO_H
