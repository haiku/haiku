/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "BulkLoadContext.h"


BulkLoadContext::BulkLoadContext()
	:
	fState(BULK_LOAD_INITIAL),
	fIconProcess(NULL),
	fRepositoryProcess(NULL),
	fPkgProcesses(new List<AbstractServerProcess*, true>()),
	fProcessOptions(0)
{
}


BulkLoadContext::~BulkLoadContext()
{
	StopAllProcesses();

	if (fIconProcess != NULL)
		delete fIconProcess;

	if (fRepositoryProcess != NULL)
		delete fRepositoryProcess;

	int32 count = fPkgProcesses->CountItems();
	int32 i;

	for (i = 0; i < count; i++)
		delete fPkgProcesses->ItemAt(i);

	delete fPkgProcesses;
}


bulk_load_state
BulkLoadContext::State()
{
	return fState;
}


void
BulkLoadContext::SetState(bulk_load_state value)
{
	fState = value;
}


void
BulkLoadContext::StopAllProcesses()
{
	if (fIconProcess != NULL)
		fIconProcess->Stop();

	if (fRepositoryProcess != NULL)
		fRepositoryProcess->Stop();

	int32 count = fPkgProcesses->CountItems();
	int32 i;

	for (i = 0; i < count; i++)
		fPkgProcesses->ItemAt(i)->Stop();
}


AbstractServerProcess*
BulkLoadContext::IconProcess()
{
	return fIconProcess;
}


void
BulkLoadContext::SetIconProcess(AbstractServerProcess* value)
{
	fIconProcess = value;
}


AbstractServerProcess*
BulkLoadContext::RepositoryProcess()
{
	return fRepositoryProcess;
}


void
BulkLoadContext::SetRepositoryProcess(
	AbstractServerProcess* value)
{
	fRepositoryProcess = value;
}


int32
BulkLoadContext::CountPkgProcesses()
{
	return fPkgProcesses->CountItems();
}


AbstractServerProcess*
BulkLoadContext::PkgProcessAt(int32 index)
{
	return fPkgProcesses->ItemAt(index);
}


void
BulkLoadContext::AddPkgProcess(AbstractServerProcess *value)
{
	fPkgProcesses->Add(value);
}


void
BulkLoadContext::AddProcessOption(uint32 flag)
{
	fProcessOptions = fProcessOptions | flag;
}


uint32
BulkLoadContext::ProcessOptions()
{
	return fProcessOptions;
}