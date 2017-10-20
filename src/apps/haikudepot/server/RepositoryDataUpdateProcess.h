/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef REPOSITORY_DATA_UPDATE_PROCESS_H
#define REPOSITORY_DATA_UPDATE_PROCESS_H


#include "AbstractServerProcess.h"

#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class RepositoryDataUpdateProcess : public AbstractServerProcess {
public:

								RepositoryDataUpdateProcess(
									const BPath& localFilePath,
									DepotList* depotList);
	virtual						~RepositoryDataUpdateProcess();

			status_t			Run();

protected:
			void				GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;
			const char*			LoggingName() const;

private:
			status_t			PopulateDataToDepots();

			BPath				fLocalFilePath;
			DepotList*			fDepotList;

};

#endif // SERVER_ICON_EXPORT_UPDATE_PROCESS_H
