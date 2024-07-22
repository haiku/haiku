/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "PackageFilterModel.h"


PackageFilterModel::PackageFilterModel()
	:
	fSearchTerms(""),
	fDepotName(""),
	fCategory(""),
	fShowAvailablePackages(true),
	fShowInstalledPackages(true),
	fShowSourcePackages(false),
	fShowDevelopPackages(false)
{
	fFilter = _CreateFilter();
}


PackageFilterModel::~PackageFilterModel()
{
}


BString
PackageFilterModel::SearchTerms() const
{
	return fSearchTerms;
}


BString
PackageFilterModel::DepotName() const
{
	return fDepotName;
}


BString
PackageFilterModel::Category() const
{
	return fCategory;
}


bool
PackageFilterModel::ShowAvailablePackages() const
{
	return fShowAvailablePackages;
}


bool
PackageFilterModel::ShowInstalledPackages() const
{
	return fShowInstalledPackages;
}


bool
PackageFilterModel::ShowSourcePackages() const
{
	return fShowSourcePackages;
}


bool
PackageFilterModel::ShowDevelopPackages() const
{
	return fShowDevelopPackages;
}


void
PackageFilterModel::SetSearchTerms(BString value)
{
	if (fSearchTerms != value) {
		fSearchTerms = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetDepotName(BString value)
{
	if (fDepotName != value) {
		fDepotName = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetCategory(BString value)
{
	if (fCategory != value) {
		fCategory = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetShowAvailablePackages(bool value)
{
	if (fShowAvailablePackages != value) {
		fShowAvailablePackages = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetShowInstalledPackages(bool value)
{
	if (fShowInstalledPackages != value) {
		fShowInstalledPackages = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetShowSourcePackages(bool value)
{
	if (fShowSourcePackages != value) {
		fShowSourcePackages = value;
		fFilter = _CreateFilter();
	}
}


void
PackageFilterModel::SetShowDevelopPackages(bool value)
{
	if (fShowDevelopPackages != value) {
		fShowDevelopPackages = value;
		fFilter = _CreateFilter();
	}
}


PackageFilterRef
PackageFilterModel::Filter()
{
	return fFilter;
}


PackageFilterRef
PackageFilterModel::_CreateFilter() const
{
	AndFilter* andFilter = new AndFilter();
	if (!fSearchTerms.IsEmpty())
		andFilter->AddFilter(PackageFilterFactory::CreateSearchTermsFilter(fSearchTerms));
	if (!fDepotName.IsEmpty())
		andFilter->AddFilter(PackageFilterFactory::CreateDepotFilter(fDepotName));
	if (!fCategory.IsEmpty())
		andFilter->AddFilter(PackageFilterFactory::CreateCategoryFilter(fCategory));
	if (!fShowAvailablePackages) {
		andFilter->AddFilter(
			PackageFilterRef(new NotFilter(PackageFilterFactory::CreateStateFilter(NONE)), true));
	}
	if (!fShowInstalledPackages) {
		andFilter->AddFilter(PackageFilterRef(
			new NotFilter(PackageFilterFactory::CreateStateFilter(ACTIVATED)), true));
	}
	if (!fShowSourcePackages) {
		andFilter->AddFilter(
			PackageFilterRef(new NotFilter(PackageFilterFactory::CreateSourceFilter()), true));
	}
	if (!fShowDevelopPackages) {
		andFilter->AddFilter(
			PackageFilterRef(new NotFilter(PackageFilterFactory::CreateDevelopmentFilter()), true));
	}
	return PackageFilterRef(andFilter, true);
}
