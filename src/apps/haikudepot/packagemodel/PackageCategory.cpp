/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageCategory.h"

#include <Collator.h>

#include "LocaleUtils.h"
#include "Logger.h"


PackageCategory::PackageCategory()
	:
	BReferenceable(),
	fCode(),
	fName()
{
}


PackageCategory::PackageCategory(const BString& code, const BString& name)
	:
	BReferenceable(),
	fCode(code),
	fName(name)
{
}


PackageCategory::PackageCategory(const PackageCategory& other)
	:
	BReferenceable(),
	fCode(other.fCode),
	fName(other.fName)
{
}


PackageCategory&
PackageCategory::operator=(const PackageCategory& other)
{
	fCode = other.fCode;
	fName = other.fName;
	return *this;
}


bool
PackageCategory::operator==(const PackageCategory& other) const
{
	return fCode == other.fCode && fName == other.fName;
}


bool
PackageCategory::operator!=(const PackageCategory& other) const
{
	return !(*this == other);
}


int
PackageCategory::Compare(const PackageCategory& other) const
{
	BCollator* collator = LocaleUtils::GetSharedCollator();
	int32 result = collator->Compare(Name().String(),
		other.Name().String());
	if (result == 0)
		result = Code().Compare(other.Code());
	return result;
}


bool IsPackageCategoryBefore(const CategoryRef& c1,
	const CategoryRef& c2)
{
	if (!c1.IsSet() || !c2.IsSet())
		HDFATAL("unexpected NULL reference in a referencable");
	return c1->Compare(*c2) < 0;
}
