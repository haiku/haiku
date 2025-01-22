/*
 * Copyright 2018-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * Copyright 2013-2014, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 2013, Rene Gollent, rene@gollent.com.
 * Copyright 2013, Ingo Weinhold, ingo_weinhold@gmx.de.

 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `MainWindow.cpp` and
 * copyrights have been latterly been carried across in 2021.
 */


#include "LocalPkgDataLoadProcess.h"

#include <map>
#include <set>
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
#include "PackageKitUtils.h"
#include "PackageManager.h"
#include "PackageUtils.h"
#include "StorageUtils.h"

#include "package/RepositoryCache.h"
#include <package/Context.h>
#include <package/PackageRoster.h>
#include <package/RefreshRepositoryRequest.h>
#include <package/manager/Exceptions.h>
#include <package/manager/RepositoryBuilder.h>
#include <package/solver/SolverPackage.h>
#include <package/solver/SolverResult.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "LocalPkgDataLoadProcess"


using namespace BPackageKit;
using namespace BPackageKit::BManager::BPrivate;


class LocalPkgDataLoadProcessUtilsData {

public:
			int32				CountPackages() const;
			void				AddPackage(PackageInfoRef packageInfo);
			const PackageInfoRef
								PackageByName(const BString& name) const;

			int32				CountDepots() const;
			void				AddDepot(DepotInfoRef depotInfo);
			const DepotInfoRef	DepotByName(const BString& name) const;

			int32				CountSolverPackages() const;
			void				AddSolverPackage(const BSolverPackage* solverPackage);

			void				AddRemotePackageName(const BString& name);
			bool				IsRemotePackageName(const BString& name) const;

			void				AddSystemFlaggedPackageFilename(const BString& filename);

			void				AddSystemInstalledPackage(const BPackageInfo& solverPackageInfo);
			const PackageInfoRef
								PackageBySystemInstalledFilename(const BString& filename);

public:

			std::map<BString, DepotInfoRef>
								fDepots;

			std::vector<const BSolverPackage*>
								fSolverPackages;

			std::map<BString, PackageInfoRef>
								fFoundPackages;
		// if a given package is installed locally, we will potentially
		// get back multiple entries, one for each local installation
		// location, and one for each remote repository the package
		// is available in. The above map is used to ensure that in such
		// cases we consolidate the information, rather than displaying
		// duplicates.

			std::set<BString>	fRemotePackageNames;
		// any package that we find in a remote repository goes in this map.
		// this is later used to discern which packages came from a local
		// installation only, as those must be handled a bit differently
		// upon uninstallation, since we'd no longer be able to pull them
		// down remotely.

			std::set<BString>	fSystemFlaggedPackageFilenames;
		// any packages flagged as a system package are added to this list.
		// such packages cannot be uninstalled, nor can any of their deps.

			std::map<BString, BString>
								fSystemInstalledFilenamesToPackageNames;
		// any packages installed in system are added to this list.
		// This is later used for dependency resolution of the actual
		// system packages in order to compute the list of protected
		// dependencies indicated above.
};


class LocalPkgDataLoadProcessUtils {
public:
	static	void				_NotifyError(const BString& messageText);

