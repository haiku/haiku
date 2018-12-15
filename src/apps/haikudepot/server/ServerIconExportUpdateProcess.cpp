/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerIconExportUpdateProcess.h"

#include <stdio.h>
#include <sys/stat.h>
#include <time.h>

#include <AutoLocker.h>
#include <Catalog.h>
#include <FileIO.h>
#include <support/StopWatch.h>
#include <support/ZlibCompressionAlgorithm.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "StorageUtils.h"
#include "TarArchiveService.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerIconExportUpdateProcess"


/*! This constructor will locate the cached data in a standardized location */

ServerIconExportUpdateProcess::ServerIconExportUpdateProcess(
	Model* model,
	uint32 serverProcessOptions)
	:
	AbstractServerProcess(serverProcessOptions),
	fModel(model),
	fCountIconsSet(0)
{
	if (fModel->IconStoragePath(fLocalIconStoragePath) != B_OK) {
		printf("[%s] unable to obtain the path for storing icons\n", Name());
		fLocalIconStoragePath.Unset();
		fLocalIconStore = NULL;
	} else {
		fLocalIconStore = new LocalIconStore(fLocalIconStoragePath);
	}
}


ServerIconExportUpdateProcess::~ServerIconExportUpdateProcess()
{
	delete fLocalIconStore;
}


const char*
ServerIconExportUpdateProcess::Name() const
{
	return "ServerIconExportUpdateProcess";
}


const char*
ServerIconExportUpdateProcess::Description() const
{
	return B_TRANSLATE("Synchronizing icons");
}


status_t
ServerIconExportUpdateProcess::RunInternal()
{
	status_t result = B_OK;

	if (NULL == fLocalIconStore || fLocalIconStoragePath.Path() == NULL)
		result = B_ERROR;

	if (IsSuccess(result) && HasOption(SERVER_PROCESS_DROP_CACHE)) {
		result = StorageUtils::RemoveDirectoryContents(fLocalIconStoragePath);
	}

	if (result == B_OK) {
		bool hasData;

		result = HasLocalData(&hasData);

		if (result == B_OK && ShouldAttemptNetworkDownload(hasData))
			result = _DownloadAndUnpack();

		if (IsSuccess(result)) {
			status_t hasDataResult = HasLocalData(&hasData);

			if (hasDataResult == B_OK && !hasData)
				result = HD_ERR_NO_DATA;
		}
	}

	if (IsSuccess(result) && !WasStopped())
		result = Populate();

	return result;
}


status_t
ServerIconExportUpdateProcess::Populate()
{
	BStopWatch watch("ServerIconExportUpdateProcess::Populate", true);
	DepotList depots = fModel->Depots();
	status_t result = B_OK;

	{
		AutoLocker<BLocker> locker(fModel->Lock());
		depots = fModel->Depots();
	}

	if (Logger::IsDebugEnabled()) {
		printf("[%s] will populate icons for %" B_PRId32 " depots\n", Name(),
			depots.CountItems());
	}

	for (int32 i = 0;
		(i < depots.CountItems()) && !WasStopped() && (result == B_OK);
		i++) {
		AutoLocker<BLocker> locker(fModel->Lock());
		DepotInfo depotInfo = depots.ItemAtFast(i);
		result = PopulateForDepot(depotInfo);
	}

	if (Logger::IsInfoEnabled()) {
		double secs = watch.ElapsedTime() / 1000000.0;
		printf("[%s] did populate %" B_PRId32 " packages' icons (%6.3g secs)\n",
			Name(), fCountIconsSet, secs);
	}

	return result;
}


/*! This method assumes that the model lock has been acquired */

status_t
ServerIconExportUpdateProcess::PopulateForDepot(const DepotInfo& depot)
{
	printf("[%s] will populate icons for depot [%s]\n",
		Name(), depot.Name().String());
	status_t result = B_OK;
	PackageList packages = depot.Packages();
	for(int32 j = 0;
		(j < packages.CountItems()) && !WasStopped() && (result == B_OK);
		j++) {
		const PackageInfoRef& packageInfoRef = packages.ItemAtFast(j);
		result = PopulateForPkg(packageInfoRef);

		if (result == B_FILE_NOT_FOUND)
			result = B_OK;
	}

	return result;
}


/*! This method assumes that the model lock has been acquired */

