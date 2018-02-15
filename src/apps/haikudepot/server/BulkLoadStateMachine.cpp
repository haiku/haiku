/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BulkLoadStateMachine.h"

#include <Autolock.h>

#include "Logger.h"
#include "PkgDataUpdateProcess.h"
#include "RepositoryDataUpdateProcess.h"
#include "ServerIconExportUpdateProcess.h"
#include "ServerSettings.h"
#include "ServerHelper.h"


BulkLoadStateMachine::BulkLoadStateMachine(Model* model)
	:
	fBulkLoadContext(NULL),
	fModel(model)
{
}


BulkLoadStateMachine::~BulkLoadStateMachine()
{
	Stop();
}


bool
BulkLoadStateMachine::IsRunning()
{
	BAutolock locker(&fLock);
	return fBulkLoadContext != NULL;
}


/*! This gets invoked when one of the background processes has exited. */

void
BulkLoadStateMachine::ServerProcessExited()
{
	ContextPoll();
}


static const char* bulk_load_state_name(bulk_load_state state) {
	switch(state) {
		case BULK_LOAD_INITIAL:
			return "BULK_LOAD_INITIAL";
		case BULK_LOAD_REPOSITORY_AND_REFERENCE:
			return "BULK_LOAD_REPOSITORY_AND_REFERENCE";
		case BULK_LOAD_PKGS_AND_ICONS:
			return "BULK_LOAD_PKGS_AND_ICONS";
		case BULK_LOAD_COMPLETE:
			return "BULK_LOAD_COMPLETE";
		default:
			return "???";
	}
}


void
BulkLoadStateMachine::SetContextState(bulk_load_state state)
{
	if (Logger::IsDebugEnabled()) {
		printf("bulk load - transition to state [%s]\n",
			bulk_load_state_name(state));
	}

	fBulkLoadContext->SetState(state);
}


/*! Bulk loading data into the model can be considered to be a state
    machine.  This method is invoked each time that an event state
    change happens.
 */

void
BulkLoadStateMachine::ContextPoll()
{
	BAutolock locker(&fLock);

	if (Logger::IsDebugEnabled())
		printf("bulk load - context poll\n");

	if (CanTransitionTo(BULK_LOAD_REPOSITORY_AND_REFERENCE)) {
		SetContextState(BULK_LOAD_REPOSITORY_AND_REFERENCE);
		InitiateBulkPopulateIcons();
		if (InitiateBulkRepositories() != B_OK)
			ContextPoll();
		return;
	}

	if (CanTransitionTo(BULK_LOAD_PKGS_AND_ICONS)) {
		fModel->LogDepotsWithNoWebAppRepositoryCode();
		SetContextState(BULK_LOAD_PKGS_AND_ICONS);
		InitiateBulkPopulatePackagesForAllDepots();
		return;
	}

	if (CanTransitionTo(BULK_LOAD_COMPLETE)) {
		SetContextState(BULK_LOAD_COMPLETE);
		delete fBulkLoadContext;
		fBulkLoadContext = NULL;
		return;
	}
}


bool
BulkLoadStateMachine::CanTransitionTo(bulk_load_state targetState)
{
	if (fBulkLoadContext != NULL) {
		bulk_load_state existingState = fBulkLoadContext->State();

		switch (targetState) {
			case BULK_LOAD_INITIAL:
				return false;
			case BULK_LOAD_REPOSITORY_AND_REFERENCE:
				return existingState == BULK_LOAD_INITIAL;
			case BULK_LOAD_PKGS_AND_ICONS:
				return (existingState == BULK_LOAD_REPOSITORY_AND_REFERENCE)
					&& ((fBulkLoadContext->RepositoryProcess() == NULL)
						|| !fBulkLoadContext->RepositoryProcess()->IsRunning());
			case BULK_LOAD_COMPLETE:
				if ((existingState == BULK_LOAD_PKGS_AND_ICONS)
					&& ((fBulkLoadContext->IconProcess() == NULL)
						|| !fBulkLoadContext->IconProcess()->IsRunning())) {
					int32 i;

					for (i = 0; i < fBulkLoadContext->CountPkgProcesses(); i++) {
						AbstractServerProcess* process =
							fBulkLoadContext->PkgProcessAt(i);
						if (process->IsRunning())
							return false;
					}

					return true;
				}
				break;
		}
	}

	return false;
}


void
BulkLoadStateMachine::StopAllProcesses()
{
	BAutolock locker(&fLock);

	if (fBulkLoadContext != NULL) {
		if (NULL != fBulkLoadContext->IconProcess())
			fBulkLoadContext->IconProcess()->Stop();

		if (NULL != fBulkLoadContext->RepositoryProcess())
			fBulkLoadContext->RepositoryProcess()->Stop();

		int32 i;

		for(i = 0; i < fBulkLoadContext->CountPkgProcesses(); i++) {
			AbstractServerProcess* serverProcess =
				fBulkLoadContext->PkgProcessAt(i);
			serverProcess->Stop();
		}
	}
}