	static	status_t			_CollateDepotInfos(LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_InitPackageManager(PackageManager& manager);
	static	status_t			_CollateSolverPackages(PackageManager& manager,
									LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_CollatePackagesData(PackageManager& manager,
									LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_CollatePackageData(PackageManager& manager,
									const BSolverPackage* solverPackage,
									LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_CollateLocalDepotAndPackageData(
									LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_EnhanceSystemPackages(
									LocalPkgDataLoadProcessUtilsData& data);
	static	status_t			_PopulateModel(LocalPkgDataLoadProcessUtilsData& data,
									Model* model, bool force);

	static	bool				_IsHomeRepository(PackageManager& manager,
									BSolverRepository* solverRepository);
	static	bool				_IsSystemRepository(PackageManager& manager,
									BSolverRepository* solverRepository);

	static	BString				_LogMessageFromFatalError(BFatalErrorException& ex);

};


/*!
	\param packageInfoListener is assigned to each package model object.
*/

LocalPkgDataLoadProcess::LocalPkgDataLoadProcess(Model* model, bool force)
	:
	AbstractProcess(),
	fModel(model),
	fForce(force)
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


/*!	The contents of this method implementation have been 'lifted and shifted'
	from MainWindow.cpp in order that the logic fits into the background
	loading processes.  The code needs to be broken up into methods with some
	sort of a state object carrying the state of the process.  As part of this,
	better error handling and error reporting would also be advantageous.
*/

status_t
LocalPkgDataLoadProcess::RunInternal()
{
	HDINFO("[%s] will refresh the package list", Name());

	status_t result = B_OK;
	LocalPkgDataLoadProcessUtilsData data;

	// Find all of the depots (repositories) that are known about in the system.

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_CollateDepotInfos(data);
		if (result == B_OK)
			HDINFO("[%s] did collate %" B_PRIi32 " depots", Name(), data.CountDepots());
		else
			HDERROR("[%s] failed to collate depot infos", Name());
	}

	PackageManager manager(B_PACKAGE_INSTALLATION_LOCATION_HOME);

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_InitPackageManager(manager);
		if (result == B_OK)
			HDINFO("[%s] did initialize the package manager", Name());
		else
			HDERROR("[%s] failed to initialize the package manager", Name());
	}

	// Collect all of the packages that are known about in the system.

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_CollateSolverPackages(manager, data);
		if (result == B_OK) {
			HDINFO("[%s] did collate %" B_PRIi32 " solver packages", Name(),
				data.CountSolverPackages());
		} else {
			HDERROR("[%s] failed to collate solver packages", Name());
		}
	}

	// Collect all of the solver packages into HaikuDepot `PackageInfo` objects for use in the
	// application.

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_CollatePackagesData(manager, data);
		if (result == B_OK)
			HDINFO("[%s] did collate %" B_PRIi32 " packages", Name(), data.CountPackages());
		else
			HDERROR("[%s] failed to collate packages", Name());
	}

	// Some of the installed packages may not have come from a remote repository. In this case
	// they will be collated under a fake Depot.

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_CollateLocalDepotAndPackageData(data);
		if (result == B_OK)
			HDINFO("[%s] did collate local depot and package data", Name());
		else
			HDERROR("[%s] failed to collate local depot and package data", Name());
	}

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_EnhanceSystemPackages(data);
		if (result == B_OK)
			HDINFO("[%s] did enhance system packages", Name());
		else
			HDERROR("[%s] failed to enhance system packages", Name());
	}

	if (result == B_OK) {
		result = LocalPkgDataLoadProcessUtils::_PopulateModel(data, fModel, fForce);
		if (result == B_OK)
			HDINFO("[%s] did populate model", Name());
		else
			HDERROR("[%s] failed to populate model", Name());
	}

	if (result == B_OK)
		HDINFO("did refresh the package list");
	else
		HDERROR("failed to refresh the package list");

	return result;
}


/*static*/ status_t
LocalPkgDataLoadProcessUtils::_CollateDepotInfos(LocalPkgDataLoadProcessUtilsData& data)
{
	BPackageRoster roster;
	BStringList repositoryNames;

	status_t result = roster.GetRepositoryNames(repositoryNames);

	if (result != B_OK)
		return result;

	for (int32 i = repositoryNames.CountStrings() - 1; i >= 0; i--) {
		const BString& repositoryName = repositoryNames.StringAt(i);
		DepotInfoBuilder depotInfoBuilder;
		depotInfoBuilder.WithName(repositoryName);

		BRepositoryConfig repoConfig;
		status_t getRepositoryConfigStatus
			= roster.GetRepositoryConfig(repositoryName, &repoConfig);

		if (getRepositoryConfigStatus == B_OK) {
			depotInfoBuilder.WithIdentifier(repoConfig.Identifier());
			HDDEBUG("local repository [%s] identifier; [%s]", repositoryName.String(),
				repoConfig.Identifier().String());
		} else {
			HDINFO("unable to obtain the repository config for local repository '%s'; %s",
				repositoryName.String(), strerror(getRepositoryConfigStatus));
		}

		data.AddDepot(depotInfoBuilder.BuildRef());
	}

	return result;
}


