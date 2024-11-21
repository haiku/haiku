/*
 * Copyright 2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "PackageVersion.h"


PackageVersion::PackageVersion()
	:
	BPackageVersion(),
	fCreateTimestamp(0)
{
}


PackageVersion::PackageVersion(const PackageVersion& other)
	:
	BPackageVersion(other),
	fCreateTimestamp(other.fCreateTimestamp)
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


void
PackageVersion::SetCreateTimestamp(uint64 value)
{
	fCreateTimestamp = value;
}


PackageVersion&
PackageVersion::operator=(const PackageVersion& other)
{
	if (this != &other) {
		// can't use BPackageVersion operator= possibly because it is inlined?
		SetTo(other.Major(), other.Minor(), other.Micro(), other.PreRelease(), other.Revision());
		fCreateTimestamp = other.fCreateTimestamp;
	}
	return *this;
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
