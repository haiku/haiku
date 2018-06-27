/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "RepositoryDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <FileIO.h>
#include <Url.h>

#include "ServerSettings.h"
#include "StorageUtils.h"
#include "Logger.h"
#include "DumpExportRepository.h"
#include "DumpExportRepositorySource.h"
#include "DumpExportRepositoryJsonListener.h"


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
		printf("associated dept [%s] (%s) with server repository "
			"source [%s] (%s)\n", modifiedDepotInfo.Name().String(),
			modifiedDepotInfo.BaseURL().String(),
			repositorySourceCode->String(),
			repositoryAndRepositorySource->repositorySource->Url()->String());
	} else {
		printf("associated depot [%s] with server repository source [%s]\n",
			modifiedDepotInfo.Name().String(), repositorySourceCode->String());
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

		BString* baseURL = repositoryAndRepositorySource
			.repositorySource->Url();
		BString* URL = repositoryAndRepositorySource
			.repositorySource->RepoInfoUrl();


		if (!baseURL->IsEmpty() || !URL->IsEmpty()) {
			fModel->ReplaceDepotByUrl(*URL, *baseURL, this,
				&repositoryAndRepositorySource);
		}
	}

	return !fStoppable->WasStopped();
}


void
DepotMatchingRepositoryListener::Complete()
{
}


RepositoryDataUpdateProcess::RepositoryDataUpdateProcess(
	AbstractServerProcessListener* listener,
	const BPath& localFilePath,
	Model* model,
	uint32 options)
	:
	AbstractSingleFileServerProcess(listener, options),
	fLocalFilePath(localFilePath),
	fModel(model)
{
}


RepositoryDataUpdateProcess::~RepositoryDataUpdateProcess()
{
}


const char*
RepositoryDataUpdateProcess::Name()
{
	return "RepositoryDataUpdateProcess";
}


BString
RepositoryDataUpdateProcess::UrlPathComponent()
{
	return BString("/__repository/all-en.json.gz");
}


BPath&
RepositoryDataUpdateProcess::LocalPath()
{
	return fLocalFilePath;
}


status_t
RepositoryDataUpdateProcess::ProcessLocalData()
{
	DepotMatchingRepositoryListener* itemListener =
		new DepotMatchingRepositoryListener(fModel, this);

	BulkContainerDumpExportRepositoryJsonListener* listener =
		new BulkContainerDumpExportRepositoryJsonListener(itemListener);

	status_t result = ParseJsonFromFileWithListener(listener, fLocalFilePath);

	if (B_OK != result)
		return result;

	return listener->ErrorStatus();
}


void
RepositoryDataUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	path.SetTo(fLocalFilePath.Path());
}


void
RepositoryDataUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}
