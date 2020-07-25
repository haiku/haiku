/*
 * Copyright 2017-2020, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ServerSettings.h"

#include <stdlib.h>
#include <pthread.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Autolock.h>
#include <NetworkInterface.h>
#include <NetworkRoster.h>
#include <Roster.h>
#include <Url.h>

#include "Logger.h"


#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"


BUrl ServerSettings::sBaseUrl = BUrl(BASEURL_DEFAULT);
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
	app_info info;

	if (be_app->GetAppInfo(&info) != B_OK) {
		HDERROR("Unable to get the application info");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BFile file(&info.ref, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		HDERROR("Unable to access the application info file");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BAppFileInfo appFileInfo(&file);
	version_info versionInfo;

	if (appFileInfo.GetVersionInfo(
			&versionInfo, B_APP_VERSION_KIND) != B_OK) {
		HDERROR("Unable to establish the application version");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BString result;
	result.SetToFormat("%" B_PRId32 ".%" B_PRId32 ".%" B_PRId32,
		versionInfo.major, versionInfo.middle, versionInfo.minor);
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
