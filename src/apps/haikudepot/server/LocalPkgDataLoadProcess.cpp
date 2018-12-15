/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "LocalPkgDataLoadProcess.h"

#include <map>
#include <vector>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Autolock.h>
#include <Catalog.h>
#include <Roster.h>
#include <StringList.h>

#include "AppUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "PackageInfo.h"
#include "PackageManager.h"
#include "RepositoryUrlUtils.h"

#include <package/Context.h>
#include <package/manager/Exceptions.h>
#include <package/manager/RepositoryBuilder.h>
#include <package/PackageRoster.h>
#include "package/RepositoryCache.h"
#include <package/RefreshRepositoryRequest.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverResult.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocalPkgDataLoadProcess"


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


typedef std::map<BString, PackageInfoRef> PackageInfoMap;


/*!
	\param packageInfoListener is assigned to each package model object.
*/

LocalPkgDataLoadProcess::LocalPkgDataLoadProcess(
	PackageInfoListener* packageInfoListener,
	Model *model, bool force)
	:
	AbstractProcess(),
	fModel(model),
	fForce(force),
	fPackageInfoListener(packageInfoListener)
{
}


LocalPkgDataLoadProcess::~LocalPkgDataLoadProcess()
{
}


const char*
LocalPkgDataLoadProcess::Name() const
{
	return "LocalPkgDataLoadProcess";
}


const char*
LocalPkgDataLoadProcess::Description() const
{
	return B_TRANSLATE("Reading repository data");
}


/*! The contents of this method implementation have been 'lifted and shifted'
    from MainWindow.cpp in order that the logic fits into the background
    loading processes.  The code needs to be broken up into methods with some
    sort of a state object carrying the state of the process.  As part of this,
    better error handling and error reporting would also be advantageous.
*/

