/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <UrlContext.h>


BUrlContext::BUrlContext()
	: 
	fCookieJar()
{
}


// #pragma mark Context modifiers


void
BUrlContext::SetCookieJar(const BNetworkCookieJar& cookieJar)
{
	fCookieJar = cookieJar;
}


// #pragma mark Context accessors


BNetworkCookieJar&
BUrlContext::GetCookieJar()
{
	return fCookieJar;
}
