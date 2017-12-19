/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef PACKAGE_DATA_UPDATE_PROCESS_H
#define PACKAGE_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include "Model.h"
#include "PackageInfo.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class PkgDataUpdateProcess : public AbstractSingleFileServerProcess {
public:
								PkgDataUpdateProcess(
									AbstractServerProcessListener* listener,
									const BPath& localFilePath,
									BString naturalLanguageCode,
									BString repositorySourceCode,
									BString depotName,
									Model *model,
									uint32 options);
	virtual						~PkgDataUpdateProcess();

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
			BString				fNaturalLanguageCode;
			BString				fRepositorySourceCode;
			Model*				fModel;
			BString				fDepotName;
			BString				fName;

};

#endif // PACKAGE_DATA_UPDATE_PROCESS_H
