/*
 * Copyright 2021-2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#include "IncrementViewCounterProcess.h"

#include <Catalog.h>

#include "Logger.h"
#include "ServerHelper.h"
#include "WebAppInterface.h"


#define ATTEMPTS						3
#define SPIN_BETWEEN_ATTEMPTS_DELAY_MI	5 * 1000 * 1000
	// 5 seconds

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "IncrementViewCounterProcess"


IncrementViewCounterProcess::IncrementViewCounterProcess(
	Model* model, const PackageInfoRef& package)
	:
	fPackage(package),
	fModel(model)
{
	fDescription = BString(B_TRANSLATE("Recording view of \"%PackageName%\""))
		.ReplaceAll("%PackageName%", fPackage->Name());
}


IncrementViewCounterProcess::~IncrementViewCounterProcess()
{
}


const char*
IncrementViewCounterProcess::Name() const
{
	return "IncrementViewCounterProcess";
}


const char*
IncrementViewCounterProcess::Description() const
{
	return fDescription.String();
}


status_t
IncrementViewCounterProcess::RunInternal()
{
	if (!ServerHelper::IsNetworkAvailable()) {
		HDINFO("no network so will not increment view counter");
		return B_OK;
	}

	if (!fPackage.IsSet()) {
		HDERROR("the package is not present to increment the view counter");
		return B_ERROR;
	}

	DepotInfoRef depot = fModel->DepotForName(fPackage->DepotName());

	if (!depot.IsSet()) {
		HDERROR("the package's depot is not present to increment the view "
			"counter");
		return B_ERROR;
	}

	if (depot->WebAppRepositorySourceCode().IsEmpty()) {
		HDERROR("cannot increment view counter because depot has no web app "
			"repository source code");
		return B_BAD_DATA;
	}

	int32 attempts = ATTEMPTS;
	status_t result;

	while (attempts > 0 && !WasStopped()) {
		BMessage resultEnvelope;
		WebAppInterface* webAppInterface = fModel->GetWebAppInterface();
		result = webAppInterface->IncrementViewCounter(fPackage, depot, resultEnvelope);

		if (result == B_OK) {
			int32 errorCode = WebAppInterface::ErrorCodeFromResponse(resultEnvelope);
			switch (errorCode) {
				case ERROR_CODE_NONE:
					HDINFO("did increment the view counter for [%s]",
						fPackage->Name().String());
					return result;
				case ERROR_CODE_OBJECTNOTFOUND:
					HDINFO("server was not able to find the package [%s]",
						fPackage->Name().String());
					return B_NAME_NOT_FOUND;
				default:
					HDERROR("a problem has arisen incrementing the view "
					"counter for pkg [%s] w/ error code %" B_PRId32,
					fPackage->Name().String(), errorCode);
					result = B_ERROR;
					break;
			}
		} else
			HDERROR("an error has arisen incrementing the view counter");

		attempts--;
		_SpinBetweenAttempts();
	}

	return result;
}


void
IncrementViewCounterProcess::_SpinBetweenAttempts()
{
	useconds_t miniSpinDelays = SPIN_BETWEEN_ATTEMPTS_DELAY_MI / 10;
	for (int32 i = 0; i < 10 && !WasStopped(); i++)
		usleep(miniSpinDelays);
}

