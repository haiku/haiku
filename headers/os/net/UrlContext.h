/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_URL_CONTEXT_H_
#define _B_URL_CONTEXT_H_


#include <HttpAuthentication.h>
#include <NetworkCookieJar.h>


namespace BPrivate {
	template <class key, class value> class HashMap;
	class HashString;
}


class BUrlContext {
public:
								BUrlContext();
								~BUrlContext();

	// Context modifiers
			void				SetCookieJar(
									const BNetworkCookieJar& cookieJar);
			void				AddAuthentication(const BUrl& url,
									BHttpAuthentication* const authentication);
											
	// Context accessors
			BNetworkCookieJar&	GetCookieJar();
			BHttpAuthentication& GetAuthentication(const BUrl& url);

private:
			BNetworkCookieJar	fCookieJar;
			typedef BPrivate::HashMap<BPrivate::HashString,
				BHttpAuthentication*> BHttpAuthenticationMap;
			BHttpAuthenticationMap* fAuthenticationMap;
};

#endif // _B_URL_CONTEXT_H_
