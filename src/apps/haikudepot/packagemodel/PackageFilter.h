/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */
#ifndef PACKAGE_FILTER_H
#define PACKAGE_FILTER_H

#include <vector>

#include <Referenceable.h>
#include <String.h>

#include "PackageInfo.h"


class PackageFilter : public BReferenceable
{
public:
	virtual						~PackageFilter();

	virtual	bool				AcceptsPackage(const PackageInfoRef& package) const = 0;
};


typedef BReference<PackageFilter> PackageFilterRef;


class NotFilter : public PackageFilter
{
public:
								NotFilter(PackageFilterRef filter);
	virtual bool				AcceptsPackage(const PackageInfoRef& package) const;

private:
			PackageFilterRef	fFilter;
};


class AndFilter : public PackageFilter
{
public:
			void				AddFilter(PackageFilterRef filter);
	virtual bool				AcceptsPackage(const PackageInfoRef& package) const;

private:
			std::vector<PackageFilterRef>
								fFilters;
};


class PackageFilterFactory
{
public:
	static	PackageFilterRef	CreateCategoryFilter(const BString& category);
	static	PackageFilterRef	CreateSearchTermsFilter(const BString& searchTerms);
	static	PackageFilterRef	CreateDepotFilter(const BString& depotName);
	static	PackageFilterRef	CreateStateFilter(PackageState state);
	static	PackageFilterRef	CreateSourceFilter();
	static	PackageFilterRef	CreateDevelopmentFilter();
};


#endif // PACKAGE_FILTER_H
