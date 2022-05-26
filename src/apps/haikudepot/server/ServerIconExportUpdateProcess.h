/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_ICON_EXPORT_UPDATE_PROCESS_H
#define SERVER_ICON_EXPORT_UPDATE_PROCESS_H


#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "AbstractSingleFileServerProcess.h"
#include "Model.h"


class DumpExportPkg;


class ServerIconExportUpdateProcess : public AbstractSingleFileServerProcess {
public:

								ServerIconExportUpdateProcess(
									Model* model, uint32 serverProcessOptions);
	virtual						~ServerIconExportUpdateProcess();

			const char*			Name() const;
			const char*			Description() const;

	virtual status_t			ProcessLocalData();

	virtual	status_t			GetLocalPath(BPath& path) const;
	virtual	status_t			IfModifiedSinceHeaderValue(
									BString& headerValue) const;


	virtual	status_t			GetStandardMetaDataPath(BPath& path) const;
	virtual	void				GetStandardMetaDataJsonPath(
									BString& jsonPath) const;

protected:
	virtual	BString				UrlPathComponent();

private:
			void				_NotifyPackagesWithIconsInDepots() const;
			void				_NotifyPackagesWithIconsInDepot(
									const DepotInfoRef& depotInfo) const;

private:
			Model*				fModel;

};

#endif // SERVER_ICON_EXPORT_UPDATE_PROCESS_H
