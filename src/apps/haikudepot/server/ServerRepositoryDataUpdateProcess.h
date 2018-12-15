/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef REPOSITORY_DATA_UPDATE_PROCESS_H
#define REPOSITORY_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "Model.h"
#include "PackageInfo.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerRepositoryDataUpdateProcess"


class ServerRepositoryDataUpdateProcess : public AbstractSingleFileServerProcess {
public:

								ServerRepositoryDataUpdateProcess(
									Model* model, uint32 serverProcessOptions);
	virtual						~ServerRepositoryDataUpdateProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
			status_t			GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;

			BString				UrlPathComponent();
			status_t			ProcessLocalData();
			status_t			GetLocalPath(BPath& path) const;

private:
			Model*				fModel;

};

#endif // REPOSITORY_DATA_UPDATE_PROCESS_H
