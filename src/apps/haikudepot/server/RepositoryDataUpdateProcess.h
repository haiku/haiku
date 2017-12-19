/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef REPOSITORY_DATA_UPDATE_PROCESS_H
#define REPOSITORY_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include "Model.h"
#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class RepositoryDataUpdateProcess : public AbstractSingleFileServerProcess {
public:

								RepositoryDataUpdateProcess(
									AbstractServerProcessListener* listener,
									const BPath& localFilePath,
									Model* model, uint32 options);
	virtual						~RepositoryDataUpdateProcess();

			const char*				Name();

protected:
			void				GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;

			BString				UrlPathComponent();
			status_t			ProcessLocalData();
			BPath&				LocalPath();

private:
			BPath				fLocalFilePath;
			Model*				fModel;

};

#endif // REPOSITORY_DATA_UPDATE_PROCESS_H
