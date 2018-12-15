/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "AbstractServerProcess.h"

#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <AutoDeleter.h>
#include <FileIO.h>
#include <HttpTime.h>
#include <UrlProtocolRoster.h>

#include <support/ZlibCompressionAlgorithm.h>

#include "HaikuDepotConstants.h"
#include "Logger.h"
#include "ServerHelper.h"
#include "ServerSettings.h"
#include "StandardMetaDataJsonEventListener.h"
#include "StorageUtils.h"
#include "ToFileUrlProtocolListener.h"


#define MAX_REDIRECTS 3
#define MAX_FAILURES 2


// 30 seconds
#define TIMEOUT_MICROSECONDS 3e+7


AbstractServerProcess::AbstractServerProcess(uint32 options)
	:
	AbstractProcess(),
	fOptions(options),
	fRequest(NULL)
{
}


AbstractServerProcess::~AbstractServerProcess()
{
}


bool
AbstractServerProcess::HasOption(uint32 flag)
{
	return (fOptions & flag) == flag;
}


bool
AbstractServerProcess::ShouldAttemptNetworkDownload(bool hasDataAlready)
{
	return
		!HasOption(SERVER_PROCESS_NO_NETWORKING)
		&& !(HasOption(SERVER_PROCESS_PREFER_CACHE) && hasDataAlready);
}


status_t
AbstractServerProcess::StopInternal()
{
	if (fRequest != NULL) {
		return fRequest->Stop();
	}

	return AbstractProcess::StopInternal();
}


status_t
AbstractServerProcess::IfModifiedSinceHeaderValue(BString& headerValue) const
{
	BPath metaDataPath;
	BString jsonPath;

	status_t result = GetStandardMetaDataPath(metaDataPath);

	if (result != B_OK)
		return result;

	GetStandardMetaDataJsonPath(jsonPath);

	return IfModifiedSinceHeaderValue(headerValue, metaDataPath, jsonPath);
}


status_t
AbstractServerProcess::IfModifiedSinceHeaderValue(BString& headerValue,
	const BPath& metaDataPath, const BString& jsonPath) const
{
	headerValue.SetTo("");
	struct stat s;

	if (-1 == stat(metaDataPath.Path(), &s)) {
		if (ENOENT != errno)
			 return B_ERROR;

		return B_FILE_NOT_FOUND;
	}

	if (s.st_size == 0)
		return B_BAD_VALUE;

	StandardMetaData metaData;
	status_t result = PopulateMetaData(metaData, metaDataPath, jsonPath);

	if (result == B_OK) {

		// An example of this output would be; 'Fri, 24 Oct 2014 19:32:27 +0000'

		BDateTime modifiedDateTime = metaData
			.GetDataModifiedTimestampAsDateTime();
		BPrivate::BHttpTime modifiedHttpTime(modifiedDateTime);
		headerValue.SetTo(modifiedHttpTime
			.ToString(BPrivate::B_HTTP_TIME_FORMAT_COOKIE));
	} else {
		fprintf(stderr, "unable to parse the meta-data date and time from [%s]"
			" - cannot set the 'If-Modified-Since' header\n",
			metaDataPath.Path());
	}

	return result;
}


status_t
AbstractServerProcess::PopulateMetaData(
	StandardMetaData& metaData, const BPath& path,
	const BString& jsonPath) const
{
	StandardMetaDataJsonEventListener listener(jsonPath, metaData);
	status_t result = ParseJsonFromFileWithListener(&listener, path);

	if (result != B_OK)
		return result;

	result = listener.ErrorStatus();

	if (result != B_OK)
		return result;

	if (!metaData.IsPopulated()) {
		fprintf(stderr, "the meta data was read from [%s], but no values "
			"were extracted\n", path.Path());
		return B_BAD_DATA;
	}

	return B_OK;
}


