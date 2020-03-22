/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <Locker.h>

#include "AbstractProcess.h"
#include "LanguageModel.h"
#include "LocalIconStore.h"
#include "PackageInfo.h"
#include "WebAppInterface.h"


class BFile;
class BMessage;
class BPath;


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


class DepotMapper {
public:
	virtual DepotInfo			MapDepot(const DepotInfo& depot,
									void* context) = 0;
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

			LanguageModel&		Language();

			BLocker*			Lock()
									{ return &fLock; }

			bool				AddListener(const ModelListenerRef& listener);

			// !Returns new PackageInfoList from current parameters
			PackageList			CreatePackageList() const;

			bool				MatchesFilter(
									const PackageInfoRef& package) const;

			bool				AddDepot(const DepotInfo& depot);
			bool				HasDepot(const BString& name) const;
			const DepotList&	Depots() const
									{ return fDepots; }
			const DepotInfo*	DepotForName(const BString& name) const;
			bool				SyncDepot(const DepotInfo& depot);

			void				Clear();

			void				AddCategories(const CategoryList& categories);
			const CategoryList&	Categories() const
									{ return fCategories; }

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

			void				SetShowFeaturedPackages(bool show);
			bool				ShowFeaturedPackages() const
									{ return fShowFeaturedPackages; }
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

			const WebAppInterface& GetWebAppInterface() const
									{ return fWebAppInterface; }

			void				ReplaceDepotByUrl(
									const BString& URL,
									DepotMapper* depotMapper,
									void* context);

			status_t			IconStoragePath(BPath& path) const;
			status_t			DumpExportReferenceDataPath(BPath& path) const;
			status_t			DumpExportRepositoryDataPath(BPath& path) const;
			status_t			DumpExportPkgDataPath(BPath& path,
									const BString& repositorySourceCode) const;

			void				LogDepotsWithNoWebAppRepositoryCode() const;

private:
			void				_AddCategory(const CategoryRef& category);

			void				_MaybeLogJsonRpcError(
									const BMessage &responsePayload,
									const char *sourceDescription) const;

			void				_UpdateIsFeaturedFilter();

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

			DepotList			fDepots;

			CategoryList		fCategories;

			PackageList			fInstalledPackages;
			PackageList			fActivatedPackages;
			PackageList			fUninstalledPackages;
			PackageList			fDownloadingPackages;
			PackageList			fUpdateablePackages;
			PackageList			fPopulatedPackages;

			PackageFilterRef	fCategoryFilter;
			BString				fDepotFilter;
			PackageFilterRef	fSearchTermsFilter;
			PackageFilterRef	fIsFeaturedFilter;

			bool				fShowFeaturedPackages;
			bool				fShowAvailablePackages;
			bool				fShowInstalledPackages;
			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;

			LanguageModel		fLanguageModel;
			WebAppInterface		fWebAppInterface;

			ModelListenerList	fListeners;
};


#endif // PACKAGE_INFO_H
