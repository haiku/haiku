/*
 * Copyright 2024-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageVersion.h"


PackageVersion::PackageVersion()
	:
	BPackageVersion(),
	fCreateTimestamp(0)
{
}


PackageVersion::PackageVersion(uint64 createTimestamp)
	:
	BPackageVersion(),
	fCreateTimestamp(createTimestamp)
{
}


PackageVersion::PackageVersion(const PackageVersion& other)
	:
	BPackageVersion(other),
	fCreateTimestamp(other.fCreateTimestamp)
{
}


PackageVersion::PackageVersion(const PackageVersion& other, uint64 createTimestamp)
	:
	BPackageVersion(other),
	fCreateTimestamp(createTimestamp)
{
}


PackageVersion::PackageVersion(const BPackageVersion& other)
	:
	BPackageVersion(other),
	fCreateTimestamp(0)
{
}


PackageVersion::~PackageVersion()
{
}


uint64
PackageVersion::CreateTimestamp() const
{
	return fCreateTimestamp;
}


bool
PackageVersion::operator==(const PackageVersion& other) const
{
	// can't use BPackageVersion operator== possibly because it is inlined?
	return Major() == other.Major() && Minor() == other.Minor() && Micro() == other.Micro()
		&& PreRelease() == other.PreRelease() && Revision() == other.Revision()
		&& fCreateTimestamp == other.CreateTimestamp();
}


bool
PackageVersion::operator!=(const PackageVersion& other) const
{
	return !(*this == other);
}
