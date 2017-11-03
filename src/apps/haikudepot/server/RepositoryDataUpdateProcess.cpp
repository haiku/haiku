/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
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
#include "DumpExportRepository.h"
#include "DumpExportRepositorySource.h"
#include "DumpExportRepositoryJsonListener.h"


/*! This repository listener (not at the JSON level) is feeding in the
    repositories as they are parsed and processing them.  Processing
    includes finding the matching depot record and coupling the data
    from the server with the data about the depot.
*/

class DepotMatchingRepositoryListener : public DumpExportRepositoryListener {
public:
								DepotMatchingRepositoryListener(
									DepotList* depotList);
	virtual						~DepotMatchingRepositoryListener();

	virtual	void				Handle(DumpExportRepository* item);
	virtual	void				Complete();

private:
			void				NormalizeUrl(BUrl& url) const;
			bool				IsUnassociatedDepotByUrl(
									const DepotInfo& depotInfo,
									const BString& urlStr) const;

			int32				IndexOfUnassociatedDepotByUrl(
									const BString& url) const;

			DepotList*			fDepotList;

};


DepotMatchingRepositoryListener::DepotMatchingRepositoryListener(
	DepotList* depotList)
{
	fDepotList = depotList;
}


DepotMatchingRepositoryListener::~DepotMatchingRepositoryListener()
{
}


void
DepotMatchingRepositoryListener::NormalizeUrl(BUrl& url) const
{
	if (url.Protocol() == "https")
		url.SetProtocol("http");

	BString path(url.Path());

	if (path.EndsWith("/"))
		url.SetPath(path.Truncate(path.Length() - 1));
}


bool
DepotMatchingRepositoryListener::IsUnassociatedDepotByUrl(
	const DepotInfo& depotInfo, const BString& urlStr) const
{
	if (depotInfo.BaseURL().Length() > 0
		&& depotInfo.WebAppRepositoryCode().Length() == 0) {
		BUrl depotInfoBaseUrl(depotInfo.BaseURL());
		BUrl url(urlStr);

		NormalizeUrl(depotInfoBaseUrl);
		NormalizeUrl(url);

		if (depotInfoBaseUrl == url)
			return true;
	}

	return false;
}


int32
DepotMatchingRepositoryListener::IndexOfUnassociatedDepotByUrl(
	const BString& url) const
{
	int32 i;

	for (i = 0; i < fDepotList->CountItems(); i++) {
		const DepotInfo& depot = fDepotList->ItemAt(i);

		if (IsUnassociatedDepotByUrl(depot, url))
			return i;
	}

	return -1;
}


void
DepotMatchingRepositoryListener::Handle(DumpExportRepository* repository)
{
	int32 i;

	for (i = 0; i < repository->CountRepositorySources(); i++) {
		DumpExportRepositorySource *repositorySource =
			repository->RepositorySourcesItemAt(i);
		int32 depotIndex = IndexOfUnassociatedDepotByUrl(
			*(repositorySource->Url()));

		if (depotIndex >= 0) {
			DepotInfo modifiedDepotInfo(fDepotList->ItemAt(depotIndex));
			modifiedDepotInfo.SetWebAppRepositoryCode(
				BString(*(repository->Code())));
			fDepotList->Replace(depotIndex, modifiedDepotInfo);

			fprintf(stderr, "associated depot [%s] with haiku depot server"
				" repository [%s]\n", modifiedDepotInfo.Name().String(),
				repository->Code()->String());
		}
	}
}


void
DepotMatchingRepositoryListener::Complete()
{
	int32 i;

	for (i = 0; i < fDepotList->CountItems(); i++) {
		const DepotInfo& depot = fDepotList->ItemAt(i);

		if (depot.WebAppRepositoryCode().Length() == 0) {
			fprintf(stderr, "depot [%s]", depot.Name().String());

			if (depot.BaseURL().Length() > 0)
				fprintf(stderr, " (%s)", depot.BaseURL().String());

			fprintf(stderr, " correlates with no repository in the haiku"
				"depot server system\n");
		}
	}
}


RepositoryDataUpdateProcess::RepositoryDataUpdateProcess(
	const BPath& localFilePath,
	DepotList* depotList)
{
	fLocalFilePath = localFilePath;
	fDepotList = depotList;
}


RepositoryDataUpdateProcess::~RepositoryDataUpdateProcess()
{
}


status_t
RepositoryDataUpdateProcess::Run()
{
	fprintf(stdout, "will fetch repositories data\n");

		// TODO: add language ISO code to the path; just 'en' for now.
	status_t result = DownloadToLocalFile(fLocalFilePath,
		ServerSettings::CreateFullUrl("/__repository/all-en.json.gz"),
		0, 0);

	if (result == B_OK) {
		fprintf(stdout, "did fetch repositories data\n");

			// now load the data in and process it.

		fprintf(stderr, "will process repository data and match to depots\n");
		result = PopulateDataToDepots();
		fprintf(stderr, "did process repository data and match to depots\n");
	}

	return result;
}


status_t
RepositoryDataUpdateProcess::PopulateDataToDepots()
{
	DepotMatchingRepositoryListener* itemListener =
		new DepotMatchingRepositoryListener(fDepotList);

	BJsonEventListener* listener =
		new BulkContainerDumpExportRepositoryJsonListener(itemListener);

	return ParseJsonFromFileWithListener(listener, fLocalFilePath);
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


const char*
RepositoryDataUpdateProcess::LoggingName() const
{
	return "repo-data-update";
}