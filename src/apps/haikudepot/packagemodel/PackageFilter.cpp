/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2014, Axel Dörfler <axeld@pinc-software.de>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */


#include "PackageFilter.h"


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
		return package->State() == fState;
	}

private:
	PackageState	fState;
};


class CategoryFilter : public PackageFilter {
public:
	CategoryFilter(const BString& category)
		:
		fCategory(category)
	{
	}

	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;

		for (int i = package->CountCategories() - 1; i >= 0; i--) {
			const CategoryRef& category = package->CategoryAtIndex(i);
			if (!category.IsSet())
				continue;
			if (category->Code() == fCategory)
				return true;
		}
		return false;
	}

	const BString& Category() const
	{
		return fCategory;
	}

private:
	BString		fCategory;
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
		if (!package.IsSet())
			return false;
		return package->DepotName() == fName;
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


class DevelopmentFilter : public PackageFilter
{
public:
	virtual bool AcceptsPackage(const PackageInfoRef& package) const
	{
		if (!package.IsSet())
			return false;
		const BString& packageName = package->Name();
		return packageName.EndsWith("_devel")
			|| packageName.EndsWith("_debuginfo");
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
			if (!_TextContains(package->Name(), term)
				&& !_TextContains(package->Title(), term)
				&& !_TextContains(package->Publisher().Name(), term)
				&& !_TextContains(package->ShortDescription(), term)
				&& !_TextContains(package->FullDescription(), term)) {
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
 		text.ToLower();
 		int32 index = text.FindFirst(string);
 		return index >= 0;
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
