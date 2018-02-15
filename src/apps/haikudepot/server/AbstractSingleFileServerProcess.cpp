/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "AbstractSingleFileServerProcess.h"

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


AbstractSingleFileServerProcess::AbstractSingleFileServerProcess(
	AbstractServerProcessListener* listener, uint32 options)
	:
	AbstractServerProcess(listener, options)
{
}


AbstractSingleFileServerProcess::~AbstractSingleFileServerProcess()
{
}


status_t
AbstractSingleFileServerProcess::RunInternal()
{
	if (Logger::IsInfoEnabled())
		printf("[%s] will fetch data\n", Name());

	BPath localPath = LocalPath();
	BString urlPathComponent = UrlPathComponent();
	status_t result = B_OK;

	if (IsSuccess(result) && HasOption(SERVER_PROCESS_DROP_CACHE))
		result = DeleteLocalFile(localPath);

	bool hasData;
	off_t size;

	if (IsSuccess(result))
		result = StorageUtils::ExistsObject(localPath, &hasData, NULL, &size);

	hasData = hasData && size > 0;

	if (IsSuccess(result) && ShouldAttemptNetworkDownload(hasData)) {
		result = DownloadToLocalFileAtomically(
			localPath,
			ServerSettings::CreateFullUrl(urlPathComponent));
	}

	if (IsSuccess(result)) {
		status_t hasDataResult = StorageUtils::ExistsObject(
			localPath, &hasData, NULL, &size);

		hasData = hasData && size > 0;

		if (hasDataResult == B_OK && !hasData)
			result = HD_ERR_NO_DATA;
	}

	if (IsSuccess(result)) {
		if (Logger::IsInfoEnabled())
			printf("[%s] did fetch data\n", Name());

			// now load the data in and process it.

		printf("[%s] will process data\n", Name());
		result = ProcessLocalData();

		switch (result) {
			case B_OK:
				printf("[%s] did process data\n", Name());
				break;
			default:
				MoveDamagedFileAside(localPath);
				break;
		}
	}

	return result;
}

