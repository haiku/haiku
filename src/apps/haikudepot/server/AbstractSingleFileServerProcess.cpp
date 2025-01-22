/*
 * Copyright 2017-2025, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractSingleFileServerProcess.h"

#include <AutoLocker.h>
#include <StopWatch.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "StorageUtils.h"


AbstractSingleFileServerProcess::AbstractSingleFileServerProcess(uint32 options)
	:
	AbstractServerProcess(options),
	fDownloadDurationSeconds(0.0),
	fProcessLocalDataDurationSeconds(0.0)
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
		BStopWatch stopWatch("download", true);
		result = DownloadToLocalFileAtomically(localPath,
			ServerSettings::CreateFullUrl(urlPathComponent));
		fDownloadDurationSeconds = ((double)stopWatch.ElapsedTime() / 1000000.0);

		if (!IsSuccess(result)) {
			if (hasData) {
				HDINFO("[%s] failed to update data, but have old data anyway so carry on with that",
					Name());
				result = B_OK;
			} else {
				HDERROR("[%s] failed to obtain data", Name());
			}
		} else {
			HDINFO("[%s] did fetch data", Name());
		}
	}

	if (IsSuccess(result)) {
		status_t hasDataResult = StorageUtils::ExistsObject(localPath, &hasData, NULL, &size);

		hasData = hasData && size > 0;

		if (hasDataResult == B_OK && !hasData) {
			HDINFO("[%s] there is no data to process", Name());
			result = HD_ERR_NO_DATA;
		}
	}

	if (IsSuccess(result)) {
		HDINFO("[%s] will process data", Name());

		BStopWatch stopWatch("process local data", true);
		result = ProcessLocalData();
		fProcessLocalDataDurationSeconds = ((double)stopWatch.ElapsedTime() / 1000000.0);

		switch (result) {
			case B_OK:
				HDINFO("[%s] did process data", Name());
				break;
			default:
				HDERROR("[%s] failed processing data", Name());
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


BString
AbstractSingleFileServerProcess::LogReport()
{
	BString result;
	result.Append(AbstractProcess::LogReport());

	AutoLocker<BLocker> locker(&fLock);

	if (ProcessState() == PROCESS_COMPLETE) {
		BString downloadLogLine;
		BString localDataLogLine;
		downloadLogLine.SetToFormat("\n - download %6.3f", fDownloadDurationSeconds);
		localDataLogLine.SetToFormat("\n - process local data %6.3f",
			fProcessLocalDataDurationSeconds);
		result.Append(downloadLogLine);
		result.Append(localDataLogLine);
	}

	return result;
}
