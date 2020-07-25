/*
 * Copyright 2018-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ProcessCoordinatorFactory.h"

#include <Autolock.h>
#include <AutoLocker.h>

#include <package/Context.h>
#include <package/PackageRoster.h>

#include "AbstractServerProcess.h"
#include "HaikuDepotConstants.h"
#include "LocalPkgDataLoadProcess.h"
#include "LocalRepositoryUpdateProcess.h"
#include "Logger.h"
#include "Model.h"
#include "PackageInfoListener.h"
#include "ProcessCoordinator.h"
#include "ProcessNode.h"
#include "ServerHelper.h"
#include "ServerIconExportUpdateProcess.h"
#include "ServerPkgDataUpdateProcess.h"
#include "ServerReferenceDataUpdateProcess.h"
#include "ServerRepositoryDataUpdateProcess.h"
#include "ServerSettings.h"
#include "StorageUtils.h"
#include "UserDetailVerifierProcess.h"


using namespace BPackageKit;


/*static*/ ProcessCoordinator*
ProcessCoordinatorFactory::CreateUserDetailVerifierCoordinator(
	UserDetailVerifierListener* userDetailVerifierListener,
	ProcessCoordinatorListener* processCoordinatorListener,
	Model* model)
{
	ProcessCoordinator* processCoordinator = new ProcessCoordinator(
		"UserDetailVerifier",
		processCoordinatorListener);
	ProcessNode* userDetailVerifier = new ProcessNode(
		new UserDetailVerifierProcess(model, userDetailVerifierListener));
	processCoordinator->AddNode(userDetailVerifier);
	return processCoordinator;
}


/* static */ ProcessCoordinator*
ProcessCoordinatorFactory::CreateBulkLoadCoordinator(
	PackageInfoListener *packageInfoListener,
	ProcessCoordinatorListener* processCoordinatorListener,
	Model* model, bool forceLocalUpdate)
{
	bool areWorkingFilesAvailable = StorageUtils::AreWorkingFilesAvailable();
	uint32 serverProcessOptions = _CalculateServerProcessOptions();
	BAutolock locker(model->Lock());
	ProcessCoordinator* processCoordinator = new ProcessCoordinator(
		"BulkLoad",
		processCoordinatorListener, new BMessage(MSG_BULK_LOAD_DONE));

	ProcessNode *localRepositoryUpdate =
		new ProcessNode(new LocalRepositoryUpdateProcess(model,
			forceLocalUpdate));
	processCoordinator->AddNode(localRepositoryUpdate);

	ProcessNode *localPkgDataLoad =
		new ProcessNode(new LocalPkgDataLoadProcess(
			packageInfoListener, model, forceLocalUpdate));
	localPkgDataLoad->AddPredecessor(localRepositoryUpdate);
	processCoordinator->AddNode(localPkgDataLoad);

	if (areWorkingFilesAvailable) {
		ProcessNode *serverIconExportUpdate =
			new ProcessNode(new ServerIconExportUpdateProcess(model,
				serverProcessOptions));
		serverIconExportUpdate->AddPredecessor(localPkgDataLoad);
		processCoordinator->AddNode(serverIconExportUpdate);

		ProcessNode *serverRepositoryDataUpdate =
			new ProcessNode(new ServerRepositoryDataUpdateProcess(model,
				serverProcessOptions));
		serverRepositoryDataUpdate->AddPredecessor(localPkgDataLoad);
		processCoordinator->AddNode(serverRepositoryDataUpdate);

		ProcessNode *serverReferenceDataUpdate =
			new ProcessNode(new ServerReferenceDataUpdateProcess(model,
				serverProcessOptions));
		processCoordinator->AddNode(serverReferenceDataUpdate);

		// create a process for each of the repositories that are configured on
		// the local system.  Later, only those that have a web-app repository
		// server code will be actually processed, but this means that the
		// creation of the 'processes' does not need to be dynamic as the
		// process coordinator runs.

		BPackageRoster roster;
		BStringList repoNames;
		status_t repoNamesResult = roster.GetRepositoryNames(repoNames);

		if (repoNamesResult == B_OK) {
			AutoLocker<BLocker> locker(model->Lock());

			for (int32 i = 0; i < repoNames.CountStrings(); i++) {
				ProcessNode* processNode = new ProcessNode(
					new ServerPkgDataUpdateProcess(
						model->Language()->PreferredLanguage()->Code(),
						repoNames.StringAt(i), model, serverProcessOptions));
				processNode->AddPredecessor(serverRepositoryDataUpdate);
				processNode->AddPredecessor(serverReferenceDataUpdate);
				processCoordinator->AddNode(processNode);
			}
		} else
			HDERROR("a problem has arisen getting the repository names.");
	}

	return processCoordinator;
}


/* static */ uint32
ProcessCoordinatorFactory::_CalculateServerProcessOptions()
{
	uint32 processOptions = 0;

	if (ServerSettings::IsClientTooOld()) {
		HDINFO("bulk load proceeding without network communications "
			"because the client is too old");
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
