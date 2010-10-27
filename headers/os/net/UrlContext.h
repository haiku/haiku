/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_CONTEXT_H_
#define _B_URL_CONTEXT_H_


#include <NetworkCookieJar.h>


class BUrlContext {
public:
								BUrlContext();

	// Context modifiers
			void				SetCookieJar(
									const BNetworkCookieJar& cookieJar);
											
	// Context accessors
			BNetworkCookieJar&	GetCookieJar();

private:
			BNetworkCookieJar	fCookieJar;
};

#endif // _B_URL_CONTEXT_H_
