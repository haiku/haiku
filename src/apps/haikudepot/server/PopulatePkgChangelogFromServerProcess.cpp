/*
 * Copyright 2016-2025, Andrew Lindesay <apl@lindesay.co.nz>.
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
	const BString& packageName, Model* model)
	:
	fModel(model),
	fPackageName(packageName)
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
	status_t result;

	result = fModel->WebApp()->GetChangelog(fPackageName, responsePayload);

	if (result == B_OK) {
		BMessage resultMessage;
		BString content;

		result = responsePayload.FindMessage("result", &resultMessage);

		if (result == B_OK) {
			if (resultMessage.FindString("content", &content) == B_OK) {
				result = _UpdateChangelog(content.Trim());
				HDDEBUG("changelog populated for [%s]", fPackageName.String());
			} else {
				HDDEBUG("no changelog present for [%s]", fPackageName.String());
			}
		}
	} else {
		ServerHelper::NotifyTransportError(result);
	}

	return result;
}


status_t
PopulatePkgChangelogFromServerProcess::_UpdateChangelog(const BString& value)
{
	PackageInfoRef package = fModel->PackageForName(fPackageName);

	if (!package.IsSet()) {
		HDERROR("the package [%s] was not found", fPackageName.String());
		return B_ERROR;
	}

	PackageLocalizedTextRef localizedText
		= PackageLocalizedTextBuilder(package->LocalizedText()).WithChangelog(value).BuildRef();

	PackageInfoRef updatedPackage
		= PackageInfoBuilder(package).WithLocalizedText(localizedText).BuildRef();

	fModel->AddPackage(updatedPackage);

	return B_OK;
}
