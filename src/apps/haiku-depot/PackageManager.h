/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <Locker.h>

#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "PackageInfo.h"


namespace BPackageKit {
	class BSolverPackage;
}


class PackageManager;
class ProblemWindow;
class ResultWindow;


class PackageAction : public BReferenceable {
public:
								PackageAction(PackageInfoRef package,
									PackageManager* manager);
	virtual						~PackageAction();

	virtual const char*			Label() const = 0;

	// TODO: Perform() needs to be passed a progress listener
	// and it needs a mechanism to report and react to errors. The
	// Package Kit supports this stuff already.
	virtual status_t			Perform() = 0;

			PackageInfoRef		Package() const
									{ return fPackage; }

protected:
			PackageManager*		fPackageManager;

private:
			PackageInfoRef		fPackage;
};


typedef BReference<PackageAction> PackageActionRef;
typedef List<PackageActionRef, false> PackageActionList;

using BPackageKit::BContext;
using BPackageKit::BPackageInstallationLocation;
using BPackageKit::BRepositoryConfig;
using BPackageKit::BPrivate::BDaemonClient;
using BPackageKit::BManager::BPrivate::BPackageManager;


class PackageManager : public BPackageManager,
	private BPackageManager::RequestHandler,
	private BPackageManager::UserInteractionHandler {
public:
								PackageManager(
									BPackageInstallationLocation location);
	virtual						~PackageManager();

	virtual	PackageState		GetPackageState(const PackageInfo& package);
	virtual	PackageActionList	GetPackageActions(PackageInfoRef package);

			status_t			SchedulePackageActions(
									PackageActionList& list);

			void				SetCurrentActionPackage(
									PackageInfoRef package,
									bool install);

			void				ClearCurrentActionPackage();

private:
	// RequestHandler
	virtual	status_t			RefreshRepository(
									const BRepositoryConfig& repoConfig);
	virtual	status_t			DownloadPackage(const BString& fileURL,
                                    const BEntry& targetEntry,
                                    const BString& checksum);

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
		static	status_t		_PackageActionWorker(void* arg);

				bool			_AddResults(
									BPackageManager::InstalledRepository&
										repository,
									ResultWindow* window);

				BPackageKit::BSolverPackage*
								_GetSolverPackage(PackageInfoRef package);

private:
			DecisionProvider	fDecisionProvider;
			JobStateListener	fJobStateListener;
			BContext			fContext;
			BPackageManager::ClientInstallationInterface
								fClientInstallationInterface;

			thread_id			fPendingActionsWorker;
			PackageActionList	fPendingActions;
			BLocker				fPendingActionsLock;
			sem_id				fPendingActionsSem;
			ProblemWindow*		fProblemWindow;
			BPackageKit::BSolverPackage*
								fCurrentInstallPackage;
			BPackageKit::BSolverPackage*
								fCurrentUninstallPackage;
};

#endif // PACKAGE_MANAGER_H
