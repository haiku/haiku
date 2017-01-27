/*
 * Copyright 2017, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SERVER_SETTINGS_H
#define SERVER_SETTINGS_H

#include <File.h>
#include <HttpHeaders.h>
#include <Locker.h>
#include <String.h>


class ServerSettings {
public:
		static status_t					SetBaseUrl(const BString& baseUrl);
		static const BString			GetUserAgent();
		static void						AugmentHeaders(BHttpHeaders& headers);
		static BString					CreateFullUrl(
											const BString urlPathComponents);
		static void						EnableUrlConnectionTraceLogging();
		static bool						UrlConnectionTraceLoggingEnabled();

private:
		static const BString			_GetUserAgentVersionString();

		static BString					fBaseUrl;
		static BString					fUserAgent;
		static BLocker					fUserAgentLocker;
		static bool						fUrlConnectionTraceLogging;
};

#endif // SERVER_SETTINGS_H
