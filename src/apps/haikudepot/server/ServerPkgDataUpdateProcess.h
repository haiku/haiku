/*
 * Copyright 2017-2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_UPDATE_PROCESS_H
#define PACKAGE_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include <String.h>

#include "Model.h"
#include "PackageInfo.h"


class ServerPkgDataUpdateProcess : public AbstractSingleFileServerProcess {
public:
								ServerPkgDataUpdateProcess(
									BString depotName,
									Model *model,
									uint32 serverProcessOptions);
	virtual						~ServerPkgDataUpdateProcess();

			const char*			Name() const;
			const BString		Description();

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

			Model*				fModel;
			BString				fDepotName;
			BString				fName;
			BString				fDescription;

};

#endif // PACKAGE_DATA_UPDATE_PROCESS_H
