/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageClassificationInfo.h"

#include <algorithm>

#include "HaikuDepotConstants.h"


PackageClassificationInfo::PackageClassificationInfo()
	:
	fCategories(),
	fProminence(PROMINANCE_ORDERING_NONE),
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


uint32
PackageClassificationInfo::Prominence() const
{
	return fProminence;
}


bool
PackageClassificationInfo::HasProminence() const
{
	return fProminence != 0;
}


int32
PackageClassificationInfo::CountCategories() const
{
	return fCategories.size();
}


const CategoryRef
PackageClassificationInfo::CategoryAtIndex(int32 index) const
{
	return fCategories[index];
}


bool
PackageClassificationInfo::AddCategory(const CategoryRef& category)
{
	std::vector<CategoryRef>::const_iterator itInsertionPt = std::lower_bound(fCategories.begin(),
		fCategories.end(), category, &IsPackageCategoryRefLess);

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


// #pragma mark - PackageClassificationInfoBuilder


PackageClassificationInfoBuilder::PackageClassificationInfoBuilder()
	:
	fCategories(),
	fProminence(-1L),
	fIsNativeDesktop(false)
{
}


PackageClassificationInfoBuilder::PackageClassificationInfoBuilder(
	const PackageClassificationInfoRef& value)
	:
	fCategories(),
	fProminence(PROMINANCE_ORDERING_NONE),
	fIsNativeDesktop(false)
{
	fSource = value;
}


PackageClassificationInfoBuilder::~PackageClassificationInfoBuilder()
{
}


void
PackageClassificationInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
PackageClassificationInfoBuilder::_Init(const PackageClassificationInfo* value)
{
	for (int32 i = value->CountCategories() - 1; i >= 0; i--)
		fCategories.push_back(value->CategoryAtIndex(i));
	fProminence = value->Prominence();
	fIsNativeDesktop = value->IsNativeDesktop();
}


PackageClassificationInfoRef
PackageClassificationInfoBuilder::BuildRef() const
{
	if (fSource.IsSet())
		return fSource;

	PackageClassificationInfo* classificationInfo = new PackageClassificationInfo();

	classificationInfo->SetProminence(fProminence);
	classificationInfo->SetIsNativeDesktop(fIsNativeDesktop);

	std::vector<CategoryRef>::const_iterator it;

	for (it = fCategories.begin(); it != fCategories.end(); it++)
		classificationInfo->AddCategory(*it);

	return PackageClassificationInfoRef(classificationInfo, true);
}


PackageClassificationInfoBuilder&
PackageClassificationInfoBuilder::WithProminence(uint32 prominence)
{
	if (!fSource.IsSet() || fSource->Prominence() != prominence) {
		_InitFromSource();
		fProminence = prominence;
	}
	return *this;
}


PackageClassificationInfoBuilder&
PackageClassificationInfoBuilder::WithIsNativeDesktop(bool value)
{
	if (!fSource.IsSet() || fSource->IsNativeDesktop() != value) {
		_InitFromSource();
		fIsNativeDesktop = value;
	}
	return *this;
}


PackageClassificationInfoBuilder&
PackageClassificationInfoBuilder::AddCategory(const CategoryRef& category)
{
	if (!fSource.IsSet() || fSource->HasCategoryByCode(category->Code())) {
		_InitFromSource();
		fCategories.push_back(category);
	}
	return *this;
}
