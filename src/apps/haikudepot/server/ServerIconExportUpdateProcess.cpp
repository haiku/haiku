/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "ServerIconExportUpdateProcess.h"

#include <sys/stat.h>
#include <time.h>

#include <AutoDeleter.h>
#include <AutoLocker.h>
#include <Catalog.h>
#include <FileIO.h>

#include "DataIOUtils.h"
#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "StandardMetaDataJsonEventListener.h"
#include "StorageUtils.h"
#include "TarArchiveService.h"

#define ENTRY_PATH_METADATA "hicn/info.json"

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerIconExportUpdateProcess"


/*!	This listener will scan the files that are available in the tar file and
	find the meta-data file.  This is a JSON piece of data that describes the
	timestamp of the last modified data in the tar file.  This is then used to
	establish if the server has any newer data and hence if it is worth
	downloading fresh data.  The process uses the standard HTTP
	If-Modified-Since header.
*/

class InfoJsonExtractEntryListener : public TarEntryListener
{
public:
								InfoJsonExtractEntryListener();
	virtual						~InfoJsonExtractEntryListener();

	virtual status_t			Handle(
									const TarArchiveHeader& header,
									size_t offset,
									BDataIO *data);

			BPositionIO&		GetInfoJsonData();
private:
			BMallocIO			fInfoJsonData;
};


InfoJsonExtractEntryListener::InfoJsonExtractEntryListener()
{
}

InfoJsonExtractEntryListener::~InfoJsonExtractEntryListener()
{
}


BPositionIO&
InfoJsonExtractEntryListener::GetInfoJsonData()
{
	return fInfoJsonData;
}


status_t
InfoJsonExtractEntryListener::Handle( const TarArchiveHeader& header,
	size_t offset, BDataIO *data)
{
	if (header.Length() > 0 && header.FileName() == ENTRY_PATH_METADATA) {
		status_t copyResult = DataIOUtils::CopyAll(&fInfoJsonData, data);
		if (copyResult == B_OK) {
			HDINFO("[InfoJsonExtractEntryListener] did extract [%s]",
				ENTRY_PATH_METADATA);
			fInfoJsonData.Seek(0, SEEK_SET);
			return B_CANCELED;
				// this will prevent further scanning of the tar file
		}
		return copyResult;
	}

	return B_OK;
}


/*!	This constructor will locate the cached data in a standardized location
*/

ServerIconExportUpdateProcess::ServerIconExportUpdateProcess(
	Model* model,
	uint32 serverProcessOptions)
	:
	AbstractSingleFileServerProcess(serverProcessOptions),
	fModel(model)
{
}


ServerIconExportUpdateProcess::~ServerIconExportUpdateProcess()
{
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
ServerIconExportUpdateProcess::ProcessLocalData()
{
	status_t result = fModel->InitPackageIconRepository();
	_NotifyPackagesWithIconsInDepots();
	return result;
}


status_t
ServerIconExportUpdateProcess::GetLocalPath(BPath& path) const
{
	return fModel->IconTarPath(path);
}


/*!	This overridden method implementation seems inefficient because it will
	apparently scan the entire tarball for the file that it is looking for, but
	actually it will cancel the scan once it has found it's target object (the
	JSON data containing the meta-data) and by convention, the server side will
	place the meta-data as one of the first objects in the tar-ball so it will
	find it quickly.
*/

status_t
ServerIconExportUpdateProcess::IfModifiedSinceHeaderValue(BString& headerValue) const
{
	headerValue.SetTo("");

	BPath tarPath;
	status_t result = GetLocalPath(tarPath);

	// early exit if the tar file is not there.

	if (result == B_OK) {
		off_t size;
		bool hasFile;

		result = StorageUtils::ExistsObject(tarPath, &hasFile, NULL, &size);

		if (result == B_OK && (!hasFile || size == 0))
			return result;
	}

	if (result == B_OK) {
		BFile *tarIo = new BFile(tarPath.Path(), O_RDONLY);
		ObjectDeleter<BFile> tarIoDeleter(tarIo);
		InfoJsonExtractEntryListener* extractDataListener
			= new InfoJsonExtractEntryListener();
		ObjectDeleter<InfoJsonExtractEntryListener> extractDataListenerDeleter(
			extractDataListener);
		result = TarArchiveService::ForEachEntry(*tarIo, extractDataListener);

		if (result == B_CANCELED) {
			// the cancellation is expected because it will cancel when it finds
			// the meta-data file to avoid any further cost of traversing the
			// tar-ball.
			result = B_OK;

			StandardMetaData metaData;
			BString metaDataJsonPath;
			GetStandardMetaDataJsonPath(metaDataJsonPath);
			StandardMetaDataJsonEventListener parseInfoJsonListener(
				metaDataJsonPath, metaData);
			BPrivate::BJson::Parse(
				&(extractDataListener->GetInfoJsonData()),
				&parseInfoJsonListener);

			result = parseInfoJsonListener.ErrorStatus();

		// An example of this output would be; 'Fri, 24 Oct 2014 19:32:27 +0000'

			if (result == B_OK) {
				SetIfModifiedSinceHeaderValueFromMetaData(
					headerValue, metaData);
			} else {
				HDERROR("[%s] unable to parse the meta data from the tar file",
					Name());
			}
		} else {
			HDERROR("[%s] did not find the metadata [%s] in the tar",
				Name(), ENTRY_PATH_METADATA);
			result = B_BAD_DATA;
		}
	}

	return result;
}


status_t
ServerIconExportUpdateProcess::GetStandardMetaDataPath(BPath& path) const
{
	return B_ERROR;
		// unsupported
}


void
ServerIconExportUpdateProcess::GetStandardMetaDataJsonPath(
	BString& jsonPath) const
{
	jsonPath.SetTo("$");
		// the "$" here indicates that the data is at the top level.
}


BString
ServerIconExportUpdateProcess::UrlPathComponent()
{
	return "/__pkgicon/all.tar.gz";
}


void
ServerIconExportUpdateProcess::_NotifyPackagesWithIconsInDepots() const
{
	for (int32 d = 0; d < fModel->CountDepots(); d++) {
		_NotifyPackagesWithIconsInDepot(fModel->DepotAtIndex(d));
	}
}


void
ServerIconExportUpdateProcess::_NotifyPackagesWithIconsInDepot(
	const DepotInfoRef& depot) const
{
	PackageIconRepository& packageIconRepository
		= fModel->GetPackageIconRepository();
	const PackageList& packages = depot->Packages();
	for (int32 p = 0; p < packages.CountItems(); p++) {
		AutoLocker<BLocker> locker(fModel->Lock());
		const PackageInfoRef& packageInfoRef = packages.ItemAtFast(p);
		if (packageIconRepository.HasAnyIcon(packageInfoRef->Name()))
			packages.ItemAtFast(p)->NotifyChangedIcon();
	}
}