/* static */ bool
AbstractServerProcess::LooksLikeGzip(const char *pathStr)
{
	int l = strlen(pathStr);
	return l > 4 && 0 == strncmp(&pathStr[l - 3], ".gz", 3);
}


/*!	Note that a B_OK return code from this method may not indicate that the
	listening process went well.  One has to see if there was an error in
	the listener.
*/

status_t
AbstractServerProcess::ParseJsonFromFileWithListener(
	BJsonEventListener *listener,
	const BPath& path) const
{
	const char* pathStr = path.Path();
	FILE* file = fopen(pathStr, "rb");

	if (file == NULL) {
		printf("[%s] unable to find the meta data file at [%s]\n", Name(),
			path.Path());
		return B_FILE_NOT_FOUND;
	}

	BFileIO rawInput(file, true); // takes ownership

		// if the file extension ends with '.gz' then the data will be
		// compressed and the algorithm needs to decompress the data as
		// it is parsed.

	if (LooksLikeGzip(pathStr)) {
		BDataIO* gzDecompressedInput = NULL;
		BZlibDecompressionParameters* zlibDecompressionParameters
			= new BZlibDecompressionParameters();

		status_t result = BZlibCompressionAlgorithm()
			.CreateDecompressingInputStream(&rawInput,
				zlibDecompressionParameters, gzDecompressedInput);

		if (B_OK != result)
			return result;

		ObjectDeleter<BDataIO> gzDecompressedInputDeleter(gzDecompressedInput);
		BPrivate::BJson::Parse(gzDecompressedInput, listener);
	} else {
		BPrivate::BJson::Parse(&rawInput, listener);
	}

	return B_OK;
}


/*! In order to reduce the chance of failure half way through downloading a
    large file, this method will download the file to a temporary file and
    then it can rename the file to the final target file.
*/

status_t
AbstractServerProcess::DownloadToLocalFileAtomically(
	const BPath& targetFilePath,
	const BUrl& url)
{
	BPath temporaryFilePath(tmpnam(NULL), NULL, true);
	status_t result = DownloadToLocalFile(
		temporaryFilePath, url, 0, 0);

		// not copying if the data has not changed because the data will be
		// zero length.  This is if the result is APP_ERR_NOT_MODIFIED.
	if (result == B_OK) {

			// if the file is zero length then assume that something has
			// gone wrong.
		off_t size;
		bool hasFile;

		result = StorageUtils::ExistsObject(temporaryFilePath, &hasFile, NULL,
			&size);

		if (result == B_OK && hasFile && size > 0) {
			if (rename(temporaryFilePath.Path(), targetFilePath.Path()) != 0) {
				printf("[%s] did rename [%s] --> [%s]\n",
					Name(), temporaryFilePath.Path(), targetFilePath.Path());
				result = B_IO_ERROR;
			}
		}
	}

	return result;
}


