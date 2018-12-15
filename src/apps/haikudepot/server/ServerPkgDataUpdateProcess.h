/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_UPDATE_PROCESS_H
#define PACKAGE_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "Model.h"
#include "PackageInfo.h"


class ServerPkgDataUpdateProcess : public AbstractSingleFileServerProcess {
public:
								ServerPkgDataUpdateProcess(
									BString naturalLanguageCode,
									BString depotName,
									Model *model,
									uint32 serverProcessOptions);
	virtual						~ServerPkgDataUpdateProcess();

			const char*			Name() const;
			const char*			Description() const;

protected:
	virtual status_t			RunInternal();

			status_t			GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;

			BString				UrlPathComponent();
			status_t			ProcessLocalData();
			status_t			GetLocalPath(BPath& path) const;

private:
			BString				_DeriveWebAppRepositorySourceCode() const;

			BString				fNaturalLanguageCode;
			Model*				fModel;
			BString				fDepotName;
			BString				fName;
			BString				fDescription;

};

#endif // PACKAGE_DATA_UPDATE_PROCESS_H