void
BulkLoadStateMachine::Start()
{
	if (Logger::IsInfoEnabled())
		printf("bulk load - start\n");

	Stop();

	{
		BAutolock locker(&fLock);

		if (!IsRunning()) {
			fBulkLoadContext = new BulkLoadContext();

			if (ServerSettings::IsClientTooOld()) {
				printf("bulk load proceeding without network communications "
					"because the client is too old\n");
				fBulkLoadContext->AddProcessOption(
					SERVER_PROCESS_NO_NETWORKING);
			}

			if (!ServerHelper::IsNetworkAvailable()) {
				fBulkLoadContext->AddProcessOption(
					SERVER_PROCESS_NO_NETWORKING);
			}

			if (ServerSettings::PreferCache())
				fBulkLoadContext->AddProcessOption(SERVER_PROCESS_PREFER_CACHE);

			if (ServerSettings::DropCache())
				fBulkLoadContext->AddProcessOption(SERVER_PROCESS_DROP_CACHE);

			ContextPoll();
		}
	}
}


void
BulkLoadStateMachine::Stop()
{
	StopAllProcesses();

	// spin lock to wait for the bulk-load processes to complete.

	while (IsRunning())
		snooze(500000);
}


/*! This method is the initial function that is invoked on starting a new
    thread.  It will start a server process that is part of the bulk-load.
 */

status_t
BulkLoadStateMachine::StartProcess(void* cookie)
{
	AbstractServerProcess* process =
		static_cast<AbstractServerProcess*>(cookie);

	if (Logger::IsInfoEnabled()) {
		printf("bulk load - starting process [%s]\n",
			process->Name());
	}

	process->Run();
	return B_OK;
}


status_t
BulkLoadStateMachine::InitiateServerProcess(AbstractServerProcess* process)
{
	if (Logger::IsInfoEnabled())
		printf("bulk load - initiating [%s]\n", process->Name());

	thread_id tid = spawn_thread(&StartProcess,
		process->Name(), B_NORMAL_PRIORITY, process);

	if (tid >= 0) {
		resume_thread(tid);
		return B_OK;
	}

	return B_ERROR;
}


status_t
BulkLoadStateMachine::InitiateBulkRepositories()
{
	status_t result = B_OK;
	BPath dataPath;

	fBulkLoadContext->SetRepositoryProcess(NULL);
	result = fModel->DumpExportRepositoryDataPath(dataPath);

	if (result != B_OK) {
		BAutolock locker(&fLock);
		printf("unable to obtain the path for storing the repository data\n");
		ContextPoll();
		return B_ERROR;
	}

	fBulkLoadContext->SetRepositoryProcess(
		new RepositoryDataUpdateProcess(this, dataPath, fModel,
			fBulkLoadContext->ProcessOptions()));
	return InitiateServerProcess(fBulkLoadContext->RepositoryProcess());
}


status_t
BulkLoadStateMachine::InitiateBulkPopulateIcons()
{
	BPath path;

	if (fModel->IconStoragePath(path) != B_OK) {
		BAutolock locker(&fLock);
		printf("unable to obtain the path for storing icons\n");
		ContextPoll();
		return B_ERROR;
	}

	AbstractServerProcess *process = new ServerIconExportUpdateProcess(
		this, path, fModel, fBulkLoadContext->ProcessOptions());
	fBulkLoadContext->SetIconProcess(process);
	return InitiateServerProcess(process);
}


status_t
BulkLoadStateMachine::InitiateBulkPopulatePackagesForDepot(
	const DepotInfo& depotInfo)
{
	BString repositorySourceCode = depotInfo.WebAppRepositorySourceCode();

	if (repositorySourceCode.Length() == 0) {
		printf("the depot [%s] has no repository source code\n",
			depotInfo.Name().String());
		return B_ERROR;
	}

	BPath repositorySourcePkgDataPath;

	if (fModel->DumpExportPkgDataPath(repositorySourcePkgDataPath,
		repositorySourceCode) != B_OK) {
		BAutolock locker(&fLock);
		printf("unable to obtain the path for storing data for [%s]\n",
			repositorySourceCode.String());
		ContextPoll();
		return B_ERROR;
	}

	AbstractServerProcess *process = new PkgDataUpdateProcess(
		this, repositorySourcePkgDataPath, fModel->PreferredLanguage(),
		repositorySourceCode, depotInfo.Name(), fModel,
		fBulkLoadContext->ProcessOptions());
	fBulkLoadContext->AddPkgProcess(process);

	return InitiateServerProcess(process);
}


// static
void
BulkLoadStateMachine::InitiatePopulatePackagesForDepotCallback(
	const DepotInfo& depotInfo, void* context)
{
	BulkLoadStateMachine* stateMachine =
		static_cast<BulkLoadStateMachine*>(context);
	stateMachine->InitiateBulkPopulatePackagesForDepot(depotInfo);
}


void
BulkLoadStateMachine::InitiateBulkPopulatePackagesForAllDepots()
{
	fModel->ForAllDepots(&InitiatePopulatePackagesForDepotCallback, this);

	printf("did initiate populate package data for %" B_PRId32 " depots\n",
		fBulkLoadContext->CountPkgProcesses());

	if (0 == fBulkLoadContext->CountPkgProcesses())
		ContextPoll();
}