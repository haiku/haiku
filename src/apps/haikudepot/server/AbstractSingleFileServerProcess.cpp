/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractSingleFileServerProcess.h"

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


AbstractSingleFileServerProcess::AbstractSingleFileServerProcess(
	uint32 options)
	:
	AbstractServerProcess(options)
{
}


AbstractSingleFileServerProcess::~AbstractSingleFileServerProcess()
{
}


status_t
AbstractSingleFileServerProcess::RunInternal()
{
	HDINFO("[%s] will fetch data", Name());
	BPath localPath;
	status_t result = GetLocalPath(localPath);

	if (result != B_OK)
		return result;

	BString urlPathComponent = UrlPathComponent();

	if (IsSuccess(result) && HasOption(SERVER_PROCESS_DROP_CACHE))
		result = DeleteLocalFile(localPath);

	bool hasData = false;
	off_t size;

	if (IsSuccess(result))
		result = StorageUtils::ExistsObject(localPath, &hasData, NULL, &size);

	hasData = hasData && size > 0;

	if (IsSuccess(result) && ShouldAttemptNetworkDownload(hasData)) {
		result = DownloadToLocalFileAtomically(
			localPath,
			ServerSettings::CreateFullUrl(urlPathComponent));

		if (!IsSuccess(result)) {
			if (hasData) {
				HDINFO("[%s] failed to update data, but have old data "
					"anyway so carry on with that", Name());
				result = B_OK;
			} else
				HDERROR("[%s] failed to obtain data", Name());
		} else
			HDINFO("[%s] did fetch data", Name());
	}

	if (IsSuccess(result)) {
		status_t hasDataResult = StorageUtils::ExistsObject(
			localPath, &hasData, NULL, &size);

		hasData = hasData && size > 0;

		if (hasDataResult == B_OK && !hasData)
			result = HD_ERR_NO_DATA;
	}

	if (IsSuccess(result)) {
		HDINFO("[%s] will process data", Name());
		result = ProcessLocalData();

		switch (result) {
			case B_OK:
				HDINFO("[%s] did process data", Name());
				break;
			default:
				MoveDamagedFileAside(localPath);
				break;
		}
	}

	return result;
}


status_t
AbstractSingleFileServerProcess::GetStandardMetaDataPath(BPath& path) const
{
	return GetLocalPath(path);
}