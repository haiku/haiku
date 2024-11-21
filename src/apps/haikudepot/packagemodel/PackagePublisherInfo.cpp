/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackagePublisherInfo.h"


PackagePublisherInfo::PackagePublisherInfo()
	:
	fName(),
	fWebsite()
{
}


PackagePublisherInfo::PackagePublisherInfo(const BString& name, const BString& website)
	:
	fName(name),
	fWebsite(website)
{
}


PackagePublisherInfo::PackagePublisherInfo(const PackagePublisherInfo& other)
	:
	fName(other.fName),
	fWebsite(other.fWebsite)
{
}


PackagePublisherInfo&
PackagePublisherInfo::operator=(const PackagePublisherInfo& other)
{
	fName = other.fName;
	fWebsite = other.fWebsite;
	return *this;
}


bool
PackagePublisherInfo::operator==(const PackagePublisherInfo& other) const
{
	return fName == other.fName && fWebsite == other.fWebsite;
}


bool
PackagePublisherInfo::operator!=(const PackagePublisherInfo& other) const
{
	return !(*this == other);
}
