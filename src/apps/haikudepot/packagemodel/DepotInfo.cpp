/*
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent <rene@gollent.com>.
 * Copyright 2016-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "DepotInfo.h"

#include <algorithm>

#include "Logger.h"


// #pragma mark - Sorting Functions


static bool
_IsPackageBeforeByName(const PackageInfoRef& p1, const BString& packageName)
{
	return p1->Name().Compare(packageName) < 0;
}


/*!	This function is used in order to provide an ordering on the packages
	that are stored on a Depot.
 */

static bool
_IsPackageBefore(const PackageInfoRef& p1, const PackageInfoRef& p2)
{
	return _IsPackageBeforeByName(p1, p2->Name());
}


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
	fPackages(other.fPackages),
	fWebAppRepositoryCode(other.fWebAppRepositoryCode),
	fWebAppRepositorySourceCode(other.fWebAppRepositorySourceCode)
{
}


DepotInfo&
DepotInfo::operator=(const DepotInfo& other)
{
	fName = other.fName;
	fIdentifier = other.fIdentifier;
	fPackages = other.fPackages;
	fWebAppRepositoryCode = other.fWebAppRepositoryCode;
	fWebAppRepositorySourceCode = other.fWebAppRepositorySourceCode;
	return *this;
}


bool
DepotInfo::operator==(const DepotInfo& other) const
{
	return fName == other.fName
		&& fIdentifier == fIdentifier
		&& fPackages == other.fPackages;
}


bool
DepotInfo::operator!=(const DepotInfo& other) const
{
	return !(*this == other);
}


int32
DepotInfo::CountPackages() const
{
	return fPackages.size();
}


PackageInfoRef
DepotInfo::PackageAtIndex(int32 index)
{
	return fPackages[index];
}


/*! This method will insert the package into the list of packages
    in order so that the list of packages remains in order.
 */

void
DepotInfo::AddPackage(PackageInfoRef& package)
{
	std::vector<PackageInfoRef>::iterator itInsertionPt
		= std::lower_bound(
			fPackages.begin(),
			fPackages.end(),
			package,
			&_IsPackageBefore);
	fPackages.insert(itInsertionPt, package);
}


bool
DepotInfo::HasPackage(const BString& packageName)
{
	std::vector<PackageInfoRef>::const_iterator it
		= std::lower_bound(
			fPackages.begin(),
			fPackages.end(),
			packageName,
			&_IsPackageBeforeByName);
	if (it != fPackages.end()) {
		PackageInfoRef candidate = *it;
		return (candidate.Get() != NULL
			&& candidate.Get()->Name() == packageName);
	}
	return false;
}


PackageInfoRef
DepotInfo::PackageByName(const BString& packageName)
{
	std::vector<PackageInfoRef>::const_iterator it
		= std::lower_bound(
			fPackages.begin(),
			fPackages.end(),
			packageName,
			&_IsPackageBeforeByName);

	if (it != fPackages.end()) {
		PackageInfoRef candidate = *it;
		if (candidate.Get() != NULL && candidate.Get()->Name() == packageName)
			return candidate;
	}
	return PackageInfoRef();
}


void
DepotInfo::SyncPackagesFromDepot(const DepotInfoRef& other)
{
	for (int32 i = other->CountPackages() - 1; i >= 0; i--) {
		PackageInfoRef otherPackage = other->PackageAtIndex(i);
		PackageInfoRef myPackage = PackageByName(otherPackage->Name());

		if (myPackage.Get() != NULL) {
			myPackage->SetState(otherPackage->State());
			myPackage->SetLocalFilePath(otherPackage->LocalFilePath());
			myPackage->SetSystemDependency(otherPackage->IsSystemDependency());
		}
		else {
			HDINFO("%s: new package: '%s'", fName.String(),
				otherPackage->Name().String());
			AddPackage(otherPackage);
		}
	}

	for (int32 i = CountPackages() - 1; i >= 0; i--) {
		PackageInfoRef myPackage = PackageAtIndex(i);
		if (!other->HasPackage(myPackage->Name())) {
			HDINFO("%s: removing package: '%s'", fName.String(),
				myPackage->Name().String());
			fPackages.erase(fPackages.begin() + i);
		}
	}
}


bool
DepotInfo::HasAnyProminentPackages() const
{
	std::vector<PackageInfoRef>::const_iterator it;
	for (it = fPackages.begin(); it != fPackages.end(); it++) {
		const PackageInfoRef& package = *it;
		if (package->IsProminent())
			return true;
	}
	return false;
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