status_t
ServerIconExportUpdateProcess::PopulateForPkg(const PackageInfoRef& package)
{
	BPath bestIconPath;

	if ( fLocalIconStore->TryFindIconPath(
		package->Name(), bestIconPath) == B_OK) {

		BFile bestIconFile(bestIconPath.Path(), O_RDONLY);
		BitmapRef bitmapRef(new(std::nothrow)SharedBitmap(bestIconFile), true);
		package->SetIcon(bitmapRef);

		if (Logger::IsDebugEnabled()) {
			printf("[%s] have set the package icon for [%s] from [%s]\n",
				Name(), package->Name().String(), bestIconPath.Path());
		}

		fCountIconsSet++;

		return B_OK;
	}

	if (Logger::IsDebugEnabled()) {
		printf("[%s] did not set the package icon for [%s]; no data\n",
			Name(), package->Name().String());
	}

	return B_FILE_NOT_FOUND;
}


status_t
ServerIconExportUpdateProcess::_DownloadAndUnpack()
{
	BPath tarGzFilePath(tmpnam(NULL));
	status_t result = B_OK;

	printf("[%s] will start fetching icons\n", Name());

	result = _Download(tarGzFilePath);

	switch (result) {
		case HD_ERR_NOT_MODIFIED:
			printf("[%s] icons not modified - will use existing\n", Name());
			return result;
			break;
		case B_OK:
			return _Unpack(tarGzFilePath);
		default:
			return (_HandleDownloadFailure() != B_OK) ? result : B_OK;
	}
}


/*! if the download failed, but there are existing icons in place to use
    then use those icons.  To detect the existing files, look for the
    icons' meta-info file.
*/

status_t
ServerIconExportUpdateProcess::_HandleDownloadFailure()
{
	bool hasData;
	status_t result = HasLocalData(&hasData);

	if (result == B_OK) {
		if (hasData) {
			printf("[%s] failed to update data, but have old data anyway "
				"so will carry on with that\n", Name());
		} else {
			printf("[%s] failed to obtain data\n", Name());
			result = HD_ERR_NO_DATA;
		}
	} else {
		printf("[%s] unable to detect if there is local data\n", Name());
	}

	return result;
}


/*! The tar-ball data of icons has arrived and so old data needs to be purged
    to make way for the new data and the new data needs to be unpacked.
*/

status_t
ServerIconExportUpdateProcess::_Unpack(BPath& tarGzFilePath)
{
	status_t result;
	printf("[%s] delete any existing stored data\n", Name());
	StorageUtils::RemoveDirectoryContents(fLocalIconStoragePath);

	BFile *tarGzFile = new BFile(tarGzFilePath.Path(), O_RDONLY);
	BDataIO* tarIn;

	BZlibDecompressionParameters* zlibDecompressionParameters
		= new BZlibDecompressionParameters();

	result = BZlibCompressionAlgorithm()
		.CreateDecompressingInputStream(tarGzFile,
			zlibDecompressionParameters, tarIn);

	if (result == B_OK) {
		BStopWatch watch(
			"ServerIconExportUpdateProcess::DownloadAndUnpack_Unpack",
			true);

		result = TarArchiveService::Unpack(*tarIn,
			fLocalIconStoragePath, NULL);

		if (result == B_OK) {
			double secs = watch.ElapsedTime() / 1000000.0;
			printf("[%s] did unpack icon tgz in (%6.3g secs)\n", Name(),
				secs);

			if (0 != remove(tarGzFilePath.Path())) {
				printf("unable to delete the temporary tgz path; %s\n",
					tarGzFilePath.Path());
			}
		}
	}

	delete tarGzFile;
	printf("[%s] did complete unpacking icons\n", Name());
	return result;
}


status_t
ServerIconExportUpdateProcess::HasLocalData(bool* result) const
{
	BPath path;
	status_t status = GetStandardMetaDataPath(path);

	if (status != B_OK)
		return status;

	off_t size;

	status = StorageUtils::ExistsObject(path, result, NULL, &size);

	if (status == B_OK && size == 0)
		*result = false;

	return status;
}


status_t
ServerIconExportUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	status_t result = fModel->IconStoragePath(path);

	if (result != B_OK)
		return result;

	path.Append("hicn/info.json");
	return B_OK;
}


void
ServerIconExportUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
		// the "$" here indicates that the data is at the top level.
	jsonPath.SetTo("$");
}


status_t
ServerIconExportUpdateProcess::_Download(BPath& tarGzFilePath)
{
	return DownloadToLocalFileAtomically(tarGzFilePath,
		ServerSettings::CreateFullUrl("/__pkgicon/all.tar.gz"));
}



