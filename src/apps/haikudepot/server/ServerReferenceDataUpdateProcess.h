/*
 * Copyright 2019-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_REFERENCE_DATA_UPDATE_PROCESS_H
#define SERVER_REFERENCE_DATA_UPDATE_PROCESS_H


#include "AbstractSingleFileServerProcess.h"

#include <File.h>
#include <Path.h>
#include <String.h>
#include <Url.h>

#include "Model.h"
#include "PackageInfo.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerReferenceDataUpdateProcess"

class DumpExportReference;

class ServerReferenceDataUpdateProcess : public AbstractSingleFileServerProcess
{
public:

								ServerReferenceDataUpdateProcess(
									Model* model, uint32 serverProcessOptions);
	virtual						~ServerReferenceDataUpdateProcess();

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
			status_t			_ProcessData(DumpExportReference* data);
			status_t			_ProcessNaturalLanguages(
									DumpExportReference* data);
			status_t			_ProcessPkgCategories(
									DumpExportReference* data);
			status_t			_ProcessRatingStabilities(
									DumpExportReference* data);

private:
			Model*				fModel;

};

#endif // SERVER_REFERENCE_DATA_UPDATE_PROCESS_H
