/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef _PACKAGE__PRIVATE__DAEMON_CLIENT_H_
#define _PACKAGE__PRIVATE__DAEMON_CLIENT_H_


#include <Messenger.h>
#include <package/PackageDefs.h>
#include <String.h>

#include <package/DaemonDefs.h>


class BDirectory;


namespace BPackageKit {


class BInstallationLocationInfo;
class BPackageInfoSet;


namespace BPrivate {


class BActivationTransaction;


class BDaemonClient {
public:
			class BCommitTransactionResult;

public:
								BDaemonClient();
								~BDaemonClient();

			status_t			GetInstallationLocationInfo(
									BPackageInstallationLocation location,
									BInstallationLocationInfo& _info);
			status_t			CommitTransaction(
									const BActivationTransaction& transaction,
									BCommitTransactionResult& _result);
									// B_OK only means _result is initialized,
									// not the success of committing the
									// transaction

			status_t			CreateTransaction(
									BPackageInstallationLocation location,
									BActivationTransaction& _transaction,
									BDirectory& _transactionDirectory);

private:
			status_t			_InitMessenger();
			status_t			_ExtractPackageInfoSet(const BMessage& message,
									const char* field, BPackageInfoSet& _infos);

private:
			BMessenger			fDaemonMessenger;
};


class BDaemonClient::BCommitTransactionResult {
public:
								BCommitTransactionResult();
								BCommitTransactionResult(int32 error,
									const BString& errorMessage,
									const BString& errorPackage,
									const BString& oldStateDirectory);
								~BCommitTransactionResult();

			void				SetTo(int32 error, const BString& errorMessage,
									const BString& errorPackage,
									const BString& oldStateDirectory);

			status_t			Error() const;
			BDaemonError		DaemonError() const;
									// may be B_DAEMON_OK, even if Error() is
									// != B_OK, then Error() is as specific as
									// is known
			const BString&		ErrorMessage() const;
									// may be empty, even on error
			const BString&		ErrorPackage() const;
									// may be empty, even on error

			BString				FullErrorMessage() const;

			const BString&		OldStateDirectory() const;

private:
			int32				fError;
			BString				fErrorMessage;
			BString				fErrorPackage;
			BString				fOldStateDirectory;
};


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__DAEMON_CLIENT_H_
