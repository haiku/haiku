/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H

#include <Locker.h>

#include "PackageInfo.h"

class BMessage;


class PackageFilter : public BReferenceable {
public:
	virtual						~PackageFilter();

	virtual	bool				AcceptsPackage(
									const PackageInfoRef& package) const = 0;
};

typedef BReference<PackageFilter> PackageFilterRef;


class Model {
public:
								Model();
	virtual						~Model();

			BLocker*			Lock()
									{ return &fLock; }

			// !Returns new PackageInfoList from current parameters
			PackageList			CreatePackageList() const;

			bool				AddDepot(const DepotInfo& depot);
			const DepotList&	Depots() const
									{ return fDepots; }

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
			const CategoryList&	UserCategories() const
									{ return fUserCategories; }
			const CategoryList&	ProgressCategories() const
									{ return fProgressCategories; }

			void				SetPackageState(
									const PackageInfoRef& package,
									PackageState state);

			// Configure PackageFilters
			void				SetCategory(const BString& category);
			void				SetDepot(const BString& depot);
			void				SetSearchTerms(const BString& searchTerms);
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

			void				PopulatePackage(const PackageInfoRef& package,
									uint32 flags);
			void				PopulateAllPackages();
			void				StopPopulatingAllPackages();

private:
	static	int32				_PopulateAllPackagesEntry(void* cookie);
			void				_PopulateAllPackagesThread(bool fromCacheOnly);

			void				_PopulatePackageInfos(
									PackageList& packages,
									bool fromCacheOnly,
									PackageList& packagesWithIcons);
			void				_PopulatePackageInfo(
									const PackageInfoRef& package,
									bool fromCacheOnly);
			void				_PopulatePackageInfo(
									const PackageInfoRef& package,
									const BMessage& data);
			void				_PopulatePackageIcon(
									const PackageInfoRef& package,
									bool fromCacheOnly);
			bool				_HasNativeIcon(const BMessage& message) const;

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
			CategoryList		fUserCategories;
			CategoryList		fProgressCategories;

			PackageList			fInstalledPackages;
			PackageList			fActivatedPackages;
			PackageList			fUninstalledPackages;
			PackageList			fDownloadingPackages;
			PackageList			fUpdateablePackages;
			PackageList			fPopulatedPackages;

			PackageFilterRef	fCategoryFilter;
			BString				fDepotFilter;
			PackageFilterRef	fSearchTermsFilter;

			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;

			thread_id			fPopulateAllPackagesThread;
	volatile bool				fStopPopulatingAllPackages;
};


#endif // PACKAGE_INFO_H
