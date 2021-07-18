/*
 * Copyright 2017-2019, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef SERVER_HELPER_H
#define SERVER_HELPER_H

#include <HttpHeaders.h>

#include "ValidationFailure.h"


using BPrivate::Network::BHttpHeaders;

class BMessage;


class ServerHelper {
public:
	static	bool						IsNetworkAvailable();
	static	bool						IsPlatformNetworkAvailable();

	static	void						NotifyClientTooOld(
											const BHttpHeaders& responseHeaders
											);
	static	void						AlertClientTooOld(BMessage* message);

	static	void						NotifyTransportError(status_t error);
	static	void						AlertTransportError(BMessage* message);

	static	void						NotifyServerJsonRpcError(
											BMessage& error);
	static	void						AlertServerJsonRpcError(
											BMessage* responseEnvelopeMessage);
	static	void						GetFailuresFromJsonRpcError(
											ValidationFailures& failures,
											BMessage& responseEnvelopeMessage);
};

#endif // SERVER_HELPER_H
