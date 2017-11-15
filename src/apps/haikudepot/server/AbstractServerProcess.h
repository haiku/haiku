/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef ABSTRACT_SERVER_PROCESS_H
#define ABSTRACT_SERVER_PROCESS_H

#include <Json.h>
#include <String.h>
#include <Url.h>

#include "StandardMetaData.h"


#define APP_ERR_NOT_MODIFIED (B_APP_ERROR_BASE + 452)


class AbstractServerProcess {
public:
	virtual	status_t			Run() = 0;

protected:
	virtual	void				GetStandardMetaDataPath(
									BPath& path) const = 0;
	virtual	void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const = 0;
	virtual const char*			LoggingName() const = 0;

			status_t			IfModifiedSinceHeaderValue(
									BString& headerValue) const;
			status_t			IfModifiedSinceHeaderValue(
									BString& headerValue,
									const BPath& metaDataPath,
									const BString& jsonPath) const;

			status_t			PopulateMetaData(
									StandardMetaData& metaData,
									const BPath& path,
									const BString& jsonPath) const;

			status_t			ParseJsonFromFileWithListener(
									BJsonEventListener *listener,
									const BPath& path) const;

			status_t			DownloadToLocalFile(
									const BPath& targetFilePath,
									const BUrl& url,
									uint32 redirects, uint32 failures);

			status_t			MoveDamagedFileAside(
									const BPath& currentFilePath);

private:
			bool				LooksLikeGzip(const char *pathStr) const;

};

#endif // ABSTRACT_SERVER_PROCESS_H