status_t
LocalPkgDataLoadProcess::RunInternal()
{
 	if (Logger::IsDebugEnabled())
 		printf("[%s] will refresh the package list\n", Name());

 	BPackageRoster roster;
 	BStringList repositoryNames;

 	status_t result = roster.GetRepositoryNames(repositoryNames);

 	if (result != B_OK)
 		return result;

 	std::vector<DepotInfo> depots(repositoryNames.CountStrings());
 	for (int32 i = 0; i < repositoryNames.CountStrings(); i++) {
 		const BString& repoName = repositoryNames.StringAt(i);
 		DepotInfo depotInfo = DepotInfo(repoName);

 		BRepositoryConfig repoConfig;
 		status_t getRepositoryConfigStatus = roster.GetRepositoryConfig(
 			repoName, &repoConfig);

 		if (getRepositoryConfigStatus == B_OK) {
 			depotInfo.SetURL(repoConfig.URL());

 			if (Logger::IsDebugEnabled()) {
 				printf("[%s] local repository [%s] info;\n"
 					" * url [%s]\n", Name(), repoName.String(),
 					repoConfig.URL().String());
 			}
 		} else {
 			printf("[%s] unable to obtain the repository config for local "
 				"repository '%s'; %s\n", Name(),
 				repoName.String(), strerror(getRepositoryConfigStatus));
 		}

 		depots[i] = depotInfo;
 	}

 	PackageManager manager(B_PACKAGE_INSTALLATION_LOCATION_HOME);
 	try {
 		manager.Init(PackageManager::B_ADD_INSTALLED_REPOSITORIES
 			| PackageManager::B_ADD_REMOTE_REPOSITORIES);
 	} catch (BException ex) {
 		BString message(B_TRANSLATE("An error occurred while "
 			"initializing the package manager: %message%"));
 		message.ReplaceFirst("%message%", ex.Message());
 		_NotifyError(message.String());
 		return B_ERROR;
 	}

 	BObjectList<BSolverPackage> packages;
 	result = manager.Solver()->FindPackages("",
 		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME
 			| BSolver::B_FIND_IN_SUMMARY | BSolver::B_FIND_IN_DESCRIPTION
 			| BSolver::B_FIND_IN_PROVIDES,
 		packages);
 	if (result != B_OK) {
 		BString message(B_TRANSLATE("An error occurred while "
 			"obtaining the package list: %message%"));
 		message.ReplaceFirst("%message%", strerror(result));
 		_NotifyError(message.String());
 		return B_ERROR;
 	}

 	if (packages.IsEmpty())
 		return B_ERROR;

 	PackageInfoMap foundPackages;
 		// if a given package is installed locally, we will potentially
 		// get back multiple entries, one for each local installation
 		// location, and one for each remote repository the package
 		// is available in. The above map is used to ensure that in such
 		// cases we consolidate the information, rather than displaying
 		// duplicates
 	PackageInfoMap remotePackages;
 		// any package that we find in a remote repository goes in this map.
 		// this is later used to discern which packages came from a local
 		// installation only, as those must be handled a bit differently
 		// upon uninstallation, since we'd no longer be able to pull them
 		// down remotely.
 	BStringList systemFlaggedPackages;
 		// any packages flagged as a system package are added to this list.
 		// such packages cannot be uninstalled, nor can any of their deps.
 	PackageInfoMap systemInstalledPackages;
 		// any packages installed in system are added to this list.
 		// This is later used for dependency resolution of the actual
 		// system packages in order to compute the list of protected
 		// dependencies indicated above.

 	for (int32 i = 0; i < packages.CountItems(); i++) {
 		BSolverPackage* package = packages.ItemAt(i);
 		const BPackageInfo& repoPackageInfo = package->Info();
 		const BString repositoryName = package->Repository()->Name();
 		PackageInfoRef modelInfo;
 		PackageInfoMap::iterator it = foundPackages.find(
 			repoPackageInfo.Name());
 		if (it != foundPackages.end())
 			modelInfo.SetTo(it->second);
 		else {
 			// Add new package info
 			modelInfo.SetTo(new(std::nothrow) PackageInfo(repoPackageInfo),
 				true);

 			if (modelInfo.Get() == NULL)
 				return B_ERROR;

 			foundPackages[repoPackageInfo.Name()] = modelInfo;
 		}

 		// The package list here considers those packages that are installed
 		// in the system as well as those that exist in remote repositories.
 		// It is better if the 'depot name' is from the remote repository
 		// because then it will be possible to perform a rating on it later.

 		if (modelInfo->DepotName().IsEmpty()
 			|| modelInfo->DepotName() == REPOSITORY_NAME_SYSTEM
 			|| modelInfo->DepotName() == REPOSITORY_NAME_INSTALLED) {
 			modelInfo->SetDepotName(repositoryName);
 		}

 		modelInfo->AddListener(fPackageInfoListener);

 		BSolverRepository* repository = package->Repository();
 		BPackageManager::RemoteRepository* remoteRepository =
 			dynamic_cast<BPackageManager::RemoteRepository*>(repository);

 		if (remoteRepository != NULL) {

 			std::vector<DepotInfo>::iterator it;

 			for (it = depots.begin(); it != depots.end(); it++) {
 				if (RepositoryUrlUtils::EqualsNormalized(
 					it->URL(), remoteRepository->Config().URL())) {
 					break;
 				}
 			}

 			if (it == depots.end()) {
 				if (Logger::IsDebugEnabled()) {
 					printf("pkg [%s] repository [%s] not recognized"
 						" --> ignored\n",
 						modelInfo->Name().String(), repositoryName.String());
 				}
 			} else {
 				it->AddPackage(modelInfo);

 				if (Logger::IsTraceEnabled()) {
 					printf("pkg [%s] assigned to [%s]\n",
 						modelInfo->Name().String(), repositoryName.String());
 				}
 			}

 			remotePackages[modelInfo->Name()] = modelInfo;
 		} else {
 			if (repository == static_cast<const BSolverRepository*>(
 					manager.SystemRepository())) {
 				modelInfo->AddInstallationLocation(
 					B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
 				if (!modelInfo->IsSystemPackage()) {
 					systemInstalledPackages[repoPackageInfo.FileName()]
 						= modelInfo;
 				}
 			} else if (repository == static_cast<const BSolverRepository*>(
 					manager.HomeRepository())) {
 				modelInfo->AddInstallationLocation(
 					B_PACKAGE_INSTALLATION_LOCATION_HOME);
 			}
 		}

 		if (modelInfo->IsSystemPackage())
 			systemFlaggedPackages.Add(repoPackageInfo.FileName());
 	}

 	BAutolock lock(fModel->Lock());

 	if (fForce)
 		fModel->Clear();

 	// filter remote packages from the found list
 	// any packages remaining will be locally installed packages
 	// that weren't acquired from a repository
 	for (PackageInfoMap::iterator it = remotePackages.begin();
 			it != remotePackages.end(); it++) {
 		foundPackages.erase(it->first);
 	}

 	if (!foundPackages.empty()) {
 		BString repoName = B_TRANSLATE("Local");
 		depots.push_back(DepotInfo(repoName));

 		for (PackageInfoMap::iterator it = foundPackages.begin();
 				it != foundPackages.end(); ++it) {
 			depots.back().AddPackage(it->second);
 		}
 	}

 	{
 		std::vector<DepotInfo>::iterator it;

 		for (it = depots.begin(); it != depots.end(); it++) {
 			if (fModel->HasDepot(it->Name()))
 				fModel->SyncDepot(*it);
 			else
 				fModel->AddDepot(*it);
 		}
 	}

 	// compute the OS package dependencies
 	try {
 		// create the solver
 		BSolver* solver;
 		status_t error = BSolver::Create(solver);
 		if (error != B_OK)
 			throw BFatalErrorException(error, "Failed to create solver.");

 		ObjectDeleter<BSolver> solverDeleter(solver);
 		BPath systemPath;
 		error = find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &systemPath);
 		if (error != B_OK) {
 			throw BFatalErrorException(error,
 				"Unable to retrieve system packages directory.");
 		}

 		// add the "installed" repository with the given packages
 		BSolverRepository installedRepository;
 		{
 			BRepositoryBuilder installedRepositoryBuilder(installedRepository,
 				REPOSITORY_NAME_INSTALLED);
 			for (int32 i = 0; i < systemFlaggedPackages.CountStrings(); i++) {
 				BPath packagePath(systemPath);
 				packagePath.Append(systemFlaggedPackages.StringAt(i));
 				installedRepositoryBuilder.AddPackage(packagePath.Path());
 			}
 			installedRepositoryBuilder.AddToSolver(solver, true);
 		}

 		// add system repository
 		BSolverRepository systemRepository;
 		{
 			BRepositoryBuilder systemRepositoryBuilder(systemRepository,
 				REPOSITORY_NAME_SYSTEM);
 			for (PackageInfoMap::iterator it = systemInstalledPackages.begin();
 					it != systemInstalledPackages.end(); it++) {
 				BPath packagePath(systemPath);
 				packagePath.Append(it->first);
 				systemRepositoryBuilder.AddPackage(packagePath.Path());
 			}
 			systemRepositoryBuilder.AddToSolver(solver, false);
 		}

 		// solve
 		error = solver->VerifyInstallation();
 		if (error != B_OK) {
 			throw BFatalErrorException(error, "Failed to compute packages to "
 				"install.");
 		}

 		BSolverResult solverResult;
 		error = solver->GetResult(solverResult);
 		if (error != B_OK) {
 			throw BFatalErrorException(error, "Failed to retrieve system "
 				"package dependency list.");
 		}

 		for (int32 i = 0; const BSolverResultElement* element
 				= solverResult.ElementAt(i); i++) {
 			BSolverPackage* package = element->Package();
 			if (element->Type() == BSolverResultElement::B_TYPE_INSTALL) {
 				PackageInfoMap::iterator it = systemInstalledPackages.find(
 					package->Info().FileName());
 				if (it != systemInstalledPackages.end())
 					it->second->SetSystemDependency(true);
 			}
 		}
 	} catch (BFatalErrorException ex) {
 		printf("Fatal exception occurred while resolving system dependencies: "
 			"%s, details: %s\n", strerror(ex.Error()), ex.Details().String());
 	} catch (BNothingToDoException) {
 		// do nothing
 	} catch (BException ex) {
 		printf("Exception occurred while resolving system dependencies: %s\n",
 			ex.Message().String());
 	} catch (...) {
 		printf("Unknown exception occurred while resolving system "
 			"dependencies.\n");
 	}

 	if (Logger::IsDebugEnabled())
 		printf("did refresh the package list\n");

 	return B_OK;
}


void
LocalPkgDataLoadProcess::_NotifyError(const BString& messageText) const
{
	printf("an error has arisen loading data of packages from local : %s\n",
		messageText.String());
	AppUtils::NotifySimpleError(
		B_TRANSLATE("Local Repository Load Error"),
		messageText);
}