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


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "ServerHelper"
#define KEY_MSG_MINIMUM_VERSION "minimumVersion"
#define KEY_HEADER_MINIMUM_VERSION "X-Desktop-Application-Minimum-Version"


void
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


void
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


bool
ServerHelper::IsNetworkAvailable()
{
	return !ServerSettings::ForceNoNetwork() && IsPlatformNetworkAvailable();
}


bool
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