status_t
AbstractServerProcess::DownloadToLocalFile(const BPath& targetFilePath,
	const BUrl& url, uint32 redirects, uint32 failures)
{
	if (WasStopped())
		return B_CANCELED;

	if (redirects > MAX_REDIRECTS) {
		printf("[%s] exceeded %d redirects --> failure\n", Name(),
			MAX_REDIRECTS);
		return B_IO_ERROR;
	}

	if (failures > MAX_FAILURES) {
		printf("[%s] exceeded %d failures\n", Name(), MAX_FAILURES);
		return B_IO_ERROR;
	}

	printf("[%s] will stream '%s' to [%s]\n", Name(), url.UrlString().String(),
		targetFilePath.Path());

	ToFileUrlProtocolListener listener(targetFilePath, Name(),
		Logger::IsTraceEnabled());

	BHttpHeaders headers;
	ServerSettings::AugmentHeaders(headers);

	BString ifModifiedSinceHeader;
	status_t ifModifiedSinceHeaderStatus = IfModifiedSinceHeaderValue(
		ifModifiedSinceHeader);

	if (ifModifiedSinceHeaderStatus == B_OK &&
		ifModifiedSinceHeader.Length() > 0) {
		headers.AddHeader("If-Modified-Since", ifModifiedSinceHeader);
	}

	thread_id thread;

	{
		fRequest = dynamic_cast<BHttpRequest *>(
			BUrlProtocolRoster::MakeRequest(url, &listener));
		fRequest->SetHeaders(headers);
		fRequest->SetMaxRedirections(0);
		fRequest->SetTimeout(TIMEOUT_MICROSECONDS);
		thread = fRequest->Run();
	}

	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		fRequest->Result());
	int32 statusCode = result.StatusCode();
	const BHttpHeaders responseHeaders = result.Headers();
	const char *locationC = responseHeaders["Location"];
	BString location;

	if (locationC != NULL)
		location.SetTo(locationC);

	delete fRequest;
	fRequest = NULL;

	if (BHttpRequest::IsSuccessStatusCode(statusCode)) {
		fprintf(stdout, "[%s] did complete streaming data [%"
			B_PRIdSSIZE " bytes]\n", Name(), listener.ContentLength());
		return B_OK;
	} else if (statusCode == B_HTTP_STATUS_NOT_MODIFIED) {
		fprintf(stdout, "[%s] remote data has not changed since [%s]\n",
			Name(), ifModifiedSinceHeader.String());
		return HD_ERR_NOT_MODIFIED;
	} else if (statusCode == B_HTTP_STATUS_PRECONDITION_FAILED) {
		ServerHelper::NotifyClientTooOld(responseHeaders);
		return HD_CLIENT_TOO_OLD;
	} else if (BHttpRequest::IsRedirectionStatusCode(statusCode)) {
		if (location.Length() != 0) {
			BUrl redirectUrl(result.Url(), location);
			fprintf(stdout, "[%s] will redirect to; %s\n",
				Name(), redirectUrl.UrlString().String());
			return DownloadToLocalFile(targetFilePath, redirectUrl,
				redirects + 1, 0);
		}

		fprintf(stdout, "[%s] unable to find 'Location' header for redirect\n",
			Name());
		return B_IO_ERROR;
	} else {
		if (statusCode == 0 || (statusCode / 100) == 5) {
			fprintf(stdout, "error response from server [%" B_PRId32 "] --> "
				"retry...\n", statusCode);
			return DownloadToLocalFile(targetFilePath, url, redirects,
				failures + 1);
		}

		fprintf(stdout, "[%s] unexpected response from server [%" B_PRId32 "]\n",
			Name(), statusCode);
		return B_IO_ERROR;
	}
}


status_t
AbstractServerProcess::DeleteLocalFile(const BPath& currentFilePath)
{
	if (0 == remove(currentFilePath.Path()))
		return B_OK;

	return B_IO_ERROR;
}


/*!	When a file is damaged or corrupted in some way, the file should be 'moved
    aside' so that it is not involved in the next update.  This method will
    create such an alternative 'damaged' file location and move this file to
    that location.
*/

status_t
AbstractServerProcess::MoveDamagedFileAside(const BPath& currentFilePath)
{
	BPath damagedFilePath;
	BString damagedLeaf;

	damagedLeaf.SetToFormat("%s__damaged", currentFilePath.Leaf());
	currentFilePath.GetParent(&damagedFilePath);
	damagedFilePath.Append(damagedLeaf.String());

	if (0 != rename(currentFilePath.Path(), damagedFilePath.Path())) {
		printf("[%s] unable to move damaged file [%s] aside to [%s]\n",
			Name(), currentFilePath.Path(), damagedFilePath.Path());
		return B_IO_ERROR;
	}

	printf("[%s] did move damaged file [%s] aside to [%s]\n",
		Name(), currentFilePath.Path(), damagedFilePath.Path());

	return B_OK;
}


bool
AbstractServerProcess::IsSuccess(status_t e) {
	return e == B_OK || e == HD_ERR_NOT_MODIFIED;
}
