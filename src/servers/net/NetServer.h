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

#include <net/if.h>


class BNetworkAddress;


// TODO: move this into a private shared header
// NOTE: this header is used by other applications (such as ifconfig,
// and Network) because of these defines
#define kNetServerSignature		"application/x-vnd.haiku-net_server"
#define kMsgConfigureInterface	'COif'
#define kMsgConfigureResolver	'COrs'
#define kMsgJoinNetwork			'JNnw'
#define kMsgLeaveNetwork		'LVnw'


int get_address_family(const char* argument);
bool parse_address(int32& family, const char* argument,
	BNetworkAddress& address);


#endif	// NET_SERVER_H