/*static*/ status_t
LocalPkgDataLoadProcessUtils::_InitPackageManager(PackageManager& manager)
{
	try {
		manager.Init(PackageManager::B_ADD_INSTALLED_REPOSITORIES
			| PackageManager::B_ADD_REMOTE_REPOSITORIES);
		return B_OK;
	} catch (BException& ex) {
		BString message(B_TRANSLATE("An error occurred while initializing the package manager: "
									"%message%"));
		message.ReplaceFirst("%message%", ex.Message());
		_NotifyError(message.String());
		return B_ERROR;
	}
}


/*static*/ status_t
LocalPkgDataLoadProcessUtils::_CollateSolverPackages(PackageManager& manager,
	LocalPkgDataLoadProcessUtilsData& data)
{
	BObjectList<BSolverPackage> packages;

	status_t result = manager.Solver()->FindPackages("",
		BSolver::B_FIND_CASE_INSENSITIVE | BSolver::B_FIND_IN_NAME | BSolver::B_FIND_IN_SUMMARY
			| BSolver::B_FIND_IN_DESCRIPTION | BSolver::B_FIND_IN_PROVIDES,
		packages);

	if (result != B_OK) {
		BString message(
			B_TRANSLATE("An error occurred while obtaining the package list: %message%"));
		message.ReplaceFirst("%message%", strerror(result));
		_NotifyError(message.String());
	} else if (packages.IsEmpty()) {
		HDERROR("no packages were found");
		result = B_ERROR;
	}

	if (result == B_OK) {
		for (int32 i = packages.CountItems() - 1; i >= 0; i--)
			data.AddSolverPackage(packages.ItemAt(i));
	}

	return result;
}


/*static*/ status_t
LocalPkgDataLoadProcessUtils::_CollatePackagesData(PackageManager& manager,
	LocalPkgDataLoadProcessUtilsData& data)
{
	std::vector<const BSolverPackage*>::iterator solverPackageIt;
	status_t result = B_OK;

	for (solverPackageIt = data.fSolverPackages.begin();
		solverPackageIt != data.fSolverPackages.end() && result == B_OK; solverPackageIt++) {
		const BSolverPackage* solverPackage = *solverPackageIt;
		result = _CollatePackageData(manager, solverPackage, data);
	}

	return result;
}

/*!	This method will collect the data from the `solverPackage` into the data in
	the supplied `data`.
*/

