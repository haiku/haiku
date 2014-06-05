/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlContext.h>

#include <stdio.h>

#include <HashMap.h>
#include <HashString.h>


BUrlContext::BUrlContext()
	:
	fCookieJar(),
	fAuthenticationMap(NULL)
{
	fAuthenticationMap = new(std::nothrow) BHttpAuthenticationMap();

	// This is the default authentication, used when nothing else is found.
	// The empty string used as a key will match all the domain strings, once
	// we have removed all components.
	fAuthenticationMap->Put(HashString("", 0), new BHttpAuthentication());
}


BUrlContext::~BUrlContext()
{
	BHttpAuthenticationMap::Iterator iterator =
		fAuthenticationMap->GetIterator();
	while (iterator.HasNext())
		delete *iterator.NextValue();

	delete fAuthenticationMap;
}


// #pragma mark Context modifiers


void
BUrlContext::SetCookieJar(const BNetworkCookieJar& cookieJar)
{
	fCookieJar = cookieJar;
}


void
BUrlContext::AddAuthentication(const BUrl& url,
	BHttpAuthentication* const authentication)
{
	BString domain = url.Host();
	domain += url.Path();
	BPrivate::HashString hostHash(domain.String(), domain.Length());

	BHttpAuthentication* previous = fAuthenticationMap->Get(hostHash);

	// Make sure we don't leak memory by overriding a previous
	// authentication for the same domain.
	if (authentication != previous) {
		fAuthenticationMap->Put(hostHash, authentication);
			// replaces the old one, or adds it in case previous == NULL
		delete previous;
	}
}


// #pragma mark Context accessors


BNetworkCookieJar&
BUrlContext::GetCookieJar()
{
	return fCookieJar;
}


BHttpAuthentication&
BUrlContext::GetAuthentication(const BUrl& url)
{
	BString domain = url.Host();
	domain += url.Path();

	BHttpAuthentication* authentication = NULL;

	do {
		authentication = fAuthenticationMap->Get( HashString(domain.String(),
			domain.Length()));

		domain.Truncate(domain.FindLast('/'));

	} while (authentication == NULL);

	return *authentication;
}
