/*
 * Copyright 2017-2021, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ServerSettings.h"

#include <stdlib.h>
#include <pthread.h>

#include <Application.h>
#include <Autolock.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Roster.h>
#include <Url.h>

#include "AppUtils.h"
#include "Logger.h"


#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"


BUrl ServerSettings::sBaseUrl = BUrl(BASEURL_DEFAULT, true);
BString ServerSettings::sUserAgent = BString();
pthread_once_t ServerSettings::sUserAgentInitOnce = PTHREAD_ONCE_INIT;
bool ServerSettings::sPreferCache = false;
bool ServerSettings::sDropCache = false;
bool ServerSettings::sForceNoNetwork = false;
bool ServerSettings::sClientTooOld = false;
BLocker ServerSettings::sLock;


status_t
ServerSettings::SetBaseUrl(const BUrl& value)
{
	if (!value.IsValid()) {
		HDERROR("the url is not valid");
		return B_BAD_VALUE;
	}

	if (value.Protocol() != "http" && value.Protocol() != "https") {
		HDERROR("the url protocol must be 'http' or 'https'");
		return B_BAD_VALUE;
	}

	sBaseUrl = value;

	return B_OK;
}


BUrl
ServerSettings::CreateFullUrl(const BString urlPathComponents)
{
	return BUrl(sBaseUrl, urlPathComponents);
}


const BString
ServerSettings::GetUserAgent()
{
	if (sUserAgent.IsEmpty())
		pthread_once(&sUserAgentInitOnce, &ServerSettings::_InitUserAgent);

	return sUserAgent;
}


void
ServerSettings::_InitUserAgent()
{
	sUserAgent.SetTo("HaikuDepot/");
	sUserAgent.Append(_GetUserAgentVersionString());
}


const BString
ServerSettings::_GetUserAgentVersionString()
{
	BString result;
	if (AppUtils::GetAppVersionString(result) != B_OK) {
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}
	return result;
}


void
ServerSettings::AugmentHeaders(BHttpHeaders& headers)
{
	headers.AddHeader("User-Agent", GetUserAgent());
}


bool
ServerSettings::PreferCache()
{
	return sPreferCache;
}


void
ServerSettings::SetPreferCache(bool value)
{
	sPreferCache = value;
}


bool
ServerSettings::DropCache()
{
	return sDropCache;
}


void
ServerSettings::SetDropCache(bool value)
{
	sDropCache = value;
}


bool
ServerSettings::ForceNoNetwork()
{
	return sForceNoNetwork;
}


void
ServerSettings::SetForceNoNetwork(bool value)
{
	sForceNoNetwork = value;
}


bool
ServerSettings::IsClientTooOld()
{
	BAutolock locker(&sLock);
	return sClientTooOld;
}


void
ServerSettings::SetClientTooOld()
{
	BAutolock locker(&sLock);
	sClientTooOld = true;
}
