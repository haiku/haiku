/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ServerIconExportUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <Autolock.h>
#include <FileIO.h>
#include <support/StopWatch.h>
#include <support/ZlibCompressionAlgorithm.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerSettings.h"
#include "StorageUtils.h"
#include "TarArchiveService.h"


/*! This constructor will locate the cached data in a standardized location */

ServerIconExportUpdateProcess::ServerIconExportUpdateProcess(
	AbstractServerProcessListener* listener,
	const BPath& localStorageDirectoryPath,
	Model* model,
	uint32 options)
	:
	AbstractServerProcess(listener, options),
	fLocalStorageDirectoryPath(localStorageDirectoryPath),
	fModel(model),
	fLocalIconStore(LocalIconStore(localStorageDirectoryPath)),
	fCountIconsSet(0)
{
}


ServerIconExportUpdateProcess::~ServerIconExportUpdateProcess()
{
}


const char*
ServerIconExportUpdateProcess::Name()
{
	return "ServerIconExportUpdateProcess";
}


status_t
ServerIconExportUpdateProcess::RunInternal()
{
	status_t result = B_OK;

	if (IsSuccess(result) && HasOption(SERVER_PROCESS_DROP_CACHE)) {
		result = StorageUtils::RemoveDirectoryContents(
			fLocalStorageDirectoryPath);
	}

	if (IsSuccess(result)) {
		bool hasData;

		result = HasLocalData(&hasData);

		if (IsSuccess(result) && ShouldAttemptNetworkDownload(hasData))
			result = DownloadAndUnpack();

		if (IsSuccess(result)) {
			status_t hasDataResult = HasLocalData(&hasData);

			if (IsSuccess(hasDataResult) && !hasData)
				result = HD_ERR_NO_DATA;
		}
	}

	if (IsSuccess(result) && !WasStopped())
		result = Populate();

	return result;
}


status_t
ServerIconExportUpdateProcess::PopulateForPkg(const PackageInfoRef& package)
{
	BPath bestIconPath;

	if ( fLocalIconStore.TryFindIconPath(
		package->Name(), bestIconPath) == B_OK) {

		BFile bestIconFile(bestIconPath.Path(), O_RDONLY);
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(bestIconFile), true);
		// TODO; somehow handle the locking!
		//BAutolock locker(&fLock);
		package->SetIcon(bitmapRef);

		if (Logger::IsDebugEnabled()) {
			fprintf(stdout, "have set the package icon for [%s] from [%s]\n",
				package->Name().String(), bestIconPath.Path());
		}

		fCountIconsSet++;

		return B_OK;
	}

	if (Logger::IsDebugEnabled()) {
		fprintf(stdout, "did not set the package icon for [%s]; no data\n",
			package->Name().String());
	}

	return B_FILE_NOT_FOUND;
}


bool
ServerIconExportUpdateProcess::ConsumePackage(
	const PackageInfoRef& packageInfoRef, void* context)
{
	PopulateForPkg(packageInfoRef);
	return !WasStopped();
}


status_t
ServerIconExportUpdateProcess::Populate()
{
	BStopWatch watch("ServerIconExportUpdateProcess::Populate", true);
	fModel->ForAllPackages(this, NULL);

	if (Logger::IsInfoEnabled()) {
		double secs = watch.ElapsedTime() / 1000000.0;
		fprintf(stdout, "did populate %" B_PRId32 " packages' icons"
		 	" (%6.3g secs)\n", fCountIconsSet, secs);
	}

	return B_OK;
}


status_t
ServerIconExportUpdateProcess::DownloadAndUnpack()
{
	BPath tarGzFilePath(tmpnam(NULL));
	status_t result = B_OK;

	printf("will start fetching icons\n");

	result = Download(tarGzFilePath);

	if (result == B_OK) {
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
			BStopWatch watch("ServerIconExportUpdateProcess::DownloadAndUnpack_Unpack", true);

			result = TarArchiveService::Unpack(*tarIn,
			    fLocalStorageDirectoryPath, NULL);

			if (result == B_OK) {
				double secs = watch.ElapsedTime() / 1000000.0;
				fprintf(stdout, "did unpack icon tgz in (%6.3g secs)\n", secs);

				if (0 != remove(tarGzFilePath.Path())) {
					fprintf(stdout, "unable to delete the temporary tgz path; "
					    "%s\n", tarGzFilePath.Path());
				}
			}
		}

		delete tarGzFile;

		printf("did complete fetching icons\n");
	}

	return result;
}


status_t
ServerIconExportUpdateProcess::HasLocalData(bool* result) const
{
	BPath path;
	off_t size;
	GetStandardMetaDataPath(path);
	return StorageUtils::ExistsObject(path, result, NULL, &size)
		&& size > 0;
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


status_t
ServerIconExportUpdateProcess::Download(BPath& tarGzFilePath)
{
	return DownloadToLocalFileAtomically(tarGzFilePath,
		ServerSettings::CreateFullUrl("/__pkgicon/all.tar.gz"));
}



