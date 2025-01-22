/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DepotInfo.h"

#include <algorithm>

#include "Logger.h"
#include "PackageUtils.h"


// #pragma mark - Class implementation


DepotInfo::DepotInfo()
	:
	fName(),
	fIdentifier(),
	fWebAppRepositoryCode()
{
}


DepotInfo::DepotInfo(const BString& name)
	:
	fName(name),
	fIdentifier(),
	fWebAppRepositoryCode(),
	fWebAppRepositorySourceCode()
{
}


DepotInfo::DepotInfo(const DepotInfo& other)
	:
	fName(other.fName),
	fIdentifier(other.fIdentifier),
	fWebAppRepositoryCode(other.fWebAppRepositoryCode),
	fWebAppRepositorySourceCode(other.fWebAppRepositorySourceCode)
{
}


bool
DepotInfo::operator==(const DepotInfo& other) const
{
	return fName == other.fName && fIdentifier == fIdentifier;
}


bool
DepotInfo::operator!=(const DepotInfo& other) const
{
	return !(*this == other);
}


const BString&
DepotInfo::Name() const
{
	return fName;
}


const BString&
DepotInfo::Identifier() const
{
	return fIdentifier;
}


const BString&
DepotInfo::WebAppRepositoryCode() const
{
	return fWebAppRepositoryCode;
}


const BString&
DepotInfo::WebAppRepositorySourceCode() const
{
	return fWebAppRepositorySourceCode;
}


void
DepotInfo::SetIdentifier(const BString& value)
{
	fIdentifier = value;
}


void
DepotInfo::SetWebAppRepositoryCode(const BString& code)
{
	fWebAppRepositoryCode = code;
}


void
DepotInfo::SetWebAppRepositorySourceCode(const BString& code)
{
	fWebAppRepositorySourceCode = code;
}


// #pragma mark - DepotInfoBuilder


DepotInfoBuilder::DepotInfoBuilder()
	:
	fName(),
	fIdentifier(),
	fWebAppRepositoryCode(),
	fWebAppRepositorySourceCode()
{
}


DepotInfoBuilder::DepotInfoBuilder(const DepotInfoRef& value)
	:
	fName(),
	fIdentifier(),
	fWebAppRepositoryCode(),
	fWebAppRepositorySourceCode()
{
	fSource = value;
}


DepotInfoBuilder::~DepotInfoBuilder()
{
}


void
DepotInfoBuilder::_InitFromSource()
{
	if (fSource.IsSet()) {
		_Init(fSource.Get());
		fSource.Unset();
	}
}


void
DepotInfoBuilder::_Init(const DepotInfo* value)
{
	fName = value->Name();
	fIdentifier = value->Identifier();
	fWebAppRepositoryCode = value->WebAppRepositoryCode();
	fWebAppRepositorySourceCode = value->WebAppRepositorySourceCode();
}


DepotInfoRef
DepotInfoBuilder::BuildRef() const
{
	if (fSource.IsSet())
		return fSource;

	DepotInfo* depotInfo = new DepotInfo(fName);
	depotInfo->SetIdentifier(fIdentifier);
	depotInfo->SetWebAppRepositoryCode(fWebAppRepositoryCode);
	depotInfo->SetWebAppRepositorySourceCode(fWebAppRepositorySourceCode);
	return DepotInfoRef(depotInfo, true);
}


DepotInfoBuilder&
DepotInfoBuilder::WithName(const BString& value)
{
	if (!fSource.IsSet() || fSource->Name() != value) {
		_InitFromSource();
		fName = value;
	}
	return *this;
}


DepotInfoBuilder&
DepotInfoBuilder::WithIdentifier(const BString& value)
{
	if (!fSource.IsSet() || fSource->Identifier() != value) {
		_InitFromSource();
		fIdentifier = value;
	}
	return *this;
}


DepotInfoBuilder&
DepotInfoBuilder::WithWebAppRepositoryCode(const BString& value)
{
	if (!fSource.IsSet() || fSource->WebAppRepositoryCode() != value) {
		_InitFromSource();
		fWebAppRepositoryCode = value;
	}
	return *this;
}


DepotInfoBuilder&
DepotInfoBuilder::WithWebAppRepositorySourceCode(const BString& value)
{
	if (!fSource.IsSet() || fSource->WebAppRepositorySourceCode() != value) {
		_InitFromSource();
		fWebAppRepositorySourceCode = value;
	}
	return *this;
}
