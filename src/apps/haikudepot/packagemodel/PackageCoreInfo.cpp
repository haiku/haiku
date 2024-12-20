/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageCoreInfo.h"

#include "Package.h"


PackageCoreInfo::PackageCoreInfo()
	:
	fVersion(),
	fPublisher(),
	fArchitecture(),
	fDepotName()
{
}


PackageCoreInfo::PackageCoreInfo(const PackageCoreInfo& other)
	:
	fVersion(other.fVersion),
	fPublisher(other.fPublisher),
	fArchitecture(other.fArchitecture),
	fDepotName(other.fDepotName)
{
}


PackageCoreInfo::~PackageCoreInfo()
{
}


PackageCoreInfo&
PackageCoreInfo::operator=(const PackageCoreInfo& other)
{
	fVersion = other.fVersion;
	fPublisher = other.fPublisher;
	fArchitecture = other.fArchitecture;
	fDepotName = other.fDepotName;

	return *this;
}


bool
PackageCoreInfo::operator==(const PackageCoreInfo& other) const
{
	return fVersion == other.fVersion
		&& fPublisher == other.fPublisher
		&& fArchitecture == other.fArchitecture
		&& fDepotName == other.fDepotName;
}


bool
PackageCoreInfo::operator!=(const PackageCoreInfo& other) const
{
	return !(*this == other);
}


void
PackageCoreInfo::SetVersion(PackageVersionRef value)
{
	fVersion = value;
}


void
PackageCoreInfo::SetPublisher(PackagePublisherInfoRef value)
{
	fPublisher = value;
}


void
PackageCoreInfo::SetArchitecture(const BString& value)
{
	fArchitecture = value;
}


void
PackageCoreInfo::SetDepotName(const BString& value)
{
	fDepotName = value;
}
