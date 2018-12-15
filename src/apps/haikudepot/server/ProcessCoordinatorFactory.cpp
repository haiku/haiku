/*
 * Copyright 2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ProcessCoordinatorFactory.h"

#include <Autolock.h>

#include <package/Context.h>
#include <package/PackageRoster.h>

#include "AbstractServerProcess.h"
#include "LocalPkgDataLoadProcess.h"
#include "LocalRepositoryUpdateProcess.h"
#include "Model.h"
#include "PackageInfoListener.h"
#include "ProcessCoordinator.h"
#include "ProcessNode.h"
#include "ServerHelper.h"
#include "ServerIconExportUpdateProcess.h"
#include "ServerPkgDataUpdateProcess.h"
#include "ServerRepositoryDataUpdateProcess.h"
#include "ServerSettings.h"


using namespace BPackageKit;


/* static */ ProcessCoordinator*
ProcessCoordinatorFactory::CreateBulkLoadCoordinator(
	PackageInfoListener *packageInfoListener,
	ProcessCoordinatorListener* processCoordinatorListener,
	Model* model, bool forceLocalUpdate)
{
	uint32 serverProcessOptions = _CalculateServerProcessOptions();
	BAutolock locker(model->Lock());
	ProcessCoordinator* processCoordinator = new ProcessCoordinator(
		processCoordinatorListener);

	ProcessNode *localRepositoryUpdate =
		new ProcessNode(new LocalRepositoryUpdateProcess(model,
			forceLocalUpdate));
	processCoordinator->AddNode(localRepositoryUpdate);

	ProcessNode *localPkgDataLoad =
		new ProcessNode(new LocalPkgDataLoadProcess(
			packageInfoListener, model, forceLocalUpdate));
	localPkgDataLoad->AddPredecessor(localRepositoryUpdate);
	processCoordinator->AddNode(localPkgDataLoad);

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

	// create a process for each of the repositories that are configured on the
	// local system.  Later, only those that have a web-app repository server
	// code will be actually processed, but this means that the creation of the
	// 'processes' does not need to be dynamic as the process coordinator runs.

	BPackageRoster roster;
	BStringList repoNames;
	status_t repoNamesResult = roster.GetRepositoryNames(repoNames);

	if (repoNamesResult == B_OK) {
		for (int32 i = 0; i < repoNames.CountStrings(); i++) {
			ProcessNode* processNode = new ProcessNode(
				new ServerPkgDataUpdateProcess(model->PreferredLanguage(),
					repoNames.StringAt(i), model, serverProcessOptions));
			processNode->AddPredecessor(serverRepositoryDataUpdate);
			processCoordinator->AddNode(processNode);
		}
	} else {
		printf("a problem has arisen getting the repository names.\n");
	}

	return processCoordinator;
}


/* static */ uint32
ProcessCoordinatorFactory::_CalculateServerProcessOptions()
{
	uint32 processOptions = 0;

	if (ServerSettings::IsClientTooOld()) {
		printf("bulk load proceeding without network communications "
			"because the client is too old\n");
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
