/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/ActivationTransaction.h>

#include <new>

#include <Message.h>


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


BActivationTransaction::BActivationTransaction(BMessage* archive,
	status_t* _error)
	:
	fLocation(B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT),
	fChangeCount(0),
	fTransactionDirectoryName(),
	fPackagesToActivate(),
	fPackagesToDeactivate()
{
	status_t error;
	int32 location;
	if ((error = archive->FindInt32("location", &location)) == B_OK
		&& (error = archive->FindInt64("change count", &fChangeCount)) == B_OK
		&& (error = archive->FindString("transaction",
			&fTransactionDirectoryName)) == B_OK
		&& (error = _ExtractStringList(archive, "activate",
			fPackagesToActivate)) == B_OK
		&& (error = _ExtractStringList(archive, "deactivate",
			fPackagesToDeactivate)) == B_OK) {
		if (location >= 0
			&& location <= B_PACKAGE_INSTALLATION_LOCATION_ENUM_COUNT) {
			fLocation = (BPackageInstallationLocation)location;
		} else
			error = B_BAD_VALUE;
	}

	if (_error != NULL)
		*_error = error;
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


status_t
BActivationTransaction::Archive(BMessage* archive, bool deep) const
{
	status_t error = BArchivable::Archive(archive, deep);
	if (error != B_OK)
		return error;

	if ((error = archive->AddInt32("location", fLocation)) != B_OK
		|| (error = archive->AddInt64("change count", fChangeCount)) != B_OK
		|| (error = archive->AddString("transaction",
			fTransactionDirectoryName)) != B_OK
		|| (error = archive->AddStrings("activate", fPackagesToActivate))
			!= B_OK
		|| (error = archive->AddStrings("deactivate", fPackagesToDeactivate))
			!= B_OK) {
		return error;
	}

	return B_OK;
}


/*static*/ BArchivable*
BActivationTransaction::Instantiate(BMessage* archive)
{
	if (validate_instantiation(archive, "BActivationTransaction"))
		return new(std::nothrow) BActivationTransaction(archive);
	return NULL;
}


/*static*/ status_t
BActivationTransaction::_ExtractStringList(BMessage* archive, const char* field,
	BStringList& _list)
{
	status_t error = archive->FindStrings(field, &_list);
	return error == B_NAME_NOT_FOUND ? B_OK : error;
		// If the field doesn't exist, that's OK.
}


}	// namespace BPrivate
}	// namespace BPackageKit