/*static*/ status_t
LocalPkgDataLoadProcessUtils::_CollatePackageData(PackageManager& manager,
	const BSolverPackage* solverPackage, LocalPkgDataLoadProcessUtilsData& data)
{
	status_t result = B_OK;
	const BPackageInfo& solverPackageInfo = solverPackage->Info();
	const BString repositoryName = solverPackage->Repository()->Name();
	const BString packageName = solverPackageInfo.Name();

	PackageCoreInfoBuilder coreInfoBuilder;
	PackageLocalizedTextBuilder localizedTextBuilder;
	PackageLocalInfoBuilder localInfoBuilder;
	PackageClassificationInfoRef classificationInfo;
	PackageUserRatingInfoRef userRatingInfo;
	PackageScreenshotInfoBuilder screenshotInfoBuilder;

	PackageInfoRef preexistingPackageInfo = data.PackageByName(packageName);

	// If there is an existing package found then build off the data from that package and add to
	// it; otherwise create new data off the package.

	if (preexistingPackageInfo.IsSet()) {

		coreInfoBuilder = PackageCoreInfoBuilder(preexistingPackageInfo->CoreInfo());
		localizedTextBuilder = PackageLocalizedTextBuilder(preexistingPackageInfo->LocalizedText());
		localInfoBuilder = PackageLocalInfoBuilder(preexistingPackageInfo->LocalInfo());
		classificationInfo
			= PackageClassificationInfoRef(preexistingPackageInfo->PackageClassificationInfo());
		userRatingInfo = PackageUserRatingInfoRef(preexistingPackageInfo->UserRatingInfo());
		screenshotInfoBuilder
			= PackageScreenshotInfoBuilder(preexistingPackageInfo->ScreenshotInfo());

		// The package list here considers those packages that are installed
		// in the system as well as those that exist in remote repositories.
		// It is better if the 'depot name' is from the remote repository
		// because then it will be possible to perform a rating on it later.

		BString preexistingPackageDepotName = PackageUtils::DepotName(preexistingPackageInfo);

		if (preexistingPackageDepotName.IsEmpty()
			|| preexistingPackageDepotName == REPOSITORY_NAME_SYSTEM
			|| preexistingPackageDepotName == REPOSITORY_NAME_INSTALLED) {
			coreInfoBuilder.WithDepotName(repositoryName);
		}

	} else {
		coreInfoBuilder
			= PackageCoreInfoBuilder()
				  .WithDepotName(repositoryName)
				  .WithArchitecture(solverPackageInfo.ArchitectureName())
				  .WithVersion(new PackageVersion(solverPackageInfo.Version()))
				  .WithPublisher(PackageKitUtils::CreatePublisherInfo(solverPackageInfo));

		localizedTextBuilder = PackageLocalizedTextBuilder()
								   .WithTitle(solverPackageInfo.Name())
								   .WithSummary(solverPackageInfo.Summary())
								   .WithDescription(solverPackageInfo.Description());

		// TODO: Retrieve local file size
		localInfoBuilder = PackageLocalInfoBuilder()
							   .WithFlags(solverPackageInfo.Flags())
							   .WithFileName(solverPackageInfo.FileName());
	}

	BSolverRepository* solverRepository = solverPackage->Repository();
	BPackageManager::RemoteRepository* solverRemoteRepository
		= dynamic_cast<BPackageManager::RemoteRepository*>(solverRepository);
	uint32 solverPackageFlags = solverPackageInfo.Flags();

	if (solverRemoteRepository != NULL) {
		const BString solverRemoteRepositoryName = solverRemoteRepository->Config().Name();
		DepotInfoRef depotInfoForSolverRemoteRepository
			= data.DepotByName(solverRemoteRepositoryName);

		if (depotInfoForSolverRemoteRepository.IsSet()) {
			coreInfoBuilder.WithDepotName(solverRemoteRepositoryName);
			HDTRACE("pkg [%s]; assigned to [%s]", packageName.String(), repositoryName.String());
		} else {
			HDDEBUG("pkg [%s]; repository [%s] not recognized --> ignored", packageName.String(),
				repositoryName.String());
		}

		data.AddRemotePackageName(packageName);
	} else {

		if (LocalPkgDataLoadProcessUtils::_IsSystemRepository(manager, solverRepository)) {
			localInfoBuilder.AddInstallationLocation(B_PACKAGE_INSTALLATION_LOCATION_SYSTEM);
			if ((solverPackageFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) == 0)
				data.AddSystemInstalledPackage(solverPackageInfo);
		} else if (LocalPkgDataLoadProcessUtils::_IsHomeRepository(manager, solverRepository)) {
			localInfoBuilder.AddInstallationLocation(B_PACKAGE_INSTALLATION_LOCATION_HOME);
		}

	}

	if ((solverPackageFlags & BPackageKit::B_PACKAGE_FLAG_SYSTEM_PACKAGE) != 0) {
		HDTRACE("am adding system flagged package [%s]", solverPackageInfo.FileName().String());
		data.AddSystemFlaggedPackageFilename(solverPackageInfo.FileName());
	}

	PackageLocalInfoRef localInfo = localInfoBuilder.BuildRef();
	PackageLocalizedTextRef localizedText = localizedTextBuilder.BuildRef();
	PackageCoreInfoRef coreInfo = coreInfoBuilder.BuildRef();
	PackageScreenshotInfoRef screenshotInfo = screenshotInfoBuilder.BuildRef();

	PackageInfoRef package = PackageInfoBuilder(packageName)
								 .WithLocalInfo(localInfo)
								 .WithLocalizedText(localizedText)
								 .WithCoreInfo(coreInfo)
								 .WithPackageClassificationInfo(classificationInfo)
								 .WithUserRatingInfo(userRatingInfo)
								 .WithScreenshotInfo(screenshotInfo)
								 .BuildRef();

	data.AddPackage(package);

	return result;
}


