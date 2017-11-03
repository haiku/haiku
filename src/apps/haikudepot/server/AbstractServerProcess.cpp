/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "AbstractServerProcess.h"

#include <errno.h>
#include <string.h>

#include <AutoDeleter.h>
#include <FileIO.h>
#include <HttpRequest.h>
#include <HttpTime.h>
#include <UrlProtocolRoster.h>

#include <support/ZlibCompressionAlgorithm.h>

#include "ServerSettings.h"
#include "StandardMetaDataJsonEventListener.h"
#include "ToFileUrlProtocolListener.h"


#define MAX_REDIRECTS 3
#define MAX_FAILURES 2

#define HTTP_STATUS_FOUND 302
#define HTTP_STATUS_NOT_MODIFIED 304

// 30 seconds
#define TIMEOUT_MICROSECONDS 3e+7


status_t
AbstractServerProcess::IfModifiedSinceHeaderValue(BString& headerValue) const
{
	BPath metaDataPath;
	BString jsonPath;

	GetStandardMetaDataPath(metaDataPath);
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
		fprintf(stderr, "unable to parse the meta-data date and time -"
			" cannot set the 'If-Modified-Since' header\n");
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


bool
AbstractServerProcess::LooksLikeGzip(const char *pathStr) const
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
		fprintf(stderr, "unable to find the meta data file at [%s]\n",
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


status_t
AbstractServerProcess::DownloadToLocalFile(const BPath& targetFilePath,
	const BUrl& url, uint32 redirects, uint32 failures)
{
	if (redirects > MAX_REDIRECTS) {
		fprintf(stdout, "exceeded %d redirects --> failure\n", MAX_REDIRECTS);
		return B_IO_ERROR;
	}

	if (failures > MAX_FAILURES) {
		fprintf(stdout, "exceeded %d failures\n", MAX_FAILURES);
		return B_IO_ERROR;
	}

	fprintf(stdout, "will stream '%s' to [%s]\n", url.UrlString().String(),
		targetFilePath.Path());

	ToFileUrlProtocolListener listener(targetFilePath, LoggingName(),
		ServerSettings::UrlConnectionTraceLoggingEnabled());

	BHttpHeaders headers;
	ServerSettings::AugmentHeaders(headers);

	BString ifModifiedSinceHeader;
	status_t ifModifiedSinceHeaderStatus = IfModifiedSinceHeaderValue(
		ifModifiedSinceHeader);

	if (ifModifiedSinceHeaderStatus == B_OK &&
		ifModifiedSinceHeader.Length() > 0) {
		headers.AddHeader("If-Modified-Since", ifModifiedSinceHeader);
	}

	BHttpRequest *request = dynamic_cast<BHttpRequest *>(
		BUrlProtocolRoster::MakeRequest(url, &listener));
	ObjectDeleter<BHttpRequest> requestDeleter(request);
	request->SetHeaders(headers);
	request->SetMaxRedirections(0);
	request->SetTimeout(TIMEOUT_MICROSECONDS);

	thread_id thread = request->Run();
	wait_for_thread(thread, NULL);

	const BHttpResult& result = dynamic_cast<const BHttpResult&>(
		request->Result());

	int32 statusCode = result.StatusCode();

	if (BHttpRequest::IsSuccessStatusCode(statusCode)) {
		fprintf(stdout, "did complete streaming data\n");
		return B_OK;
	} else if (statusCode == HTTP_STATUS_NOT_MODIFIED) {
		fprintf(stdout, "remote data has not changed since [%s]\n",
			ifModifiedSinceHeader.String());
		return APP_ERR_NOT_MODIFIED;
	} else if (BHttpRequest::IsRedirectionStatusCode(statusCode)) {
		const BHttpHeaders responseHeaders = result.Headers();
		const char *locationValue = responseHeaders["Location"];

		if (locationValue != NULL && strlen(locationValue) != 0) {
			BUrl location(result.Url(), locationValue);
			fprintf(stdout, "will redirect to; %s\n",
				location.UrlString().String());
				return DownloadToLocalFile(targetFilePath, location, redirects + 1, 0);
		}

		fprintf(stdout, "unable to find 'Location' header for redirect\n");
		return B_IO_ERROR;
	} else {
		if (statusCode == 0 || (statusCode / 100) == 5) {
			fprintf(stdout, "error response from server; %" B_PRId32 " --> "
				"retry...\n", statusCode);
			return DownloadToLocalFile(targetFilePath, url, redirects, failures + 1);
		}

		fprintf(stdout, "unexpected response from server; %" B_PRId32 "\n",
			statusCode);
		return B_IO_ERROR;
	}
}