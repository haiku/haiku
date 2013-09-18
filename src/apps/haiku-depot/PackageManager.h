/*
 * Copyright 2013, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2011, Ingo Weinhold, <ingo_weinhold@gmx.de>
 * Copyright 2013, Rene Gollent, <rene@gollent.com>
 *
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_MANAGER_H
#define PACKAGE_MANAGER_H

#include <package/DaemonClient.h>
#include <package/manager/PackageManager.h>

#include "DecisionProvider.h"
#include "JobStateListener.h"
#include "PackageInfo.h"


class PackageAction : public BReferenceable {
public:
								PackageAction(const PackageInfo& package);
	virtual						~PackageAction();

	virtual const char*			Label() const = 0;

	// TODO: Perform() needs to be passed a progress listener
	// and it needs a mechanism to report and react to errors. The
	// Package Kit supports this stuff already.
	virtual status_t			Perform() = 0;

			const PackageInfo&	Package() const
									{ return fPackage; }

private:
			PackageInfo			fPackage;
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
	virtual	PackageActionList	GetPackageActions(const PackageInfo& package);

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
			DecisionProvider	fDecisionProvider;
			JobStateListener	fJobStateListener;
			BContext			fContext;
			BPackageManager::ClientInstallationInterface
									fClientInstallationInterface;
};

#endif // PACKAGE_MANAGER_H
