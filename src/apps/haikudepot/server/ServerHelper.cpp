/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "ServerHelper.h"

#include <stdio.h>
#include <stdlib.h>

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>

#include "HaikuDepotConstants.h"
#include "ServerSettings.h"
#include "WebAppInterface.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerHelper"

#define KEY_MSG_MINIMUM_VERSION "minimumVersion"
#define KEY_HEADER_MINIMUM_VERSION "X-Desktop-Application-Minimum-Version"


/*! \brief This method will cause an alert to be shown to the user regarding a
    JSON-RPC error that has been sent from the application server.  It will
    send a message to the application looper which will then relay the message
    to the looper and then onto the user to see.
    \param responsePayload The top level payload returned from the server.
*/

/*static*/ void
ServerHelper::NotifyServerJsonRpcError(BMessage& responsePayload)
{
	BMessage message(MSG_SERVER_ERROR);
	message.AddMessage("error", &responsePayload);
	be_app->PostMessage(&message);
}


/*static*/ void
ServerHelper::AlertServerJsonRpcError(BMessage* responseEnvelopeMessage)
{
	BMessage errorMessage;
	int32 errorCode = 0;

	if (responseEnvelopeMessage->FindMessage("error", &errorMessage) == B_OK)
		errorCode = WebAppInterface::ErrorCodeFromResponse(errorMessage);

	BString alertText;

	switch (errorCode) {
		case ERROR_CODE_VALIDATION:
				// TODO; expand on that message.
			alertText = B_TRANSLATE("A validation error has occurred");
			break;
		case ERROR_CODE_OBJECTNOTFOUND:
			alertText = B_TRANSLATE("A requested object or an object involved"
				" in the request was not found on the server.");
			break;
		case ERROR_CODE_CAPTCHABADRESPONSE:
			alertText = B_TRANSLATE("The response to the captcha was"
				" incorrect.");
			break;
		case ERROR_CODE_AUTHORIZATIONFAILURE:
		case ERROR_CODE_AUTHORIZATIONRULECONFLICT:
			alertText = B_TRANSLATE("Authorization or security issue. Logout"
				" and log back in again to check that your password is correct"
				" and also check that you have agreed to the latest usage"
				" conditions.");
			break;
		default:
			alertText.SetToFormat(
				B_TRANSLATE("An unexpected error has been sent from the"
					" server [%" B_PRIi32 "]"), errorCode);
			break;
	}

	BAlert* alert = new BAlert(
		B_TRANSLATE("Server error"),
		alertText,
		B_TRANSLATE("OK"));

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


/*static*/ void
ServerHelper::NotifyTransportError(status_t error)
{
	switch (error) {
		case HD_CLIENT_TOO_OLD:
				// this is handled earlier on because it requires some
				// information from the HTTP request to create a sensible
				// error message.
			break;

		default:
		{
			BMessage message(MSG_NETWORK_TRANSPORT_ERROR);
			message.AddInt64("errno", (int64) error);
			be_app->PostMessage(&message);
			break;
		}
	}
}


/*static*/ void
ServerHelper::AlertTransportError(BMessage* message)
{
	status_t errno = B_OK;
	int64 errnoInt64;
	message->FindInt64("errno", &errnoInt64);
	errno = (status_t) errnoInt64;

	BString errorDescription("?");
	BString alertText;

	switch (errno) {
		case HD_NETWORK_INACCESSIBLE:
			errorDescription = B_TRANSLATE("Network error");
			break;
		default:
			errorDescription.SetTo(strerror(errno));
			break;
	}

	alertText.SetToFormat(B_TRANSLATE("A network transport error has arisen"
		" communicating with the server system: %s"),
		errorDescription.String());

	BAlert* alert = new BAlert(
		B_TRANSLATE("Network transport error"),
		alertText,
		B_TRANSLATE("OK"));

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


/*static*/ void
ServerHelper::NotifyClientTooOld(const BHttpHeaders& responseHeaders)
{
	if (!ServerSettings::IsClientTooOld()) {
		ServerSettings::SetClientTooOld();

		const char* minimumVersionC = responseHeaders[KEY_HEADER_MINIMUM_VERSION];
		BMessage message(MSG_CLIENT_TOO_OLD);

		if (minimumVersionC != NULL && strlen(minimumVersionC) != 0) {
			message.AddString(KEY_MSG_MINIMUM_VERSION, minimumVersionC);
		}

		be_app->PostMessage(&message);
	}
}


/*static*/ void
ServerHelper::AlertClientTooOld(BMessage* message)
{
	BString minimumVersion;
	BString alertText;

	if (message->FindString(KEY_MSG_MINIMUM_VERSION, &minimumVersion) != B_OK)
		minimumVersion = "???";

	alertText.SetToFormat(
		B_TRANSLATE("This application is too old to communicate with the"
			" server system. Obtain a newer version of this application"
			" by updating your system. The minimum required version of this"
			" application is \"%s\"."), minimumVersion.String());

	BAlert* alert = new BAlert(
		B_TRANSLATE("Client version too old"),
		alertText,
		B_TRANSLATE("OK"));

	alert->SetFlags(alert->Flags() | B_CLOSE_ON_ESCAPE);
	alert->Go();
}


/*static*/ bool
ServerHelper::IsNetworkAvailable()
{
	return !ServerSettings::ForceNoNetwork() && IsPlatformNetworkAvailable();
}


/*static*/ bool
ServerHelper::IsPlatformNetworkAvailable()
{
	BNetworkRoster& roster = BNetworkRoster::Default();
	BNetworkInterface interface;
	uint32 cookie = 0;
	while (roster.GetNextInterface(&cookie, interface) == B_OK) {
		uint32 flags = interface.Flags();
		if ((flags & IFF_LOOPBACK) == 0
			&& (flags & (IFF_UP | IFF_LINK)) == (IFF_UP | IFF_LINK)) {
			return true;
		}
	}

	return false;
}


/*! If the response is an error and the error is a validation failure then
 * various validation errors may be carried in the error data.  These are
 * copied into the supplied failures.  An abridged example input JSON structure
 * would be;
 *
 * \code
 * {
 *   ...
 *   "error": {
 *     "code": -32800,
 *     "data": {
         "validationfailures": [
           { "property": "nickname", "message": "required" },
           ...
         ]
       },
       ...
 *   }
 * }
 * \endcode
 *
 * \param failures is the object into which the validation failures are to be
 *   written.
 * \param responseEnvelopeMessage is a representation of the entire JSON-RPC
 *   response sent back from the server when the error occurred.
 *
 */

/*static*/ void
ServerHelper::GetFailuresFromJsonRpcError(
	ValidationFailures& failures, BMessage& responseEnvelopeMessage)
{
	BMessage errorMessage;
	int32 errorCode = WebAppInterface::ErrorCodeFromResponse(
		responseEnvelopeMessage);

	if (responseEnvelopeMessage.FindMessage("error", &errorMessage) == B_OK) {
		BMessage dataMessage;

		if (errorMessage.FindMessage("data", &dataMessage) == B_OK) {
			BMessage validationFailuresMessage;

			if (dataMessage.FindMessage("validationfailures",
					&validationFailuresMessage) == B_OK) {
				_GetFailuresFromJsonRpcFailures(failures,
					validationFailuresMessage);
			}
		}
	}
}


/*static*/ void
ServerHelper::_GetFailuresFromJsonRpcFailures(
	ValidationFailures& failures, BMessage& jsonRpcFailures)
{
	int32 index = 0;
	while (true) {
		BString name;
		name << index++;
		BMessage failure;
		if (jsonRpcFailures.FindMessage(name, &failure) != B_OK)
			break;

		BString property;
		BString message;
		if (failure.FindString("property", &property) == B_OK
				&& failure.FindString("message", &message) == B_OK) {
			failures.AddFailure(property, message);
		}
	}
}