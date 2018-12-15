/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
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


/*! This method will cause an alert to be shown to the user regarding a
    JSON-RPC error that has been sent from the application server.  It will
    send a message to the application looper which will then relay the message
    to the looper and then onto the user to see.
*/

/*static*/ void
ServerHelper::NotifyServerJsonRpcError(BMessage& error)
{
	BMessage message(MSG_SERVER_ERROR);
	message.AddMessage("error", &error);
	be_app->PostMessage(&message);
}


/*static*/ void
ServerHelper::AlertServerJsonRpcError(BMessage* message)
{
	BMessage error;
	int32 errorCode = 0;

	if (message->FindMessage("error", &error) == B_OK)
		errorCode = WebAppInterface::ErrorCodeFromResponse(error);

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
			alertText = B_TRANSLATE("The response to the captcha was incorrect.");
			break;
		case ERROR_CODE_AUTHORIZATIONFAILURE:
		case ERROR_CODE_AUTHORIZATIONRULECONFLICT:
			alertText = B_TRANSLATE("Authorization or security issue");
			break;
		default:
			alertText.SetToFormat(
				B_TRANSLATE("An unexpected error has been sent from the"
					" HaikuDepot server [%" B_PRIi32 "]"), errorCode);
			break;
	}

	BAlert* alert = new BAlert(
		B_TRANSLATE("Server Error"),
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
			errorDescription = B_TRANSLATE("Network Error");
			break;
		default:
			errorDescription.SetTo(strerror(errno));
			break;
	}

	alertText.SetToFormat(B_TRANSLATE("A network transport error has arisen"
		" communicating with the HaikuDepot server system: %s"),
		errorDescription.String());

	BAlert* alert = new BAlert(
		B_TRANSLATE("Network Transport Error"),
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
		B_TRANSLATE("This application is too old to communicate with the "
			" HaikuDepot server system.  Obtain a newer version of HaikuDepot "
			" by updating your Haiku system.  The minimum version of "
			" HaikuDepot required is \"%s\"."), minimumVersion.String());

	BAlert* alert = new BAlert(
		B_TRANSLATE("client_version_too_old"),
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