/*!	This method will filter out any packages that are not from a repository. It will
	then be able to identify those that are "local" only. A fake Depot is
	created for these so that they can be seen.
*/
/*static*/ status_t
LocalPkgDataLoadProcessUtils::_CollateLocalDepotAndPackageData(
	LocalPkgDataLoadProcessUtilsData& data)
{
	uint32 count = 0;
	std::map<BString, PackageInfoRef>::const_iterator it;

	for (it = data.fFoundPackages.begin(); it != data.fFoundPackages.end(); it++) {
		BString packageName = it->first;
		PackageInfoRef packageInfo = it->second;

		if (!data.IsRemotePackageName(packageName)) {
			BString repoName = B_TRANSLATE("Local");

			if (count == 0) {
				data.AddDepot(DepotInfoBuilder()
						.WithName(repoName)
						.WithIdentifier(LOCAL_DEPOT_IDENTIFIER)
						.BuildRef());
			}

			// This one is local and so should be replaced with the special identifier.

			PackageCoreInfoRef coreInfo = PackageCoreInfoBuilder(packageInfo->CoreInfo())
											  .WithDepotName(repoName)
											  .BuildRef();

			PackageInfoRef replacementPackageInfo
				= PackageInfoBuilder(packageInfo).WithCoreInfo(coreInfo).BuildRef();

			data.AddPackage(replacementPackageInfo);

			count++;
		}
	}

	return B_OK;
}


/*!	This method will identify those packages that are required for the system
	to function and mark those on the in-memory models of the packages.
*/

/*static*/ status_t
LocalPkgDataLoadProcessUtils::_EnhanceSystemPackages(LocalPkgDataLoadProcessUtilsData& data)
{
	try {
		// create the solver
		BSolver* solver;
		status_t error = BSolver::Create(solver);
		if (error != B_OK)
			throw BFatalErrorException(error, "Failed to create solver.");

		ObjectDeleter<BSolver> solverDeleter(solver);
		BPath systemPath;
		error = find_directory(B_SYSTEM_PACKAGES_DIRECTORY, &systemPath);
		if (error != B_OK)
			throw BFatalErrorException(error, "Unable to retrieve system packages directory.");

		HDDEBUG("will assemble the system packages to enhance");

		// add the "installed" repository with the given packages

		BSolverRepository installedRepository;
		{
			BRepositoryBuilder installedRepositoryBuilder(installedRepository,
				REPOSITORY_NAME_INSTALLED);

			std::set<BString>::iterator it;

			for (it = data.fSystemFlaggedPackageFilenames.begin();
				it != data.fSystemFlaggedPackageFilenames.end(); it++) {
				BPath packagePath(systemPath);
				packagePath.Append(*(it));

				bool exists = false;

				if (StorageUtils::ExistsObject(packagePath, &exists, NULL, NULL) != B_OK)
					throw BFatalErrorException(error, "unable to check package file's existence.");

				const char* packagePathStr = packagePath.Path();

				if (exists) {
					HDTRACE("adding system flagged package filename [%s]", packagePathStr);
					installedRepositoryBuilder.AddPackage(packagePathStr);
				} else {
					HDINFO("system flagged package filename [%s] is not existing --> ignored",
						packagePathStr);
				}
			}

			installedRepositoryBuilder.AddToSolver(solver, true);
		}

		HDDEBUG("creating a system repository");

		// add system repository
		BSolverRepository systemRepository;

		{
			BRepositoryBuilder systemRepositoryBuilder(systemRepository, REPOSITORY_NAME_SYSTEM);
			std::map<BString, BString>::iterator it;

			for (it = data.fSystemInstalledFilenamesToPackageNames.begin();
				it != data.fSystemInstalledFilenamesToPackageNames.end(); it++) {
				BPath packagePath(systemPath);
				packagePath.Append(it->first);
				systemRepositoryBuilder.AddPackage(packagePath.Path());
			}

			systemRepositoryBuilder.AddToSolver(solver, false);
		}

		HDDEBUG("verifying system repository");

		// solve
		error = solver->VerifyInstallation();

		if (error != B_OK)
			throw BFatalErrorException(error, "Failed to compute packages to install.");

		HDDEBUG("getting system repository result");

		BSolverResult solverResult;
		error = solver->GetResult(solverResult);

		if (error != B_OK)
			throw BFatalErrorException(error, "Failed to retrieve system package dependency list.");

		HDDEBUG("processing system repository files");

		for (int32 i = 0; const BSolverResultElement* element = solverResult.ElementAt(i); i++) {
			BSolverPackage* package = element->Package();

			if (element->Type() == BSolverResultElement::B_TYPE_INSTALL) {
				BString packageFilename = package->Info().FileName();

				HDTRACE("will process package filename [%s]", packageFilename.String());

				PackageInfoRef systemInstalledPackage
					= data.PackageBySystemInstalledFilename(packageFilename);

				if (systemInstalledPackage.IsSet()) {
					PackageLocalInfoRef localInfo
						= PackageLocalInfoBuilder(systemInstalledPackage->LocalInfo())
							  .WithSystemDependency(true)
							  .BuildRef();

					PackageInfoRef systemInstalledPackageUpdated
						= PackageInfoBuilder(systemInstalledPackage)
							  .WithLocalInfo(localInfo)
							  .BuildRef();

					data.AddPackage(systemInstalledPackageUpdated);
				}
			}
		}
	} catch (BFatalErrorException& ex) {
		BString logMessage = _LogMessageFromFatalError(ex);
		HDERROR(logMessage.String());
		return B_ERROR;
	} catch (BNothingToDoException&) {
		// do nothing
	} catch (BException& ex) {
		HDERROR("Exception occurred while resolving system dependencies: %s",
			ex.Message().String());
		return B_ERROR;
	} catch (...) {
		HDERROR("Unknown exception occurred while resolving system dependencies.");
		return B_ERROR;
	}

	return B_OK;
}


