/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <FindDirectory.h>
#include <Locker.h>

#include "AbstractProcess.h"
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

			// Access to global categories
			const CategoryRef&	CategoryAudio() const
									{ return fCategoryAudio; }
			const CategoryRef&	CategoryBusiness() const
									{ return fCategoryBusiness; }
			const CategoryRef&	CategoryDevelopment() const
									{ return fCategoryDevelopment; }
			const CategoryRef&	CategoryEducation() const
									{ return fCategoryEducation; }
			const CategoryRef&	CategoryInternetAndNetwork() const
									{ return fCategoryInternetAndNetwork; }
			const CategoryRef&	CategoryGames() const
									{ return fCategoryGames; }
			const CategoryRef&	CategoryGraphics() const
									{ return fCategoryGraphics; }
			const CategoryRef&	CategoryProductivity() const
									{ return fCategoryProductivity; }
			const CategoryRef&	CategoryScienceAndMathematics() const
									{ return fCategoryScienceAndMathematics; }
			const CategoryRef&	CategorySystemAndUtilities() const
									{ return fCategorySystemAndUtilities; }
			const CategoryRef&	CategoryVideo() const
									{ return fCategoryVideo; }

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

			const StringList&	SupportedLanguages() const
									{ return fSupportedLanguages; }

			const BString&		PreferredLanguage() const
									{ return fPreferredLanguage; }

			void				SetUsername(BString username);
			const BString&		Username() const;
			void				SetAuthorization(const BString& username,
									const BString& password,
									bool storePassword);

			const WebAppInterface& GetWebAppInterface() const
									{ return fWebAppInterface; }

			void				ReplaceDepotByUrl(
									const BString& URL,
									DepotMapper* depotMapper,
									void* context);

			status_t			IconStoragePath(BPath& path) const;
			status_t			DumpExportRepositoryDataPath(BPath& path) const;
			status_t			DumpExportPkgDataPath(BPath& path,
									const BString& repositorySourceCode) const;

			void				LogDepotsWithNoWebAppRepositoryCode() const;

private:
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

			bool				_GetCacheFile(BPath& path, BFile& file,
									directory_which directory,
									const char* relativeLocation,
									const char* fileName,
									uint32 openMode) const;
			bool				_GetCacheFile(BPath& path, BFile& file,
									directory_which directory,
									const char* relativeLocation,
									const char* fileName,
									bool ignoreAge, time_t maxAge) const;

			void				_NotifyAuthorizationChanged();

private:
			BLocker				fLock;

			DepotList			fDepots;

			CategoryRef			fCategoryAudio;
			CategoryRef			fCategoryBusiness;
			CategoryRef			fCategoryDevelopment;
			CategoryRef			fCategoryEducation;
			CategoryRef			fCategoryGames;
			CategoryRef			fCategoryGraphics;
			CategoryRef			fCategoryInternetAndNetwork;
			CategoryRef			fCategoryProductivity;
			CategoryRef			fCategoryScienceAndMathematics;
			CategoryRef			fCategorySystemAndUtilities;
			CategoryRef			fCategoryVideo;
			// TODO: Dynamic categories retrieved from web-app

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

			StringList			fSupportedLanguages;
			BString				fPreferredLanguage;

			WebAppInterface		fWebAppInterface;

			ModelListenerList	fListeners;
};


#endif // PACKAGE_INFO_H
