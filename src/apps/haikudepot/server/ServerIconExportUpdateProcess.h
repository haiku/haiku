/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SERVER_ICON_EXPORT_UPDATE_PROCESS_H
#define SERVER_ICON_EXPORT_UPDATE_PROCESS_H


#include "AbstractServerProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class ServerIconExportUpdateProcess : public AbstractServerProcess {
public:

								ServerIconExportUpdateProcess(
									const BPath& localStorageDirectoryPath);
	virtual						~ServerIconExportUpdateProcess();

			status_t			Run();

protected:
			void				GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;
			const char*			LoggingName() const;


private:
			status_t			Download(BPath& tarGzFilePath);

			BPath				fLocalStorageDirectoryPath;

};

#endif // SERVER_ICON_EXPORT_UPDATE_PROCESS_H
