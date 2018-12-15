/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerRepositoryDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <AutoDeleter.h>
#include <Catalog.h>
#include <FileIO.h>
#include <Url.h>

#include "ServerSettings.h"
#include "StorageUtils.h"
#include "Logger.h"
#include "DumpExportRepository.h"
#include "DumpExportRepositorySource.h"
#include "DumpExportRepositoryJsonListener.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerRepositoryDataUpdateProcess"


/*! This repository listener (not at the JSON level) is feeding in the
    repositories as they are parsed and processing them.  Processing
    includes finding the matching depot record and coupling the data
    from the server with the data about the depot.
*/

class DepotMatchingRepositoryListener :
	public DumpExportRepositoryListener, public DepotMapper {
public:
								DepotMatchingRepositoryListener(Model* model,
									Stoppable* stoppable);
	virtual						~DepotMatchingRepositoryListener();

	virtual DepotInfo			MapDepot(const DepotInfo& depot, void *context);
	virtual	bool				Handle(DumpExportRepository* item);
	virtual	void				Complete();

private:
			void				NormalizeUrl(BUrl& url) const;
			bool				IsUnassociatedDepotByUrl(
									const DepotInfo& depotInfo,
									const BString& urlStr) const;

			int32				IndexOfUnassociatedDepotByUrl(
									const BString& url) const;

			Model*				fModel;
			Stoppable*			fStoppable;
};


DepotMatchingRepositoryListener::DepotMatchingRepositoryListener(
	Model* model, Stoppable* stoppable)
	:
	fModel(model),
	fStoppable(stoppable)
{
}


DepotMatchingRepositoryListener::~DepotMatchingRepositoryListener()
{
}


struct repository_and_repository_source {
	DumpExportRepository* repository;
	DumpExportRepositorySource* repositorySource;
};


/*! This is invoked as a result of logic in 'Handle(..)' that requests that the
    model call this method with the requested DepotInfo instance.
*/

DepotInfo
DepotMatchingRepositoryListener::MapDepot(const DepotInfo& depot, void *context)
{
	repository_and_repository_source* repositoryAndRepositorySource =
		(repository_and_repository_source*) context;
	BString* repositoryCode =
		repositoryAndRepositorySource->repository->Code();
	BString* repositorySourceCode =
		repositoryAndRepositorySource->repositorySource->Code();

	DepotInfo modifiedDepotInfo(depot);
	modifiedDepotInfo.SetWebAppRepositoryCode(BString(*repositoryCode));
	modifiedDepotInfo.SetWebAppRepositorySourceCode(
		BString(*repositorySourceCode));

	if (Logger::IsDebugEnabled()) {
		printf("[DepotMatchingRepositoryListener] associated depot [%s] (%s) "
			"with server repository source [%s] (%s)\n",
			modifiedDepotInfo.Name().String(),
			modifiedDepotInfo.URL().String(),
			repositorySourceCode->String(),
			repositoryAndRepositorySource->repositorySource->Url()->String());
	} else {
		printf("[DepotMatchingRepositoryListener] associated depot [%s] with "
			"server repository source [%s]\n",
			modifiedDepotInfo.Name().String(),
			repositorySourceCode->String());
	}

	return modifiedDepotInfo;
}


bool
DepotMatchingRepositoryListener::Handle(DumpExportRepository* repository)
{
	int32 i;

	for (i = 0; i < repository->CountRepositorySources(); i++) {
		repository_and_repository_source repositoryAndRepositorySource;
		repositoryAndRepositorySource.repository = repository;
		repositoryAndRepositorySource.repositorySource =
			repository->RepositorySourcesItemAt(i);

		BString* repoInfoURL = repositoryAndRepositorySource
			.repositorySource->RepoInfoUrl();

		if (!repoInfoURL->IsEmpty()) {
			fModel->ReplaceDepotByUrl(*repoInfoURL, this,
				&repositoryAndRepositorySource);
		}
	}

	return !fStoppable->WasStopped();
}


void
DepotMatchingRepositoryListener::Complete()
{
}


ServerRepositoryDataUpdateProcess::ServerRepositoryDataUpdateProcess(
	Model* model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
	fModel(model)
{
}


ServerRepositoryDataUpdateProcess::~ServerRepositoryDataUpdateProcess()
{
}


const char*
ServerRepositoryDataUpdateProcess::Name() const
{
	return "ServerRepositoryDataUpdateProcess";
}


const char*
ServerRepositoryDataUpdateProcess::Description() const
{
	return B_TRANSLATE("Synchronizing meta-data about repositories");
}


BString
ServerRepositoryDataUpdateProcess::UrlPathComponent()
{
	return BString("/__repository/all-en.json.gz");
}


status_t
ServerRepositoryDataUpdateProcess::GetLocalPath(BPath& path) const
{
	return fModel->DumpExportRepositoryDataPath(path);
}


status_t
ServerRepositoryDataUpdateProcess::ProcessLocalData()
{
	DepotMatchingRepositoryListener* itemListener =
		new DepotMatchingRepositoryListener(fModel, this);
	ObjectDeleter<DepotMatchingRepositoryListener>
		itemListenerDeleter(itemListener);

	BulkContainerDumpExportRepositoryJsonListener* listener =
		new BulkContainerDumpExportRepositoryJsonListener(itemListener);
	ObjectDeleter<BulkContainerDumpExportRepositoryJsonListener>
		listenerDeleter(listener);

	BPath localPath;
	status_t result = GetLocalPath(localPath);

	if (result != B_OK)
		return result;

	result = ParseJsonFromFileWithListener(listener, localPath);

	if (result != B_OK)
		return result;

	return listener->ErrorStatus();
}


status_t
ServerRepositoryDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	return GetLocalPath(path);
}


void
ServerRepositoryDataUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}
