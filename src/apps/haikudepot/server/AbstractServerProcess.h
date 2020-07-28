/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ABSTRACT_SERVER_PROCESS_H
#define ABSTRACT_SERVER_PROCESS_H

#include <HttpRequest.h>
#include <Json.h>
#include <String.h>
#include <Url.h>

#include "AbstractProcess.h"
#include "StandardMetaData.h"


typedef enum server_process_options {
	SERVER_PROCESS_NO_NETWORKING	= 1 << 0,
	SERVER_PROCESS_PREFER_CACHE		= 1 << 1,
	SERVER_PROCESS_DROP_CACHE		= 1 << 2
} server_process_options;


/*! This is the superclass of Processes that communicate with the Haiku Depot
    Server (HDS) system.
*/


class AbstractServerProcess : public AbstractProcess {
public:
								AbstractServerProcess(uint32 options);
	virtual						~AbstractServerProcess();

protected:
	virtual	status_t			GetStandardMetaDataPath(
									BPath& path) const = 0;
	virtual	void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const = 0;

	virtual	status_t			IfModifiedSinceHeaderValue(
									BString& headerValue) const;
			status_t			IfModifiedSinceHeaderValue(
									BString& headerValue,
									const BPath& metaDataPath,
									const BString& jsonPath) const;
	static	void				SetIfModifiedSinceHeaderValueFromMetaData(
									BString& headerValue,
									const StandardMetaData& metaData);

			status_t			PopulateMetaData(
									StandardMetaData& metaData,
									const BPath& path,
									const BString& jsonPath) const;

			status_t			ParseJsonFromFileWithListener(
									BJsonEventListener *listener,
									const BPath& path) const;

	virtual	status_t			DownloadToLocalFileAtomically(
									const BPath& targetFilePath,
									const BUrl& url);

			status_t			DeleteLocalFile(const BPath& filePath);
			status_t			MoveDamagedFileAside(const BPath& filePath);

			bool				HasOption(uint32 flag);
			bool				ShouldAttemptNetworkDownload(
									bool hasDataAlready);

	static	bool				IsSuccess(status_t e);

protected:
	virtual status_t			StopInternal();

private:
			status_t			DownloadToLocalFile(
									const BPath& targetFilePath,
									const BUrl& url,
									uint32 redirects, uint32 failures);

	static bool					_LooksLikeGzip(const char *pathStr);
	static status_t				_DeGzipInSitu(const BPath& path);

private:
			uint32				fOptions;

			BHttpRequest*		fRequest;
};

#endif // ABSTRACT_SERVER_PROCESS_H
