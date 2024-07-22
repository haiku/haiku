/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILTER_MODEL_H
#define PACKAGE_FILTER_MODEL_H

#include <String.h>

#include "PackageFilter.h"


class PackageFilterModel
{
public:
								PackageFilterModel();
	virtual						~PackageFilterModel();

			BString				SearchTerms() const;
			BString				DepotName() const;
			BString				Category() const;
			bool				ShowAvailablePackages() const;
			bool				ShowInstalledPackages() const;
			bool				ShowSourcePackages() const;
			bool				ShowDevelopPackages() const;

			void				SetSearchTerms(BString value);
			void				SetDepotName(BString value);
			void				SetCategory(BString value);
			void				SetShowAvailablePackages(bool value);
			void				SetShowInstalledPackages(bool value);
			void				SetShowSourcePackages(bool value);
			void				SetShowDevelopPackages(bool value);

			PackageFilterRef	Filter();

private:
			PackageFilterRef	_CreateFilter() const;

private:
			BString				fSearchTerms;
			BString				fDepotName;
			BString				fCategory;
			bool				fShowAvailablePackages;
			bool				fShowInstalledPackages;
			bool				fShowSourcePackages;
			bool				fShowDevelopPackages;

			PackageFilterRef	fFilter;
};


#endif // PACKAGE_FILTER_MODEL_H
