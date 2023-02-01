/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NETWORK_COOKIE_JAR_PRIVATE_H_
#define _B_NETWORK_COOKIE_JAR_PRIVATE_H_


#include <HashMap.h>

using BPrivate::Network::BNetworkCookie;
using BPrivate::Network::BNetworkCookieJar;
using BPrivate::Network::BNetworkCookieList;


typedef BPrivate::SynchronizedHashMap<HashString, BNetworkCookieList*>
	BNetworkCookieHashMap;

struct BNetworkCookieJar::PrivateHashMap : public BNetworkCookieHashMap {
};

struct BNetworkCookieJar::PrivateIterator {
								PrivateIterator(
									BNetworkCookieHashMap::Iterator it)
									:
									fCookieMapIterator(it)
								{
								}

	HashString					fKey;
	BNetworkCookieHashMap::Iterator
								fCookieMapIterator;
};

#endif // _B_NETWORK_COOKIE_JAR_PRIVATE_H_
