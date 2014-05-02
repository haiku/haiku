/*
 * Copyright 2013-2014, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */


#include <package/DaemonClient.h>

#include <time.h>

#include <Directory.h>
#include <Entry.h>
#include <package/InstallationLocationInfo.h>
#include <package/PackageInfo.h>

#include <package/ActivationTransaction.h>
#include <package/PackagesDirectoryDefs.h>


namespace BPackageKit {
namespace BPrivate {


// #pragma mark - BCommitTransactionResult


BDaemonClient::BDaemonClient()
	:
	fDaemonMessenger()
{
}


BDaemonClient::~BDaemonClient()
{
}


status_t
BDaemonClient::GetInstallationLocationInfo(
	BPackageInstallationLocation location, BInstallationLocationInfo& _info)
{
	status_t error = _InitMessenger();
	if (error != B_OK)
		return error;

	// send the request
	BMessage request(B_MESSAGE_GET_INSTALLATION_LOCATION_INFO);
	error = request.AddInt32("location", location);
	if (error != B_OK)
		return error;

	BMessage reply;
	fDaemonMessenger.SendMessage(&request, &reply);
	if (reply.what != B_MESSAGE_GET_INSTALLATION_LOCATION_INFO_REPLY)
		return B_ERROR;

	// extract the location info
	int32 baseDirectoryDevice;
	int64 baseDirectoryNode;
	int32 packagesDirectoryDevice;
	int64 packagesDirectoryNode;
	BString oldStateName;
	int64 changeCount;
	BPackageInfoSet latestActivePackages;
	BPackageInfoSet latestInactivePackages;
	BPackageInfoSet currentlyActivePackages;
	if ((error = reply.FindInt32("base directory device", &baseDirectoryDevice))
			!= B_OK
		|| (error = reply.FindInt64("base directory node", &baseDirectoryNode))
			!= B_OK
		|| (error = reply.FindInt32("packages directory device",
			&packagesDirectoryDevice)) != B_OK
		|| (error = reply.FindInt64("packages directory node",
			&packagesDirectoryNode)) != B_OK
		|| (error = _ExtractPackageInfoSet(reply, "latest active packages",
			latestActivePackages)) != B_OK
		|| (error = _ExtractPackageInfoSet(reply, "latest inactive packages",
			latestInactivePackages)) != B_OK
		|| (error = _ExtractPackageInfoSet(reply, "currently active packages",
			currentlyActivePackages)) != B_OK
		|| (error = reply.FindString("old state", &oldStateName)) != B_OK
		|| (error = reply.FindInt64("change count", &changeCount)) != B_OK) {
		return error;
	}

	_info.Unset();
	_info.SetLocation(location);
	_info.SetBaseDirectoryRef(node_ref(baseDirectoryDevice, baseDirectoryNode));
	_info.SetPackagesDirectoryRef(
		node_ref(packagesDirectoryDevice, packagesDirectoryNode));
	_info.SetLatestActivePackageInfos(latestActivePackages);
	_info.SetLatestInactivePackageInfos(latestInactivePackages);
	_info.SetCurrentlyActivePackageInfos(currentlyActivePackages);
	_info.SetOldStateName(oldStateName);
	_info.SetChangeCount(changeCount);

	return B_OK;
}


status_t
BDaemonClient::CommitTransaction(const BActivationTransaction& transaction,
	BCommitTransactionResult& _result)
{
	if (transaction.InitCheck() != B_OK)
		return B_BAD_VALUE;

	status_t error = _InitMessenger();
	if (error != B_OK)
		return error;

	// send the request
	BMessage request(B_MESSAGE_COMMIT_TRANSACTION);
	error = transaction.Archive(&request);
	if (error != B_OK)
		return error;

	BMessage reply;
	fDaemonMessenger.SendMessage(&request, &reply);
	if (reply.what != B_MESSAGE_COMMIT_TRANSACTION_REPLY)
		return B_ERROR;

	// extract the result
	int32 requestError;
	error = reply.FindInt32("error", &requestError);
	if (error != B_OK)
		return error;

	BString errorMessage;
	BString errorPackage;
	BString oldStateDirectory;
	if (requestError == 0) {
		error = reply.FindString("old state", &oldStateDirectory);
		if (error != B_OK)
			return error;
	} else {
		reply.FindString("error message", &errorMessage);
		reply.FindString("error package", &errorPackage);
	}

	_result.SetTo(requestError, errorMessage, errorPackage, oldStateDirectory);
	return B_OK;
		// Even on error. B_OK just indicates that we have initialized _result.
}


status_t
BDaemonClient::CreateTransaction(BPackageInstallationLocation location,
	BActivationTransaction& _transaction, BDirectory& _transactionDirectory)
{
	// get an info for the location
	BInstallationLocationInfo info;
	status_t error = GetInstallationLocationInfo(location, info);
	if (error != B_OK)
		return error;

	// open admin directory
	entry_ref entryRef;
	entryRef.device = info.PackagesDirectoryRef().device;
	entryRef.directory = info.PackagesDirectoryRef().node;
	error = entryRef.set_name(PACKAGES_DIRECTORY_ADMIN_DIRECTORY);
	if (error != B_OK)
		return error;

	BDirectory adminDirectory;
	error = adminDirectory.SetTo(&entryRef);
	if (error != B_OK)
		return error;

	// create a transaction directory
	int uniqueId = 1;
	BString directoryName;
	for (;; uniqueId++) {
		directoryName.SetToFormat("transaction-%d", uniqueId);
		if (directoryName.IsEmpty())
			return B_NO_MEMORY;

		error = adminDirectory.CreateDirectory(directoryName,
			&_transactionDirectory);
		if (error == B_OK)
			break;
		if (error != B_FILE_EXISTS)
			return error;
	}

	// init the transaction
	error = _transaction.SetTo(location, info.ChangeCount(), directoryName);
	if (error != B_OK) {
		BEntry entry;
		_transactionDirectory.GetEntry(&entry);
		_transactionDirectory.Unset();
		if (entry.InitCheck() == B_OK)
			entry.Remove();
		return error;
	}

	return B_OK;
}


status_t
BDaemonClient::_InitMessenger()
{
	if (fDaemonMessenger.IsValid())
		return B_OK;

		// get the package daemon's address
	status_t error;
	fDaemonMessenger = BMessenger(B_PACKAGE_DAEMON_APP_SIGNATURE, -1, &error);
	return error;
}


status_t
BDaemonClient::_ExtractPackageInfoSet(const BMessage& message,
	const char* field, BPackageInfoSet& _infos)
{
	// get the number of items
	type_code type;
	int32 count;
	if (message.GetInfo(field, &type, &count) != B_OK) {
		// the field is missing
		return B_OK;
	}
	if (type != B_MESSAGE_TYPE)
		return B_BAD_DATA;

	for (int32 i = 0; i < count; i++) {
		BMessage archive;
		status_t error = message.FindMessage(field, i, &archive);
		if (error != B_OK)
			return error;

		BPackageInfo info(&archive, &error);
		if (error != B_OK)
			return error;

		error = _infos.AddInfo(info);
		if (error != B_OK)
			return error;
	}

	return B_OK;
}


// #pragma mark - BCommitTransactionResult


BDaemonClient::BCommitTransactionResult::BCommitTransactionResult()
	:
	fError(B_NO_INIT),
	fErrorMessage(),
	fErrorPackage(),
	fOldStateDirectory()
{
}


BDaemonClient::BCommitTransactionResult::BCommitTransactionResult(int32 error,
	const BString& errorMessage, const BString& errorPackage,
	const BString& oldStateDirectory)
	:
	fError(error),
	fErrorMessage(errorMessage),
	fErrorPackage(errorPackage),
	fOldStateDirectory(oldStateDirectory)
{
}


BDaemonClient::BCommitTransactionResult::~BCommitTransactionResult()
{
}


void
BDaemonClient::BCommitTransactionResult::SetTo(int32 error,
	const BString& errorMessage, const BString& errorPackage,
	const BString& oldStateDirectory)
{
	fError = error;
	fErrorMessage = errorMessage;
	fErrorPackage = errorPackage;
	fOldStateDirectory = oldStateDirectory;
}


status_t
BDaemonClient::BCommitTransactionResult::Error() const
{
	return fError <= 0 ? fError : B_ERROR;
}


BDaemonError
BDaemonClient::BCommitTransactionResult::DaemonError() const
{
	return fError > 0 ? (BDaemonError)fError : B_DAEMON_OK;
}


const BString&
BDaemonClient::BCommitTransactionResult::ErrorMessage() const
{
	return fErrorMessage;
}


const BString&
BDaemonClient::BCommitTransactionResult::ErrorPackage() const
{
	return fErrorPackage;
}


BString
BDaemonClient::BCommitTransactionResult::FullErrorMessage() const
{
	if (fError == 0)
		return "no error";

	const char* errorString;
	if (fError > 0) {
		switch ((BDaemonError)fError) {
			case B_DAEMON_INSTALLATION_LOCATION_BUSY:
				errorString = "another package operation already in progress";
				break;
			case B_DAEMON_CHANGE_COUNT_MISMATCH:
				errorString = "transaction out of date";
				break;
			case B_DAEMON_BAD_REQUEST:
				errorString = "invalid transaction";
				break;
			case B_DAEMON_NO_SUCH_PACKAGE:
				errorString = "no such package";
				break;
			case B_DAEMON_PACKAGE_ALREADY_EXISTS:
				errorString = "package already exists";
				break;
			case B_DAEMON_OK:
			default:
				errorString = "unknown error";
				break;
		}
	} else
		errorString = strerror(fError);

	BString result;
	if (!fErrorMessage.IsEmpty()) {
		result = fErrorMessage;
		result << ": ";
	}

	result << errorString;

	if (!fErrorPackage.IsEmpty())
		result << ", package: \"" << fErrorPackage << '"';

	return result;
}


const BString&
BDaemonClient::BCommitTransactionResult::OldStateDirectory() const
{
	return fOldStateDirectory;
}


}	// namespace BPrivate
}	// namespace BPackageKit
