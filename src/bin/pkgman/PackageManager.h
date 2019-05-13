/*
 * Copyright 2013-2015, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 *		Rene Gollent <rene@gollent.com>
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H


#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "DecisionProvider.h"


using namespace BPackageKit;
using BPackageKit::BPrivate::BDaemonClient;
using BManager::BPrivate::BPackageManager;


class PackageManager : public BPackageManager,
	private BPackageManager::UserInteractionHandler {
public:
								PackageManager(
									BPackageInstallationLocation location,
									bool interactive = true);
								~PackageManager();

			void				SetInteractive(bool interactive);

	virtual	void				JobFailed(BSupportKit::BJob* job);

private:
	// UserInteractionHandler
	virtual	void				HandleProblems();
	virtual	void				ConfirmChanges(bool fromMostSpecific);

	virtual	void				Warn(status_t error, const char* format, ...);


	virtual	void				ProgressPackageDownloadStarted(
									const char* packageName);
	virtual	void				ProgressPackageDownloadActive(
									const char* packageName,
									float completionPercentage,
									off_t bytes, off_t totalBytes);
	virtual	void				ProgressPackageDownloadComplete(
									const char* packageName);
	virtual	void				ProgressPackageChecksumStarted(
									const char* packageName);
	virtual	void				ProgressPackageChecksumComplete(
									const char* packageName);

	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository);
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const BCommitTransactionResult& result);
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository);

private:
			void				_PrintResult(InstalledRepository&
									installationRepository);

private:
			DecisionProvider	fDecisionProvider;
			BPackageManager::ClientInstallationInterface
									fClientInstallationInterface;
			bool				fInteractive;

			bool				fShowProgress;
			off_t				fLastBytes;
			bigtime_t			fLastRateCalcTime;
			float				fDownloadRate;
};


#endif	// PACKAGE_MANAGER_H
