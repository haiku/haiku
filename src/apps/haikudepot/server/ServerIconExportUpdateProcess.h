/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SERVER_ICON_PROCESS_H
#define SERVER_ICON_PROCESS_H


#include "AbstractServerProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "IconMetaData.h"


class ServerIconExportUpdateProcess : public AbstractServerProcess {
public:

								ServerIconExportUpdateProcess(
									const BPath& localStorageDirectoryPath);

			status_t			Run();

private:
			status_t			_Download(BPath& tarGzFilePath);
			status_t			_Download(BPath& tarGzFilePath, const BUrl& url,
									uint32 redirects, uint32 failures);
			BString				_FormFullUrl(const BString& suffix) const;
			status_t			_IfModifiedSinceHeaderValue(
									BString& headerValue) const;
			status_t			_IfModifiedSinceHeaderValue(
									BString& headerValue,
									BPath& iconMetaDataPath) const;

			status_t			_PopulateIconMetaData(
									IconMetaData& iconMetaData, BPath& path)
									const;
			status_t			_PopulateIconMetaData(
									IconMetaData& iconMetaData,
									BString& jsonString) const;
			status_t			_PopulateIconMetaData(
									IconMetaData& iconMetaData,
									BMessage& message) const;

			BString				fBaseUrl;
			BPath				fLocalStorageDirectoryPath;

};

#endif // SERVER_ICON_PROCESS_H
