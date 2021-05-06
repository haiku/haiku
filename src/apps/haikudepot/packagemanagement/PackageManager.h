/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 * Copyright 2017, Julian Harnath <julian.harnath@rwth-aachen.de>.
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>.
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <Locker.h>

#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "Collector.h"
#include "DecisionProvider.h"
#include "DeskbarLink.h"
#include "JobStateListener.h"
#include "PackageAction.h"
#include "PackageInfo.h"
#include "PackageProgressListener.h"


namespace BPackageKit {
	class BSolverPackage;
}

class PackageManager;
class ProblemWindow;
class ResultWindow;


using BPackageKit::BCommitTransactionResult;
using BPackageKit::BContext;
using BPackageKit::BPackageInstallationLocation;
using BPackageKit::BRepositoryConfig;
using BPackageKit::BPrivate::BDaemonClient;
using BPackageKit::BManager::BPrivate::BPackageManager;


class PackageManager : public BPackageManager,
	private BPackageManager::UserInteractionHandler {
public:
								PackageManager(
									BPackageInstallationLocation location);
	virtual						~PackageManager();

	virtual	PackageState		GetPackageState(const PackageInfo& package);
	virtual	void				CollectPackageActions(PackageInfoRef package,
									Collector<PackageActionRef>& actionList);

			void				SetCurrentActionPackage(
									PackageInfoRef package,
									bool install);

	virtual	status_t			RefreshRepository(
									const BRepositoryConfig& repoConfig);
	virtual	status_t			DownloadPackage(const BString& fileURL,
									const BEntry& targetEntry,
									const BString& checksum);

			void				AddProgressListener(
									PackageProgressListener* listener);
			void				RemoveProgressListener(
									PackageProgressListener* listener);

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
									const char* title);
	virtual	void				ProgressPackageChecksumComplete(
									const char* title);

	virtual	void				ProgressStartApplyingChanges(
									InstalledRepository& repository);
	virtual	void				ProgressTransactionCommitted(
									InstalledRepository& repository,
									const BCommitTransactionResult& result);
	virtual	void				ProgressApplyingChangesDone(
									InstalledRepository& repository);

private:
			PackageActionRef	_CreateUninstallPackageAction(
									const PackageInfoRef& package);
			PackageActionRef	_CreateInstallPackageAction(
									const PackageInfoRef& package);
			PackageActionRef	_CreateOpenPackageAction(
									const PackageInfoRef& package,
									const DeskbarLink& link);

			void				_CollectPackageActionsForActivatedOrInstalled(
									PackageInfoRef package,
									Collector<PackageActionRef>& actionList);
			bool				_AddResults(
									BPackageManager::InstalledRepository&
										repository,
									ResultWindow* window);
			void				_NotifyChangesConfirmed();

			BPackageKit::BSolverPackage*
								_GetSolverPackage(PackageInfoRef package);

private:
			DecisionProvider	fDecisionProvider;
			BPackageManager::ClientInstallationInterface
								fClientInstallationInterface;

			ProblemWindow*		fProblemWindow;
			BPackageKit::BSolverPackage*
								fCurrentInstallPackage;
			BPackageKit::BSolverPackage*
								fCurrentUninstallPackage;

			PackageProgressListenerList
								fPackageProgressListeners;
};

#endif // PACKAGE_MANAGER_H
