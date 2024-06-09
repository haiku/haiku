/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PublisherInfo.h"


PublisherInfo::PublisherInfo()
	:
	fName(),
	fWebsite()
{
}


PublisherInfo::PublisherInfo(const BString& name, const BString& website)
	:
	fName(name),
	fWebsite(website)
{
}


PublisherInfo::PublisherInfo(const PublisherInfo& other)
	:
	fName(other.fName),
	fWebsite(other.fWebsite)
{
}


PublisherInfo&
PublisherInfo::operator=(const PublisherInfo& other)
{
	fName = other.fName;
	fWebsite = other.fWebsite;
	return *this;
}


bool
PublisherInfo::operator==(const PublisherInfo& other) const
{
	return fName == other.fName && fWebsite == other.fWebsite;
}


bool
PublisherInfo::operator!=(const PublisherInfo& other) const
{
	return !(*this == other);
}
