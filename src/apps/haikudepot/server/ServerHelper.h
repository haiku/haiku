/*
 * Copyright 2017-2018, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include <HttpHeaders.h>
#include <Message.h>


class ServerHelper {
public:
		static bool						IsNetworkAvailable();
		static bool						IsPlatformNetworkAvailable();

		static void						NotifyClientTooOld(
											const BHttpHeaders& responseHeaders
											);
		static void						AlertClientTooOld(BMessage* message);
};

#endif // SERVER_HELPER_H
