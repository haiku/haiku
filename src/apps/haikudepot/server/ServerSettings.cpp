/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ServerSettings.h"

#include <stdio.h>
#include <stdlib.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <Roster.h>
#include <Url.h>


#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"


BUrl ServerSettings::sBaseUrl = BUrl(BASEURL_DEFAULT);
BString ServerSettings::sUserAgent = BString();
pthread_once_t ServerSettings::sUserAgentInitOnce = PTHREAD_ONCE_INIT;
bool ServerSettings::sUrlConnectionTraceLogging = false;


status_t
ServerSettings::SetBaseUrl(const BUrl& value)
{
	if (!value.IsValid()) {
		fprintf(stderr, "the url is not valid\n");
		return B_BAD_VALUE;
	}

	if (value.Protocol() != "http" && value.Protocol() != "https") {
		fprintf(stderr, "the url protocol must be 'http' or 'https'\n");
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


void
ServerSettings::EnableUrlConnectionTraceLogging() {
	sUrlConnectionTraceLogging = true;
}

bool
ServerSettings::UrlConnectionTraceLoggingEnabled() {
	return sUrlConnectionTraceLogging;
}


const BString
ServerSettings::_GetUserAgentVersionString()
{
	app_info info;

	if (be_app->GetAppInfo(&info) != B_OK) {
		fprintf(stderr, "Unable to get the application info\n");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BFile file(&info.ref, B_READ_ONLY);

	if (file.InitCheck() != B_OK) {
		fprintf(stderr, "Unable to access the application info file\n");
		be_app->Quit();
		return BString(USERAGENT_FALLBACK_VERSION);
	}

	BAppFileInfo appFileInfo(&file);
	version_info versionInfo;

	if (appFileInfo.GetVersionInfo(
		&versionInfo, B_APP_VERSION_KIND) != B_OK) {
		fprintf(stderr, "Unable to establish the application version\n");
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


