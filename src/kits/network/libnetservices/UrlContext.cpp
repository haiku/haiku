/*
 * Copyright 2010-2015 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Adrien Destugues, pulkomandy@pulkomandy.tk
 */


#include <UrlContext.h>

#include <stdio.h>

#include <HashMap.h>
#include <HashString.h>

using namespace BPrivate::Network;


class BUrlContext::BHttpAuthenticationMap : public
	SynchronizedHashMap<BPrivate::HashString, BHttpAuthentication*> {};


BUrlContext::BUrlContext()
	:
	fCookieJar(),
	fAuthenticationMap(NULL),
	fCertificates(20, true),
	fProxyHost(),
	fProxyPort(0)
{
	fAuthenticationMap = new(std::nothrow) BHttpAuthenticationMap();

	// This is the default authentication, used when nothing else is found.
	// The empty string used as a key will match all the domain strings, once
	// we have removed all components.
	fAuthenticationMap->Put(HashString("", 0), new BHttpAuthentication());
}


BUrlContext::~BUrlContext()
{
	BHttpAuthenticationMap::Iterator iterator
		= fAuthenticationMap->GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;

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


void
BUrlContext::SetProxy(BString host, uint16 port)
{
	fProxyHost = host;
	fProxyPort = port;
}


void
BUrlContext::AddCertificateException(const BCertificate& certificate)
{
	BCertificate* copy = new(std::nothrow) BCertificate(certificate);
	if (copy != NULL) {
		fCertificates.AddItem(copy);
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


bool
BUrlContext::UseProxy()
{
	return !fProxyHost.IsEmpty();
}


BString
BUrlContext::GetProxyHost()
{
	return fProxyHost;
}


uint16
BUrlContext::GetProxyPort()
{
	return fProxyPort;
}


bool
BUrlContext::HasCertificateException(const BCertificate& certificate)
{
	struct Equals: public UnaryPredicate<const BCertificate> {
		Equals(const BCertificate& itemToMatch)
			:
			fItemToMatch(itemToMatch)
		{
		}

		int operator()(const BCertificate* item) const
		{
			/* Must return 0 if there is a match! */
			return !(*item == fItemToMatch);
		}

		const BCertificate& fItemToMatch;
	} comparator(certificate);

	return fCertificates.FindIf(comparator) != NULL;
}
