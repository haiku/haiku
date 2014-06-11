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
	const BHttpAuthentication& authentication)
{
	BString domain = url.Host();
	domain += url.Path();
	BPrivate::HashString hostHash(domain.String(), domain.Length());

	fAuthenticationMap->Lock();

	BHttpAuthentication* previous = fAuthenticationMap->Get(hostHash);

	if (previous)
		*previous = authentication;
	else {
		BHttpAuthentication* copy
			= new(std::nothrow) BHttpAuthentication(authentication);
		fAuthenticationMap->Put(hostHash, copy);
	}

	fAuthenticationMap->Unlock();
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
