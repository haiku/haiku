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


class BCommitTransactionResult;
class BInstallationLocationInfo;
class BPackageInfoSet;


namespace BPrivate {


class BActivationTransaction;


class BDaemonClient {
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


}	// namespace BPrivate
}	// namespace BPackageKit


#endif	// _PACKAGE__PRIVATE__DAEMON_CLIENT_H_
