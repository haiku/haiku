/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_ICON_EXPORT_UPDATE_PROCESS_H
#define SERVER_ICON_EXPORT_UPDATE_PROCESS_H


#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "AbstractServerProcess.h"
#include "LocalIconStore.h"
#include "Model.h"


class DumpExportPkg;


class ServerIconExportUpdateProcess : public AbstractServerProcess {
public:

								ServerIconExportUpdateProcess(
									Model* model, uint32 serverProcessOptions);
	virtual						~ServerIconExportUpdateProcess();

			const char*			Name() const;
			const char*			Description() const;

			status_t			RunInternal();

protected:
			status_t			PopulateForPkg(const PackageInfoRef& package);
			status_t			PopulateForDepot(const DepotInfo& depot);
			status_t			Populate();
			status_t			HasLocalData(bool* result) const;
			status_t			GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;
private:
			status_t			_Unpack(BPath& tarGzFilePath);
			status_t			_HandleDownloadFailure();
			status_t			_DownloadAndUnpack();
			status_t			_Download(BPath& tarGzFilePath);

			Model*				fModel;
			BPath				fLocalIconStoragePath;
			LocalIconStore*		fLocalIconStore;
			int32				fCountIconsSet;

};

#endif // SERVER_ICON_EXPORT_UPDATE_PROCESS_H
