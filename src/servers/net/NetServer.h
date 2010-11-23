/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef NET_SERVER_H
#define NET_SERVER_H


#include <SupportDefs.h>

#include <NetServer.h>


class BNetworkAddress;


int get_address_family(const char* argument);
bool parse_address(int32& family, const char* argument,
	BNetworkAddress& address);


#endif	// NET_SERVER_H
