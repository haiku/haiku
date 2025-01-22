/*
 * Copyright 2018-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ProcessCoordinatorFactory.h"

#include <AutoLocker.h>
#include <Autolock.h>

#include <package/Context.h>
#include <package/PackageRoster.h>

#include "AbstractServerProcess.h"
#include "CacheScreenshotProcess.h"
#include "DeskbarLink.h"
#include "HaikuDepotConstants.h"
#include "IncrementViewCounterProcess.h"
#include "InstallPackageProcess.h"
#include "LocalPkgDataLoadProcess.h"
#include "LocalRepositoryUpdateProcess.h"
#include "Logger.h"
#include "Model.h"
#include "OpenPackageProcess.h"
#include "PackageInfoListener.h"
#include "PopulatePkgChangelogFromServerProcess.h"
#include "PopulatePkgSizesProcess.h"
#include "PopulatePkgUserRatingsFromServerProcess.h"
#include "ProcessCoordinator.h"
#include "ServerHelper.h"
#include "ServerIconExportUpdateProcess.h"
#include "ServerPkgDataUpdateProcess.h"
#include "ServerReferenceDataUpdateProcess.h"
#include "ServerRepositoryDataUpdateProcess.h"
#include "ServerSettings.h"
#include "StorageUtils.h"
#include "ThreadedProcessNode.h"
#include "UninstallPackageProcess.h"
#include "UserDetailVerifierProcess.h"


using namespace BPackageKit;


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::CreateIncrementViewCounter(Model* model, const PackageInfoRef package)
{
	ProcessCoordinator* processCoordinator = new ProcessCoordinator("IncrementViewCounter");
	AbstractProcessNode* node
		= new ThreadedProcessNode(new IncrementViewCounterProcess(model, package));
	processCoordinator->AddNode(node);
	return processCoordinator;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::CreateUserDetailVerifierCoordinator(
	UserDetailVerifierListener* userDetailVerifierListener, Model* model)
{
	ProcessCoordinator* processCoordinator = new ProcessCoordinator("UserDetailVerifier");
	AbstractProcessNode* userDetailVerifier
		= new ThreadedProcessNode(new UserDetailVerifierProcess(model, userDetailVerifierListener));
	processCoordinator->AddNode(userDetailVerifier);
	return processCoordinator;
}


/* static */ ProcessCoordinator*
ProcessCoordinatorFactory::CreateBulkLoadCoordinator(Model* model, bool forceLocalUpdate)
{
	bool areWorkingFilesAvailable = StorageUtils::AreWorkingFilesAvailable();
	uint32 serverProcessOptions = _CalculateServerProcessOptions();
	ProcessCoordinator* processCoordinator
		= new ProcessCoordinator("BulkLoad", new BMessage(MSG_BULK_LOAD_DONE));

	AbstractProcessNode* localRepositoryUpdate
		= new ThreadedProcessNode(new LocalRepositoryUpdateProcess(model, forceLocalUpdate));
	processCoordinator->AddNode(localRepositoryUpdate);

	AbstractProcessNode* localPkgDataLoad
		= new ThreadedProcessNode(new LocalPkgDataLoadProcess(model, forceLocalUpdate));
	localPkgDataLoad->AddPredecessor(localRepositoryUpdate);
	processCoordinator->AddNode(localPkgDataLoad);

	if (areWorkingFilesAvailable) {
		AbstractProcessNode* serverIconExportUpdate = new ThreadedProcessNode(
			new ServerIconExportUpdateProcess(model, serverProcessOptions));
		serverIconExportUpdate->AddPredecessor(localPkgDataLoad);
		processCoordinator->AddNode(serverIconExportUpdate);

		AbstractProcessNode* serverRepositoryDataUpdate = new ThreadedProcessNode(
			new ServerRepositoryDataUpdateProcess(model, serverProcessOptions));
		serverRepositoryDataUpdate->AddPredecessor(localPkgDataLoad);
		processCoordinator->AddNode(serverRepositoryDataUpdate);

		AbstractProcessNode* serverReferenceDataUpdate = new ThreadedProcessNode(
			new ServerReferenceDataUpdateProcess(model, serverProcessOptions));
		processCoordinator->AddNode(serverReferenceDataUpdate);

		// This one has to run after the server data is taken up because it
		// will fill in the gaps based on local data that was not able to be
		// sourced from the server. It has all of the
		// `ServerPkgDataUpdateProcess` nodes configured as its predecessors.

		AbstractProcessNode* populatePkgSizes
			= new ThreadedProcessNode(new PopulatePkgSizesProcess(model));

		// create a process for each of the repositories that are configured on
		// the local system.  Later, only those that have a web-app repository
		// server code will be actually processed, but this means that the
		// creation of the 'processes' does not need to be dynamic as the
		// process coordinator runs.

		BPackageRoster roster;
		BStringList repoNames;
		status_t repoNamesResult = roster.GetRepositoryNames(repoNames);

		if (repoNamesResult == B_OK) {
			for (int32 i = 0; i < repoNames.CountStrings(); i++) {
				AbstractProcessNode* processNode
					= new ThreadedProcessNode(new ServerPkgDataUpdateProcess(repoNames.StringAt(i),
						model, serverProcessOptions));
				processNode->AddPredecessor(serverRepositoryDataUpdate);
				processNode->AddPredecessor(serverReferenceDataUpdate);
				processCoordinator->AddNode(processNode);

				populatePkgSizes->AddPredecessor(processNode);
			}
		} else {
			HDERROR("a problem has arisen getting the repository names.");
		}

		processCoordinator->AddNode(populatePkgSizes);
	}

	return processCoordinator;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::CreatePackageActionCoordinator(Model* model, BMessage* message)
{
	switch (message->what) {
		case MSG_PKG_INSTALL:
			return _CreateInstallPackageActionCoordinator(model, message);
		case MSG_PKG_UNINSTALL:
			return _CreateUninstallPackageActionCoordinator(model, message);
		case MSG_PKG_OPEN:
			return _CreateOpenPackageActionCoordinator(model, message);
		default:
			HDFATAL("unexpected package action message what");
	}
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::CacheScreenshotCoordinator(Model* model,
	ScreenshotCoordinate& screenshotCoordinate)
{
	return _CreateSingleProcessCoordinator("CacheScreenshot",
		new CacheScreenshotProcess(model, screenshotCoordinate));
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::PopulatePkgChangelogCoordinator(Model* model, const BString& packageName)
{
	return _CreateSingleProcessCoordinator("PopulatePkgChangelog",
		new PopulatePkgChangelogFromServerProcess(packageName, model));
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::PopulatePkgUserRatingsCoordinator(Model* model,
	const BString& packageName)
{
	return _CreateSingleProcessCoordinator("PopulatePkgUserRatings",
		new PopulatePkgUserRatingsFromServerProcess(packageName, model));
}


/*static*/ BString
ProcessCoordinatorFactory::_ExtractPackageNameFromMessage(BMessage* message)
{
	BString pkgName;
	if (message->FindString(KEY_PACKAGE_NAME, &pkgName) != B_OK)
		HDFATAL("malformed message missing key [%s]", KEY_PACKAGE_NAME);
	return pkgName;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::_CreateInstallPackageActionCoordinator(Model* model, BMessage* message)
{
	ProcessCoordinator* processCoordinator
		= new ProcessCoordinator("InstallPackage", new BMessage(MSG_PACKAGE_ACTION_DONE));

	AbstractProcessNode* processNode = new ThreadedProcessNode(
		new InstallPackageProcess(_ExtractPackageNameFromMessage(message), model), 10);

	processCoordinator->AddNode(processNode);

	return processCoordinator;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::_CreateUninstallPackageActionCoordinator(Model* model, BMessage* message)
{
	ProcessCoordinator* processCoordinator
		= new ProcessCoordinator("UninstallPackage", new BMessage(MSG_PACKAGE_ACTION_DONE));

	AbstractProcessNode* processNode = new ThreadedProcessNode(
		new UninstallPackageProcess(_ExtractPackageNameFromMessage(message), model), 10);

	processCoordinator->AddNode(processNode);

	return processCoordinator;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::_CreateOpenPackageActionCoordinator(Model* model, BMessage* message)
{
	ProcessCoordinator* processCoordinator
		= new ProcessCoordinator("OpenPackage", new BMessage(MSG_PACKAGE_ACTION_DONE));

	BMessage deskbarLinkMessage;

	if (message->FindMessage(KEY_DESKBAR_LINK, &deskbarLinkMessage) != B_OK)
		HDFATAL("malformed message missing key [%s]", KEY_DESKBAR_LINK);

	DeskbarLink deskbarLink(&deskbarLinkMessage);

	AbstractProcessNode* processNode = new ThreadedProcessNode(
		new OpenPackageProcess(_ExtractPackageNameFromMessage(message), model, deskbarLink));

	processCoordinator->AddNode(processNode);

	return processCoordinator;
}


/* static */ uint32
ProcessCoordinatorFactory::_CalculateServerProcessOptions()
{
	uint32 processOptions = 0;

	if (ServerSettings::IsClientTooOld()) {
		HDINFO("bulk load proceeding without network communications because the client is too old");
		processOptions |= SERVER_PROCESS_NO_NETWORKING;
	}

	if (!ServerHelper::IsNetworkAvailable())
		processOptions |= SERVER_PROCESS_NO_NETWORKING;

	if (ServerSettings::PreferCache())
		processOptions |= SERVER_PROCESS_PREFER_CACHE;

	if (ServerSettings::DropCache())
		processOptions |= SERVER_PROCESS_DROP_CACHE;

	return processOptions;
}


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::_CreateSingleProcessCoordinator(const char* name,
	AbstractProcess* process)
{
	ProcessCoordinator* processCoordinator = new ProcessCoordinator(name);
	AbstractProcessNode* cacheScreenshotNode = new ThreadedProcessNode(process);
	processCoordinator->AddNode(cacheScreenshotNode);
	return processCoordinator;
}