/*static*/ BString
LocalPkgDataLoadProcessUtils::_LogMessageFromFatalError(BFatalErrorException& ex)
{
	BString logMessage;

	logMessage.SetToFormat("a fatal exception has occurred resolving system dependencies: [%s]",
		strerror(ex.Error()));

	if (!ex.Details().IsEmpty()) {
		logMessage.Append(", details: [");
		logMessage.Append(ex.Details());
		logMessage.Append("]");
	}

	if (ex.HasCommitTransactionFailed()) {
		logMessage.Append("txn result: [");
		logMessage.Append(ex.CommitTransactionResult().FullErrorMessage());
		logMessage.Append("]");
	}

	return logMessage;
}


/*!	This method will write all of the assembled packages into the model. It will
	also load in the Depots as well.
*/
/*static*/ status_t
LocalPkgDataLoadProcessUtils::_PopulateModel(LocalPkgDataLoadProcessUtilsData& data, Model* model,
	bool force)
{

	// Transfer the packages to a vector data structure to supply to the model.

	std::vector<PackageInfoRef> packages;
	std::map<BString, PackageInfoRef>::iterator packageIt;

	for (packageIt = data.fFoundPackages.begin(); packageIt != data.fFoundPackages.end();
		packageIt++) {
		packages.push_back(packageIt->second);
	}

	if (force)
		model->Clear();

	// first populate the depots into the model.

	std::map<BString, DepotInfoRef>::iterator depotIt;
	std::vector<DepotInfoRef> depots;

	for (depotIt = data.fDepots.begin(); depotIt != data.fDepots.end(); depotIt++) {
		const DepotInfoRef depotInfo = depotIt->second;
		if (!depotInfo->Identifier().IsEmpty())
			depots.push_back(depotInfo);
	}

	model->SetDepots(depots);

	uint32 changeMask = PKG_CHANGED_LOCAL_INFO | PKG_CHANGED_CORE_INFO | PKG_CHANGED_LOCALIZED_TEXT;
	model->AddPackagesWithChange(packages, changeMask);

	return B_OK;
}


