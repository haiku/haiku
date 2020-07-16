/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
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

#include "DataIOUtils.h"
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

	if (result == B_OK)
		SetIfModifiedSinceHeaderValueFromMetaData(headerValue, metaData);
	else {
		HDERROR("unable to parse the meta-data date and time from [%s]"
			" - cannot set the 'If-Modified-Since' header",
			metaDataPath.Path());
	}

	return result;
}


/*static*/ void
AbstractServerProcess::SetIfModifiedSinceHeaderValueFromMetaData(
	BString& headerValue,
	const StandardMetaData& metaData)
{
	// An example of this output would be; 'Fri, 24 Oct 2014 19:32:27 +0000'
	BDateTime modifiedDateTime = metaData
		.GetDataModifiedTimestampAsDateTime();
	BPrivate::BHttpTime modifiedHttpTime(modifiedDateTime);
	headerValue.SetTo(modifiedHttpTime
		.ToString(BPrivate::B_HTTP_TIME_FORMAT_COOKIE));
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
		HDERROR("the meta data was read from [%s], but no values "
			"were extracted", path.Path());
		return B_BAD_DATA;
	}

	return B_OK;
}


/* static */ bool
AbstractServerProcess::_LooksLikeGzip(const char *pathStr)
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
		HDERROR("[%s] unable to find the meta data file at [%s]", Name(),
			path.Path());
		return B_FILE_NOT_FOUND;
	}

	BFileIO rawInput(file, true); // takes ownership

		// if the file extension ends with '.gz' then the data will be
		// compressed and the algorithm needs to decompress the data as
		// it is parsed.

	if (_LooksLikeGzip(pathStr)) {
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

	// if the data is coming in as .gz, but is not stored as .gz then
	// the data should be decompressed in the temporary file location
	// before being shifted into place.

	if (result == B_OK
			&& _LooksLikeGzip(url.Path())
			&& !_LooksLikeGzip(targetFilePath.Path()))
		result = _DeGzipInSitu(temporaryFilePath);

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
				HDINFO("[%s] did rename [%s] --> [%s]",
					Name(), temporaryFilePath.Path(), targetFilePath.Path());
				result = B_IO_ERROR;
			}
		}
	}

	return result;
}


/*static*/ status_t
AbstractServerProcess::_DeGzipInSitu(const BPath& path)
{
	const char* tmpPath = tmpnam(NULL);
	status_t result = B_OK;

	{
		BFile file(path.Path(), O_RDONLY);
		BFile tmpFile(tmpPath, O_WRONLY | O_CREAT);

		BDataIO* gzDecompressedInput = NULL;
		BZlibDecompressionParameters* zlibDecompressionParameters
			= new BZlibDecompressionParameters();

		result = BZlibCompressionAlgorithm()
			.CreateDecompressingInputStream(&file,
				zlibDecompressionParameters, gzDecompressedInput);

		if (result == B_OK) {
			ObjectDeleter<BDataIO> gzDecompressedInputDeleter(
				gzDecompressedInput);
			result = DataIOUtils::CopyAll(&tmpFile, gzDecompressedInput);
		}
	}

	if (result == B_OK) {
		if (rename(tmpPath, path.Path()) != 0) {
			HDERROR("unable to move the uncompressed data into place");
			result = B_ERROR;
		}
	}
	else {
		HDERROR("it was not possible to decompress the data");
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
		HDINFO("[%s] exceeded %d redirects --> failure", Name(),
			MAX_REDIRECTS);
		return B_IO_ERROR;
	}

	if (failures > MAX_FAILURES) {
		HDINFO("[%s] exceeded %d failures", Name(), MAX_FAILURES);
		return B_IO_ERROR;
	}

	HDINFO("[%s] will stream '%s' to [%s]", Name(), url.UrlString().String(),
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

	BUrlRequest* request = BUrlProtocolRoster::MakeRequest(url, &listener);
	if (request == NULL)
		return B_NO_MEMORY;

	fRequest = dynamic_cast<BHttpRequest *>(request);
	if (fRequest == NULL) {
		delete request;
		return B_ERROR;
	}
	fRequest->SetHeaders(headers);
	fRequest->SetMaxRedirections(0);
	fRequest->SetTimeout(TIMEOUT_MICROSECONDS);
	thread = fRequest->Run();

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
		HDINFO("[%s] did complete streaming data [%"
			B_PRIdSSIZE " bytes]", Name(), listener.ContentLength());
		return B_OK;
	} else if (statusCode == B_HTTP_STATUS_NOT_MODIFIED) {
		HDINFO("[%s] remote data has not changed since [%s]", Name(),
			ifModifiedSinceHeader.String());
		return HD_ERR_NOT_MODIFIED;
	} else if (statusCode == B_HTTP_STATUS_PRECONDITION_FAILED) {
		ServerHelper::NotifyClientTooOld(responseHeaders);
		return HD_CLIENT_TOO_OLD;
	} else if (BHttpRequest::IsRedirectionStatusCode(statusCode)) {
		if (location.Length() != 0) {
			BUrl redirectUrl(result.Url(), location);
			HDINFO("[%s] will redirect to; %s",
				Name(), redirectUrl.UrlString().String());
			return DownloadToLocalFile(targetFilePath, redirectUrl,
				redirects + 1, 0);
		}

		HDERROR("[%s] unable to find 'Location' header for redirect", Name());
		return B_IO_ERROR;
	} else {
		if (statusCode == 0 || (statusCode / 100) == 5) {
			HDERROR("error response from server [%" B_PRId32 "] --> retry...",
				statusCode);
			return DownloadToLocalFile(targetFilePath, url, redirects,
				failures + 1);
		}

		HDERROR("[%s] unexpected response from server [%" B_PRId32 "]",
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
		HDERROR("[%s] unable to move damaged file [%s] aside to [%s]",
			Name(), currentFilePath.Path(), damagedFilePath.Path());
		return B_IO_ERROR;
	}

	HDINFO("[%s] did move damaged file [%s] aside to [%s]",
		Name(), currentFilePath.Path(), damagedFilePath.Path());

	return B_OK;
}


bool
AbstractServerProcess::IsSuccess(status_t e) {
	return e == B_OK || e == HD_ERR_NOT_MODIFIED;
}
