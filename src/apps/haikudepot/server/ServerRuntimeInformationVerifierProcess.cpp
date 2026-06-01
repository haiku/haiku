/*
 * Copyright 2026, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerRuntimeInformationVerifierProcess.h"

#include <Catalog.h>

#include "AppUtils.h"
#include "Logger.h"
#include "ServerHelper.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerRuntimeInformationVerifierProcess"


/*! The tolerance in the discrepancy between the server and client. */
static int64 kMillisTimestampTolerance = 2 * 60 * 1000;


ServerRuntimeInformationVerifierProcess::ServerRuntimeInformationVerifierProcess(Model* model)
	:
	fModel(model)
{
}


ServerRuntimeInformationVerifierProcess::~ServerRuntimeInformationVerifierProcess()
{
}


const char*
ServerRuntimeInformationVerifierProcess::Name() const
{
	return "ServerRuntimeInformationVerifierProcess";
}


const char*
ServerRuntimeInformationVerifierProcess::Description() const
{
	return B_TRANSLATE("Checking server information");
}


status_t
ServerRuntimeInformationVerifierProcess::RunInternal()
{
	status_t result = B_OK;

	if (_ShouldVerify()) {
		ServerRuntimeInformation serverRuntimeInformation;
		result = _TryFetchServerRuntimeInformation(serverRuntimeInformation);

		if (result == B_OK) {
			if (serverRuntimeInformation.CurrentTimestamp() > 0) {
				_CheckAndNotifyExcessCurrentTimestampDelta(
					serverRuntimeInformation.CurrentTimestamp());
			} else {
				HDERROR("[%s] missing server runtime information current timestamp", Name());
			}

			if (serverRuntimeInformation.AllowsNicknamePasswordAuthentication()) {
				HDDEBUG("[%s] allowing nickname password authentication", Name());
				fModel->SetCanNicknamePasswordAuthenticate(true);
			} else {
				HDINFO("[%s] disallowing nickname password authentication", Name());
				fModel->SetCanNicknamePasswordAuthenticate(false);
			}
		}
	}

	return result;
}


bool
ServerRuntimeInformationVerifierProcess::_ShouldVerify()
{
	if (!ServerHelper::IsNetworkAvailable()) {
		HDINFO("[%s] no network --> will not verify server information", Name());
		return false;
	}

	return true;
}


void
ServerRuntimeInformationVerifierProcess::_CheckAndNotifyExcessCurrentTimestampDelta(
	int64 serverCurrentTimestamp)
{
	int64 millisSinceEpoc = static_cast<int64>(real_time_clock()) * 1000;
	int64 millisDelta = llabs(millisSinceEpoc - serverCurrentTimestamp);

	// Check to make sure that there is not too much drift between the server and the
	// client.

	HDDEBUG("[%s] server current timestamp %" B_PRIi64 "ms, client current timestamp %" B_PRIi64
			"ms, delta %" B_PRIi64 "ms",
		Name(), serverCurrentTimestamp, millisSinceEpoc, millisDelta);

	if (millisDelta > kMillisTimestampTolerance) {
		HDERROR("[%s] delta is not acceptable; will warn the user", Name());

		BString message = B_TRANSLATE(
			"The server and this computer have a discrepancy "
			"in the current timestamp of around %DiscrepancySeconds% seconds. This may cause "
			"issues for HaikuDepot communicating with the server. Check your computer's clock "
			"settings and correct the current time if necessary.");

		BString discrepancySecondsString;
		discrepancySecondsString.SetToFormat("%" B_PRIi64, millisDelta),
			message.ReplaceAll("%DiscrepancySeconds%", discrepancySecondsString);

		AppUtils::NotifySimpleError(SimpleAlert(B_TRANSLATE("Timestamp discrepancy"), message));
	} else {
		HDDEBUG("[%s] delta is < %" B_PRIi64 "ms which is acceptable", Name(),
			kMillisTimestampTolerance);
	}
}


status_t
ServerRuntimeInformationVerifierProcess::_TryFetchServerRuntimeInformation(
	ServerRuntimeInformation& serverInformation)
{
	WebAppInterfaceRef interface = fModel->WebApp();
	BMessage responseEnvelopeMessage;
	BMessage responseResultMessage;
	status_t result;

	result = interface->GetRuntimeInformation(responseEnvelopeMessage);

	if (result != B_OK) {
		HDERROR("[%s] a problem has arisen retrieving the server runtime information: %s", Name(),
			strerror(result));
	}

	if (result == B_OK) {
		int32 errorCode = WebAppInterface::ErrorCodeFromResponse(responseEnvelopeMessage);
		if (errorCode != ERROR_CODE_NONE) {
			HDERROR(
				"[%s] a problem has arisen retrieving the server runtime information: jrpc error "
				"code %" B_PRId32,
				Name(), errorCode);
			result = B_ERROR;
		}
	}

	if (result == B_OK)
		result = responseEnvelopeMessage.FindMessage("result", &responseResultMessage);

	// TODO (apl) This should really be int64 but we can't deal with that right now.
	// will need to pick this up once the OpenAPI spec is consumed in the C++ side
	// and code-generated.
	double currentTimestamp;

	if (result == B_OK)
		result = responseResultMessage.FindDouble("currentTimestamp", &currentTimestamp);

	if (result == B_OK)
		serverInformation.SetCurrentTimestamp(static_cast<int64>(currentTimestamp));

	if (result == B_OK) {
		// There is a current delay in getting HDS deployed so this is current as if this value
		// might not be present in the payload.

		bool allowsNicknamePasswordAuthentication = false;
		status_t allowsNicknamePasswordAuthenticationResult = responseResultMessage.FindBool(
			"allowsNicknamePasswordAuthentication", &allowsNicknamePasswordAuthentication);

		switch (allowsNicknamePasswordAuthenticationResult) {
			case B_OK:
				if (allowsNicknamePasswordAuthentication) {
					HDDEBUG("[%s] allowing nickname + password auth", Name());
					serverInformation.SetAllowsNicknamePasswordAuthentication(true);
				} else {
					HDINFO("[%s] disallowing nickname + password auth", Name());
					serverInformation.SetAllowsNicknamePasswordAuthentication(false);
				}
				break;
			case B_NAME_NOT_FOUND:
				HDDEBUG("[%s] legacy behavior; allowing nickname + password auth", Name());
				serverInformation.SetAllowsNicknamePasswordAuthentication(true);
				break;
			default:
				result = allowsNicknamePasswordAuthenticationResult;
				break;
		}
	}

	return result;
}


// #pragma mark - server runtime information model


ServerRuntimeInformation::ServerRuntimeInformation()
	:
	fCurrentTimestamp(-1L),
	fAllowsNicknamePasswordAuthentication(true)
{
}


ServerRuntimeInformation::~ServerRuntimeInformation()
{
}


int64
ServerRuntimeInformation::CurrentTimestamp() const
{
	return fCurrentTimestamp;
}


void
ServerRuntimeInformation::SetCurrentTimestamp(int64 value)
{
	fCurrentTimestamp = value;
}


bool
ServerRuntimeInformation::AllowsNicknamePasswordAuthentication() const
{
	return fAllowsNicknamePasswordAuthentication;
}


void
ServerRuntimeInformation::SetAllowsNicknamePasswordAuthentication(bool value)
{
	fAllowsNicknamePasswordAuthentication = value;
}
