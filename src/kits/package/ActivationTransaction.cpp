/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/ActivationTransaction.h>


namespace BPackageKit {
namespace BPrivate {


BActivationTransaction::BActivationTransaction()
	:
	fLocation(B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT),
	fChangeCount(0),
	fTransactionDirectoryName(),
	fPackagesToActivate(),
	fPackagesToDeactivate()
{
}


BActivationTransaction::~BActivationTransaction()
{
}


status_t
BActivationTransaction::InitCheck() const
{
	if (fLocation < 0 || fLocation >= B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT
		|| fTransactionDirectoryName.IsEmpty()
		|| (fPackagesToActivate.IsEmpty() && fPackagesToDeactivate.IsEmpty())) {
		return B_BAD_VALUE;
	}
	return B_OK;
}

status_t
BActivationTransaction::SetTo(BPackageInstallationLocation location,
	int64 changeCount, const BString& directoryName)
{
	if (location < 0 || location >= B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT
		|| directoryName.IsEmpty()) {
		return B_BAD_VALUE;
	}

	fLocation = location;
	fChangeCount = changeCount;
	fTransactionDirectoryName = directoryName;
	fPackagesToActivate.MakeEmpty();
	fPackagesToDeactivate.MakeEmpty();

	return B_OK;
}


BPackageInstallationLocation
BActivationTransaction::Location() const
{
	return fLocation;
}


void
BActivationTransaction::SetLocation(BPackageInstallationLocation location)
{
	fLocation = location;
}


int64
BActivationTransaction::ChangeCount() const
{
	return fChangeCount;
}


void
BActivationTransaction::SetChangeCount(int64 changeCount)
{
	fChangeCount = changeCount;
}


const BString&
BActivationTransaction::TransactionDirectoryName() const
{
	return fTransactionDirectoryName;
}


void
BActivationTransaction::SetTransactionDirectoryName(
	const BString& directoryName)
{
	fTransactionDirectoryName = directoryName;
}


const BStringList&
BActivationTransaction::PackagesToActivate() const
{
	return fPackagesToActivate;
}


bool
BActivationTransaction::SetPackagesToActivate(const BStringList& packages)
{
	fPackagesToActivate = packages;
	return fPackagesToActivate.CountStrings() == packages.CountStrings();
}


bool
BActivationTransaction::AddPackageToActivate(const BString& package)
{
	return fPackagesToActivate.Add(package);
}


const BStringList&
BActivationTransaction::PackagesToDeactivate() const
{
	return fPackagesToDeactivate;
}


bool
BActivationTransaction::SetPackagesToDeactivate(const BStringList& packages)
{
	fPackagesToDeactivate = packages;
	return fPackagesToDeactivate.CountStrings() == packages.CountStrings();
}


bool
BActivationTransaction::AddPackageToDeactivate(const BString& package)
{
	return fPackagesToDeactivate.Add(package);
}


}	// namespace BPrivate
}	// namespace BPackageKit
