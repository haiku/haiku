/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H


#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "DecisionProvider.h"
#include "JobStateListener.h"


using namespace BPackageKit;
using BPackageKit::BPrivate::BDaemonClient;
using BManager::BPrivate::BPackageManager;


class PackageManager : public BPackageManager,
	private BPackageManager::UserInteractionHandler {
public:
								PackageManager(
									BPackageInstallationLocation location);
								~PackageManager();

private:
	// UserInteractionHandler
	virtual	void				HandleProblems();
	virtual	void				ConfirmChanges(bool fromMostSpecific);

	virtual	void				Warn(status_t error, const char* format, ...);
	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository);
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const char* transactionDirectoryName);
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository);

private:
			void				_PrintResult(InstalledRepository&
									installationRepository);

private:
			DecisionProvider	fDecisionProvider;
			JobStateListener	fJobStateListener;
			BPackageManager::ClientInstallationInterface
									fClientInstallationInterface;
};


#endif	// PACKAGE_MANAGER_H
