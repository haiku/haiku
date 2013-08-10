/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef MODEL_H
#define MODEL_H


#include "PackageInfo.h"


class PackageFilter : public BReferenceable {
public:
	virtual						~PackageFilter();

	virtual	bool				AcceptsPackage(
									const PackageInfo& package) const = 0;
};

typedef BReference<PackageFilter> PackageFilterRef;


class Model {
public:
								Model();

			// !Returns new PackageInfoList from current parameters
			PackageList			CreatePackageList() const;

			bool				AddDepot(const DepotInfo& depot);
			const DepotList&	Depots() const
									{ return fDepots; }

			// Access to global categories
			const CategoryRef&	CategoryAudio() const
									{ return fCategoryAudio; }
			const CategoryRef&	CategoryVideo() const
									{ return fCategoryVideo; }
			const CategoryRef&	CategoryGraphics() const
									{ return fCategoryGraphics; }
			const CategoryRef&	CategoryProductivity() const
									{ return fCategoryProductivity; }
			const CategoryRef&	CategoryDevelopment() const
									{ return fCategoryDevelopment; }
			const CategoryRef&	CategoryCommandLine() const
									{ return fCategoryCommandLine; }
			const CategoryRef&	CategoryGames() const
									{ return fCategoryGames; }

			const CategoryList&	Categories() const
									{ return fCategories; }
			const CategoryList&	UserCategories() const
									{ return fUserCategories; }
			const CategoryList&	ProgressCategories() const
									{ return fProgressCategories; }

			void				SetPackageState(
									const PackageInfo& package,
									PackageState state);

			// Configure PackageFilters
			void				SetCategory(const BString& category);
			void				SetDepot(const BString& depot);
			void				SetSearchTerms(const BString& searchTerms);

private:
			DepotList			fDepots;

			CategoryRef			fCategoryAudio;
			CategoryRef			fCategoryVideo;
			CategoryRef			fCategoryGraphics;
			CategoryRef			fCategoryProductivity;
			CategoryRef			fCategoryDevelopment;
			CategoryRef			fCategoryCommandLine;
			CategoryRef			fCategoryGames;
			// TODO: More categories

			CategoryList		fCategories;
			CategoryList		fUserCategories;
			CategoryList		fProgressCategories;

			PackageList			fInstalledPackages;
			PackageList			fActivatedPackages;
			PackageList			fUninstalledPackages;
			PackageList			fDownloadingPackages;
			PackageList			fUpdateablePackages;

			PackageFilterRef	fCategoryFilter;
			BString				fDepotFilter;
			PackageFilterRef	fSearchTermsFilter;
};


#endif // PACKAGE_INFO_H
