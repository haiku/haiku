/*
 * Copyright 2016-2024, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 *
 * Note that this file included code earlier from `Model.cpp` and
 * copyrights have been latterly been carried across in 2024.
 */


#include "PopulatePkgChangelogFromServerProcess.h"

#include <Autolock.h>
#include <Catalog.h>

#include "Logger.h"
#include "ServerHelper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "PopulatePkgChangelogFromServerProcess"


PopulatePkgChangelogFromServerProcess::PopulatePkgChangelogFromServerProcess(
	PackageInfoRef packageInfo, Model* model)
	:
	fModel(model),
	fPackageInfo(packageInfo)
{
}


PopulatePkgChangelogFromServerProcess::~PopulatePkgChangelogFromServerProcess()
{
}


const char*
PopulatePkgChangelogFromServerProcess::Name() const
{
	return "PopulatePkgChangelogFromServerProcess";
}


const char*
PopulatePkgChangelogFromServerProcess::Description() const
{
	return B_TRANSLATE("Fetching changelog for package");
}


status_t
PopulatePkgChangelogFromServerProcess::RunInternal()
{
	// TODO; use API spec to code generation techniques instead of this manually written client.

	BMessage responsePayload;
	BString packageName = fPackageInfo->Name();
	status_t result;

	result = fModel->GetWebAppInterface()->GetChangelog(packageName, responsePayload);

	if (result == B_OK) {
		BMessage resultMessage;
		BString content;

		result = responsePayload.FindMessage("result", &resultMessage);

		if (result == B_OK) {
			if (resultMessage.FindString("content", &content) == B_OK)
				content.Trim();

			if (!content.IsEmpty()) {
				// TODO; later work to make the `PackageInfo` immutable to avoid this locking
				BAutolock locker(fModel->Lock());
				fPackageInfo->SetChangelog(content);
				HDDEBUG("changelog populated for [%s]", packageName.String());
			} else {
				HDDEBUG("no changelog present for [%s]", packageName.String());
			}
		} else {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(responsePayload);

			if (errorCode != ERROR_CODE_NONE)
				ServerHelper::NotifyServerJsonRpcError(responsePayload);
		}
	} else {
		ServerHelper::NotifyTransportError(result);
	}

	return result;
}
