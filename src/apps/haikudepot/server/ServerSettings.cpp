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

#include "AutoLocker.h"


#define BASEURL_DEFAULT "https://depot.haiku-os.org"
#define USERAGENT_FALLBACK_VERSION "0.0.0"


BString ServerSettings::fBaseUrl = BString(BASEURL_DEFAULT);
BString ServerSettings::fUserAgent = BString();
BLocker ServerSettings::fUserAgentLocker;
bool ServerSettings::fUrlConnectionTraceLogging = false;


status_t
ServerSettings::SetBaseUrl(const BString& value)
{
	BUrl url(value);

	if (!url.IsValid()) {
		fprintf(stderr, "the url is not valid\n");
		return B_BAD_VALUE;
	}

	if (url.Protocol() != "http" && url.Protocol() != "https") {
		fprintf(stderr, "the url protocol must be 'http' or 'https'\n");
		return B_BAD_VALUE;
	}

	fBaseUrl.SetTo(value);

	if (fBaseUrl.EndsWith("/")) {
		fprintf(stderr, "will remove trailing '/' character in url base\n");
		fBaseUrl.Remove(fBaseUrl.Length() - 1, 1);
	}

	return B_OK;
}


BString
ServerSettings::CreateFullUrl(const BString urlPathComponents)
{
	return BString(fBaseUrl) << urlPathComponents;
}


const BString
ServerSettings::GetUserAgent()
{
	AutoLocker<BLocker> lock(&fUserAgentLocker);

	if (fUserAgent.IsEmpty()) {
		fUserAgent.SetTo("HaikuDepot/");
		fUserAgent.Append(_GetUserAgentVersionString());
	}

	return fUserAgent;
}


void
ServerSettings::EnableUrlConnectionTraceLogging() {
	fUrlConnectionTraceLogging = true;
}

bool
ServerSettings::UrlConnectionTraceLoggingEnabled() {
	return fUrlConnectionTraceLogging;
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


