/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Ingo Weinhold <ingo_weinhold@gmx.de>
 * 		Stephan Aßmus <superstippi@gmx.de>
 * 		Rene Gollent, reneGollent.com.
 */


#include "PackageManager.h"

#include <Catalog.h>

#include <package/DownloadFileRequest.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverProblem.h>
#include <package/solver/SolverProblemSolution.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PackageManager"


using BPackageKit::BRefreshRepositoryRequest;
using BPackageKit::DownloadFileRequest;
using namespace BPackageKit::BPrivate;


// #pragma mark - PackageAction


PackageAction::PackageAction(const PackageInfo& package)
	:
	fPackage(package)
{
}


PackageAction::~PackageAction()
{
}


// #pragma mark - InstallPackageAction


class InstallPackageAction : public PackageAction {
public:
	InstallPackageAction(const PackageInfo& package)
		:
		PackageAction(package)
	{
	}

	virtual const char* Label() const
	{
		return B_TRANSLATE("Install");
	}

	virtual status_t Perform()
	{
		// TODO: Trigger asynchronous installation of the package
		return B_ERROR;
	}
};


// #pragma mark - PackageManager


PackageManager::PackageManager(BPackageInstallationLocation location)
	:
	BPackageManager(location),
	BPackageManager::UserInteractionHandler(),
	fDecisionProvider(),
	fJobStateListener(),
	fContext(fDecisionProvider, fJobStateListener),
	fClientInstallationInterface()
{
	fInstallationInterface = &fClientInstallationInterface;
	fRequestHandler = this;
	fUserInteractionHandler = this;
}


PackageManager::~PackageManager()
{
}


PackageState
PackageManager::GetPackageState(const PackageInfo& package)
{
	// TODO: Fetch information from the PackageKit
	return NONE;
}


PackageActionList
PackageManager::GetPackageActions(const PackageInfo& package)
{
	PackageActionList actionList;

	// TODO: Actually fetch applicable actions for this package.
	// If the package is installed and active, it can be
	//		* uninstalled
	//		* deactivated
	// If the package is installed and inactive, it can be
	//		* uninstalled
	//		* activated
	// If the package is not installed, it can be
	//		* installed (and it will be automatically activated)
	actionList.Add(PackageActionRef(new InstallPackageAction(package), true));

	return actionList;
}


status_t
PackageManager::RefreshRepository(const BRepositoryConfig& repoConfig)
{
	return BRefreshRepositoryRequest(fContext, repoConfig).Process();
}


status_t
PackageManager::DownloadPackage(const BString& fileURL,
	const BEntry& targetEntry, const BString& checksum)
{
	return DownloadFileRequest(fContext, fileURL, targetEntry, checksum)
		.Process();
}


void
PackageManager::HandleProblems()
{
	// TODO: implement
}


void
PackageManager::ConfirmChanges(bool fromMostSpecific)
{
	// TODO: implement
}


void
PackageManager::Warn(status_t error, const char* format, ...)
{
	// TODO: implement
}


void
PackageManager::ProgressStartApplyingChanges(InstalledRepository& repository)
{
	// TODO: implement
}


void
PackageManager::ProgressTransactionCommitted(InstalledRepository& repository,
	const char* transactionDirectoryName)
{
	// TODO: implement
}


void
PackageManager::ProgressApplyingChangesDone(InstalledRepository& repository)
{
	// TODO: implement
}


void
PackageManager::_PrintResult(InstalledRepository& installationRepository)
{
	// TODO: implement
}
