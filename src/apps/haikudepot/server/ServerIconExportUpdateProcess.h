/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SERVER_ICON_EXPORT_UPDATE_PROCESS_H
#define SERVER_ICON_EXPORT_UPDATE_PROCESS_H


#include "AbstractServerProcess.h"
#include "LocalIconStore.h"
#include "Model.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>


class ServerIconExportUpdateProcess :
	public AbstractServerProcess, public PackageConsumer {
public:

								ServerIconExportUpdateProcess(
									AbstractServerProcessListener* listener,
									const BPath& localStorageDirectoryPath,
									Model* model, uint32 options);
	virtual						~ServerIconExportUpdateProcess();

			const char*				Name();
			status_t			RunInternal();

	virtual	bool				ConsumePackage(
									const PackageInfoRef& packageInfoRef,
									void *context);
protected:
			status_t			PopulateForPkg(const PackageInfoRef& package);
			status_t			Populate();
			status_t			DownloadAndUnpack();
			status_t			HasLocalData(bool* result) const;
			void				GetStandardMetaDataPath(BPath& path) const;
			void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;
private:
			status_t			Download(BPath& tarGzFilePath);

			BPath				fLocalStorageDirectoryPath;
			Model*				fModel;
			LocalIconStore		fLocalIconStore;
			int32				fCountIconsSet;

};

#endif // SERVER_ICON_EXPORT_UPDATE_PROCESS_H
