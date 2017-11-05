/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ServerIconExportUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <FileIO.h>
#include <support/ZlibCompressionAlgorithm.h>

#include "ServerSettings.h"
#include "StorageUtils.h"
#include "TarArchiveService.h"


/*! This constructor will locate the cached data in a standardized location */

ServerIconExportUpdateProcess::ServerIconExportUpdateProcess(
	const BPath& localStorageDirectoryPath)
{
	fLocalStorageDirectoryPath = localStorageDirectoryPath;
}


ServerIconExportUpdateProcess::~ServerIconExportUpdateProcess()
{
}


status_t
ServerIconExportUpdateProcess::Run()
{
	BPath tarGzFilePath(tmpnam(NULL));
	status_t result = B_OK;

	printf("will start fetching icons\n");

	result = Download(tarGzFilePath);

	if (result != APP_ERR_NOT_MODIFIED) {
		if (result != B_OK)
			return result;

		printf("delete any existing stored data\n");
		StorageUtils::RemoveDirectoryContents(fLocalStorageDirectoryPath);

		BFile *tarGzFile = new BFile(tarGzFilePath.Path(), O_RDONLY);
		BDataIO* tarIn;

		BZlibDecompressionParameters* zlibDecompressionParameters
			= new BZlibDecompressionParameters();

		result = BZlibCompressionAlgorithm()
			.CreateDecompressingInputStream(tarGzFile,
			    zlibDecompressionParameters, tarIn);

		if (result == B_OK) {
			result = TarArchiveService::Unpack(*tarIn,
			    fLocalStorageDirectoryPath);

			if (result == B_OK) {
				if (0 != remove(tarGzFilePath.Path())) {
					fprintf(stdout, "unable to delete the temporary tgz path; "
					    "%s\n", tarGzFilePath.Path());
				}
			}
		}

		delete tarGzFile;
	}

	printf("did complete fetching icons\n");

	return result;
}


void
ServerIconExportUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	path.SetTo(fLocalStorageDirectoryPath.Path());
	path.Append("hicn/info.json");
}


void
ServerIconExportUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
		// the "$" here indicates that the data is at the top level.
	jsonPath.SetTo("$");
}


const char*
ServerIconExportUpdateProcess::LoggingName() const
{
	return "icon-export-update";
}


status_t
ServerIconExportUpdateProcess::Download(BPath& tarGzFilePath)
{
	return DownloadToLocalFile(tarGzFilePath,
		ServerSettings::CreateFullUrl("/__pkgicon/all.tar.gz"), 0, 0);
}