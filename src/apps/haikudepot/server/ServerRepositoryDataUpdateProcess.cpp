/*
 * Copyright 2017-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ServerRepositoryDataUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <AutoDeleter.h>
#include <Autolock.h>
#include <Catalog.h>
#include <FileIO.h>
#include <Url.h>

#include "DumpExportRepository.h"
#include "DumpExportRepositoryJsonListener.h"
#include "DumpExportRepositorySource.h"
#include "Logger.h"
#include "PackageInfo.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerRepositoryDataUpdateProcess"


/*! This repository listener (not at the JSON level) is feeding in the
	repositories as they are parsed and processing them.  Processing
	includes finding the matching depot record and coupling the data
	from the server with the data about the depot.
*/

class DepotMatchingRepositoryListener : public DumpExportRepositoryListener {
public:
								DepotMatchingRepositoryListener(Model* model,
									Stoppable* stoppable);
	virtual						~DepotMatchingRepositoryListener();

	virtual	bool				Handle(DumpExportRepository* item);
			void				Handle(DumpExportRepository* repository,
									DumpExportRepositorySource*
										repositorySource);
			void				Handle(const BString& identifier,
									DumpExportRepository* repository,
									DumpExportRepositorySource*
										repositorySource);
	virtual	void				Complete();

private:
			void				_SetupRepositoryData(
									DepotInfoRef& depot,
									DumpExportRepository* repository,
									DumpExportRepositorySource*
										repositorySource);

private:
			Model*				fModel;
			Stoppable*			fStoppable;
			std::vector<DepotInfoRef>
								fDepots;
};


DepotMatchingRepositoryListener::DepotMatchingRepositoryListener(Model* model, Stoppable* stoppable)
	:
	fModel(model),
	fStoppable(stoppable),
	fDepots()
{
}


DepotMatchingRepositoryListener::~DepotMatchingRepositoryListener()
{
}


void
DepotMatchingRepositoryListener::_SetupRepositoryData(DepotInfoRef& depot,
	DumpExportRepository* repository, DumpExportRepositorySource* repositorySource)
{
	BString* repositoryCode = repository->Code();
	BString* repositorySourceCode = repositorySource->Code();

	DepotInfoBuilder depotBuilder(depot);

	depotBuilder.WithWebAppRepositoryCode(*repositoryCode);
	depotBuilder.WithWebAppRepositorySourceCode(*repositorySourceCode);

	DepotInfoRef depotInfo = depotBuilder.BuildRef();
	fDepots.push_back(depotInfo);

	if (Logger::IsDebugEnabled()) {
		HDDEBUG("[DepotMatchingRepositoryListener] associated depot [%s] (%s) "
				"with server repository source [%s] (%s)",
			depot->Name().String(), depot->Identifier().String(), repositorySourceCode->String(),
			repositorySource->Identifier()->String());
	} else {
		HDINFO("[DepotMatchingRepositoryListener] associated depot [%s] with "
			   "server repository source [%s]",
			depot->Name().String(), repositorySourceCode->String());
	}
}


void
DepotMatchingRepositoryListener::Handle(const BString& identifier, DumpExportRepository* repository,
	DumpExportRepositorySource* repositorySource)
{
	if (!identifier.IsEmpty()) {
		DepotInfoRef depotForIdentifier = fModel->DepotForIdentifier(identifier);

		if (depotForIdentifier.IsSet())
			_SetupRepositoryData(depotForIdentifier, repository, repositorySource);
	}
}


void
DepotMatchingRepositoryListener::Handle(DumpExportRepository* repository,
	DumpExportRepositorySource* repositorySource)
{
	if (!repositorySource->IdentifierIsNull())
		Handle(*(repositorySource->Identifier()), repository, repositorySource);

	// there may be additional identifiers for the remote repository and
	// these should also be taken into consideration.

	for (int32 i = 0; i < repositorySource->CountExtraIdentifiers(); i++)
		Handle(*(repositorySource->ExtraIdentifiersItemAt(i)), repository, repositorySource);
}


bool
DepotMatchingRepositoryListener::Handle(DumpExportRepository* repository)
{
	int32 i;
	for (i = 0; i < repository->CountRepositorySources(); i++)
		Handle(repository, repository->RepositorySourcesItemAt(i));
	return !fStoppable->WasStopped();
}


void
DepotMatchingRepositoryListener::Complete()
{
	fModel->SetDepots(fDepots);
}


ServerRepositoryDataUpdateProcess::ServerRepositoryDataUpdateProcess(Model* model,
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
	BString result;
	result.SetToFormat("/__repository/all-%s.json.gz", fModel->PreferredLanguage()->ID());
	return result;
}


status_t
ServerRepositoryDataUpdateProcess::GetLocalPath(BPath& path) const
{
	return StorageUtils::DumpExportRepositoryDataPath(path, fModel->PreferredLanguage());
}


status_t
ServerRepositoryDataUpdateProcess::ProcessLocalData()
{
	DepotMatchingRepositoryListener* itemListener
		= new DepotMatchingRepositoryListener(fModel, this);
	ObjectDeleter<DepotMatchingRepositoryListener> itemListenerDeleter(itemListener);

	BulkContainerDumpExportRepositoryJsonListener* listener
		= new BulkContainerDumpExportRepositoryJsonListener(itemListener);
	ObjectDeleter<BulkContainerDumpExportRepositoryJsonListener> listenerDeleter(listener);

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
ServerRepositoryDataUpdateProcess::GetStandardMetaDataJsonPath(BString& jsonPath) const
{
	jsonPath.SetTo("$.info");
}