/*static*/ void
LocalPkgDataLoadProcessUtils::_NotifyError(const BString& messageText)
{
	HDERROR("an error has arisen loading data of packages from local : %s", messageText.String());
	AppUtils::NotifySimpleError(B_TRANSLATE("Local repository load error"), messageText);
}


/*static*/ bool
LocalPkgDataLoadProcessUtils::_IsHomeRepository(PackageManager& manager,
	BSolverRepository* solverRepository)
{
	return solverRepository == static_cast<const BSolverRepository*>(manager.HomeRepository());
}


/*static*/ bool
LocalPkgDataLoadProcessUtils::_IsSystemRepository(PackageManager& manager,
	BSolverRepository* solverRepository)
{
	return solverRepository == static_cast<const BSolverRepository*>(manager.SystemRepository());
}


int32
LocalPkgDataLoadProcessUtilsData::CountPackages() const
{
	return static_cast<int32>(fFoundPackages.size());
}


void
LocalPkgDataLoadProcessUtilsData::AddPackage(PackageInfoRef packageInfo)
{
	if (!packageInfo.IsSet())
		HDFATAL("attempt to work with unset package info");
	const BString packageName = packageInfo->Name();
	HDTRACE("did add package [%s]", packageName.String());
	fFoundPackages[packageName] = packageInfo;
}


const PackageInfoRef
LocalPkgDataLoadProcessUtilsData::PackageByName(const BString& name) const
{
	if (!name.IsEmpty()) {
		std::map<BString, PackageInfoRef>::const_iterator it = fFoundPackages.find(name);

		if (it != fFoundPackages.end())
			return it->second;
	}

	return PackageInfoRef();
}


void
LocalPkgDataLoadProcessUtilsData::AddDepot(DepotInfoRef depotInfo)
{
	if (!depotInfo.IsSet())
		HDFATAL("attempt to work with unset depot info");
	fDepots[depotInfo->Name()] = depotInfo;
}


int32
LocalPkgDataLoadProcessUtilsData::CountSolverPackages() const
{
	return static_cast<int32>(fSolverPackages.size());
}


void
LocalPkgDataLoadProcessUtilsData::AddSolverPackage(const BSolverPackage* solverPackage)
{
	fSolverPackages.push_back(solverPackage);
}


void
LocalPkgDataLoadProcessUtilsData::AddRemotePackageName(const BString& name)
{
	fRemotePackageNames.insert(name);
}


bool
LocalPkgDataLoadProcessUtilsData::IsRemotePackageName(const BString& name) const
{
	return fRemotePackageNames.find(name) != fRemotePackageNames.end();
}


void
LocalPkgDataLoadProcessUtilsData::AddSystemFlaggedPackageFilename(const BString& filename)
{
	fSystemFlaggedPackageFilenames.insert(filename);
}


void
LocalPkgDataLoadProcessUtilsData::AddSystemInstalledPackage(const BPackageInfo& solverPackageInfo)
{
	const BString packageName = solverPackageInfo.Name();
	const BString packageFilename = solverPackageInfo.FileName();
	fSystemInstalledFilenamesToPackageNames[packageFilename] = packageName;
}


const PackageInfoRef
LocalPkgDataLoadProcessUtilsData::PackageBySystemInstalledFilename(const BString& packageFilename)
{
	BString packageName;

	if (packageFilename.IsEmpty()) {
		std::map<BString, BString>::const_iterator it;

		BString packageName;
		it = fSystemInstalledFilenamesToPackageNames.find(packageFilename);

		if (it != fSystemInstalledFilenamesToPackageNames.end())
			packageName = it->second;
	}

	return PackageByName(packageName);
}


const DepotInfoRef
LocalPkgDataLoadProcessUtilsData::DepotByName(const BString& name) const
{
	if (!name.IsEmpty()) {
		std::map<BString, DepotInfoRef>::const_iterator it = fDepots.find(name);

		if (it != fDepots.end())
			return it->second;
	}

	return DepotInfoRef();
}


int32
LocalPkgDataLoadProcessUtilsData::CountDepots() const
{
	return static_cast<int32>(fDepots.size());
}
