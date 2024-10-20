/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageClassificationInfo.h"

#include <algorithm>

#include "HaikuDepotConstants.h"


PackageClassificationInfo::PackageClassificationInfo()
	:
	fCategories(),
	fProminence(-1L),
	fIsNativeDesktop(false)
{
}


PackageClassificationInfo::PackageClassificationInfo(const PackageClassificationInfo& other)
	:
	fCategories(other.fCategories),
	fProminence(other.Prominence()),
	fIsNativeDesktop(other.IsNativeDesktop())
{
}


int32
PackageClassificationInfo::CountCategories() const
{
	return fCategories.size();
}


CategoryRef
PackageClassificationInfo::CategoryAtIndex(int32 index) const
{
	return fCategories[index];
}


void
PackageClassificationInfo::ClearCategories()
{
	if (!fCategories.empty())
		fCategories.clear();
}


bool
PackageClassificationInfo::AddCategory(const CategoryRef& category)
{
	std::vector<CategoryRef>::const_iterator itInsertionPt = std::lower_bound(fCategories.begin(),
		fCategories.end(), category, &IsPackageCategoryBefore);

	if (itInsertionPt == fCategories.end()) {
		fCategories.push_back(category);
		return true;
	}
	return false;
}


bool
PackageClassificationInfo::HasCategoryByCode(const BString& code) const
{
	for (int32 i = CountCategories() - 1; i >= 0; i--) {
		if (CategoryAtIndex(i)->Code() == code)
			return true;
	}

	return false;
}


void
PackageClassificationInfo::SetProminence(uint32 prominence)
{
	fProminence = prominence;
}


bool
PackageClassificationInfo::IsProminent() const
{
	return HasProminence() && Prominence() <= PROMINANCE_ORDERING_PROMINENT_MAX;
}


bool
PackageClassificationInfo::IsNativeDesktop() const
{
	return fIsNativeDesktop;
}


void
PackageClassificationInfo::SetIsNativeDesktop(bool value)
{
	fIsNativeDesktop = value;
}


bool
PackageClassificationInfo::operator==(const PackageClassificationInfo& other) const
{
	if (fIsNativeDesktop != other.IsNativeDesktop())
		return false;

	if (fProminence != other.Prominence())
		return false;

	if (CountCategories() != other.CountCategories())
		return false;

	for (int32 i = CountCategories() - 1; i >= 0; i--) {
		if (other.CategoryAtIndex(i) != CategoryAtIndex(i))
			return false;
	}

	return true;
}


bool
PackageClassificationInfo::operator!=(const PackageClassificationInfo& other) const
{
	return !(*this == other);
}
