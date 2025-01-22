/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */
#include "PackageFilter.h"

#include "Logger.h"
#include "PackageFilterSpecification.h"
#include "PackageUtils.h"


PackageFilter::~PackageFilter()
{
}


void
AndFilter::AddFilter(PackageFilterRef filter)
{
	fFilters.push_back(filter);
}


NotFilter::NotFilter(PackageFilterRef filter)
	:
	fFilter(filter)
{
}


bool
NotFilter::AcceptsPackage(const PackageInfoRef& package) const
{
	return !(fFilter.IsSet() && fFilter->AcceptsPackage(package));
}


bool
AndFilter::AcceptsPackage(const PackageInfoRef& package) const
{
	std::vector<PackageFilterRef>::const_iterator it;
	for (it = fFilters.begin(); it != fFilters.end(); it++) {
		PackageFilterRef aFilter = *it;
		if (!aFilter.IsSet() || !aFilter->AcceptsPackage(package))
			return false;
	}
	return true;
}


class FalseFilter : public PackageFilter
{
public:
	FalseFilter()
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return false;
	}
};


class StateFilter : public PackageFilter
{
public:
	StateFilter(PackageState state)
		:
		fState(state)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		return PackageUtils::State(package) == fState;
	}

private:
	PackageState	fState;
};


class CategoryFilter : public PackageFilter {
public:
	CategoryFilter(const BString& categoryCode)
		:
		fCategoryCode(categoryCode)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;

		PackageClassificationInfoRef classificationInfo = package->PackageClassificationInfo();

		if (!classificationInfo.IsSet())
			return false;

		return classificationInfo->HasCategoryByCode(CategoryCode());
	}

	const BString& CategoryCode() const
	{
		return fCategoryCode;
	}

private:
	BString		fCategoryCode;
};


class DepotFilter : public PackageFilter {
public:
	DepotFilter(const BString& name)
		:
		fName(name)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		BString depotName = PackageUtils::DepotName(package);
		return !depotName.IsEmpty() && depotName == fName;
	}

	const BString& Name() const
	{
		return fName;
	}

private:
	BString		fName;
};


class SourceFilter : public PackageFilter {
public:
	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;
		const BString& packageName = package->Name();
		return packageName.EndsWith("_source");
	}
};


class DevelopmentFilter : public PackageFilter {
public:
	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;
		const BString& packageName = package->Name();
		return packageName.EndsWith("_devel") || packageName.EndsWith("_debuginfo");
	}
};


class SearchTermsFilter : public PackageFilter {
public:
	SearchTermsFilter(const BString& searchTerms)
	{
		// Separate the string into terms at spaces
		int32 index = 0;
		while (index < searchTerms.Length()) {
			int32 nextSpace = searchTerms.FindFirst(" ", index);
			if (nextSpace < 0)
				nextSpace = searchTerms.Length();
			if (nextSpace > index) {
				BString term;
				searchTerms.CopyInto(term, index, nextSpace - index);
				term.ToLower();
				fSearchTerms.Add(term);
			}
			index = nextSpace + 1;
		}
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;
		// Every search term must be found in one of the package texts
		for (int32 i = fSearchTerms.CountStrings() - 1; i >= 0; i--) {
			const BString& term = fSearchTerms.StringAt(i);
			if (package->Name().FindFirst(term) < 0 && !_AcceptsPackageFromPublisher(package, term)
				&& !_AcceptsPackageFromLocalizedText(package, term)) {
				return false;
			}
		}
		return true;
	}

	BString SearchTerms() const
	{
		BString searchTerms;
		for (int32 i = 0; i < fSearchTerms.CountStrings(); i++) {
			const BString& term = fSearchTerms.StringAt(i);
			if (term.IsEmpty())
				continue;
			if (!searchTerms.IsEmpty())
				searchTerms.Append(" ");
			searchTerms.Append(term);
		}
		return searchTerms;
	}

private:
	bool _TextContains(BString text, const BString& string) const
	{
		int32 index = text.IFindFirst(string);
		return index >= 0;
	}

	bool _AcceptsPackageFromPublisher(const PackageInfoRef& package,
		const BString& searchTerm) const
	{
		BString publisherName = PackageUtils::PublisherName(package);
		return _TextContains(publisherName, searchTerm);
	}

	bool _AcceptsPackageFromLocalizedText(const PackageInfoRef& package,
		const BString& searchTerm) const
	{
		if (!package.IsSet())
			return false;

		PackageLocalizedTextRef localizedText = package->LocalizedText();

		if (!localizedText.IsSet())
			return false;

		return _TextContains(localizedText->Title(), searchTerm)
			|| _TextContains(localizedText->Summary(), searchTerm)
			|| _TextContains(localizedText->Description(), searchTerm);
	}

private:
	BStringList fSearchTerms;
};


// #pragma mark - factory


/*static*/ PackageFilterRef
PackageFilterFactory::CreateCategoryFilter(const BString& category)
{
	return PackageFilterRef(new CategoryFilter(category), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateSearchTermsFilter(const BString& searchTerms)
{
	return PackageFilterRef(new SearchTermsFilter(searchTerms), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateDepotFilter(const BString& depotName)
{
	return PackageFilterRef(new DepotFilter(depotName), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateStateFilter(PackageState state)
{
	return PackageFilterRef(new StateFilter(state), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateSourceFilter()
{
	return PackageFilterRef(new SourceFilter(), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateDevelopmentFilter()
{
	return PackageFilterRef(new DevelopmentFilter(), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateFalseFilter()
{
	return PackageFilterRef(new FalseFilter(), true);
}


/*static*/ PackageFilterRef
PackageFilterFactory::CreateFilter(const PackageFilterSpecificationRef specification)
{
	if (!specification.IsSet())
		return CreateFalseFilter();

	AndFilter* andFilter = new AndFilter();

	if (!specification->SearchTerms().IsEmpty())
		andFilter->AddFilter(CreateSearchTermsFilter(specification->SearchTerms()));

	if (!specification->DepotName().IsEmpty())
		andFilter->AddFilter(CreateDepotFilter(specification->DepotName()));

	if (!specification->Category().IsEmpty())
		andFilter->AddFilter(CreateCategoryFilter(specification->Category()));

	if (!specification->ShowAvailablePackages())
		andFilter->AddFilter(PackageFilterRef(new NotFilter(CreateStateFilter(NONE)), true));

	if (!specification->ShowInstalledPackages())
		andFilter->AddFilter(PackageFilterRef(new NotFilter(CreateStateFilter(ACTIVATED)), true));

	if (!specification->ShowSourcePackages())
		andFilter->AddFilter(PackageFilterRef(new NotFilter(CreateSourceFilter()), true));

	if (!specification->ShowDevelopPackages())
		andFilter->AddFilter(PackageFilterRef(new NotFilter(CreateDevelopmentFilter()), true));

	return PackageFilterRef(andFilter, true);
}
