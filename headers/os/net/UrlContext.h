/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#ifndef _B_URL_CONTEXT_H_
#define _B_URL_CONTEXT_H_


#include <HttpAuthentication.h>
#include <NetworkCookieJar.h>
#include <Referenceable.h>


namespace BPrivate {
	template <class key, class value> class SynchronizedHashMap;
	class HashString;
}


class BUrlContext: public BReferenceable {
public:
								BUrlContext();
								~BUrlContext();

	// Context modifiers
			void				SetCookieJar(
									const BNetworkCookieJar& cookieJar);
			void				AddAuthentication(const BUrl& url,
									const BHttpAuthentication& authentication);
			void				SetProxy(BString host, uint16 port);

	// Context accessors
			BNetworkCookieJar&	GetCookieJar();
			BHttpAuthentication& GetAuthentication(const BUrl& url);
			bool				UseProxy();
			BString				GetProxyHost();
			uint16				GetProxyPort();

private:
			BNetworkCookieJar	fCookieJar;
			typedef BPrivate::SynchronizedHashMap<BPrivate::HashString,
				BHttpAuthentication*> BHttpAuthenticationMap;
			BHttpAuthenticationMap* fAuthenticationMap;

			BString				fProxyHost;
			uint16				fProxyPort;
};


#endif // _B_URL_CONTEXT_H_
