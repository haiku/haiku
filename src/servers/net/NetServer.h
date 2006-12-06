/*
 * Copyright 2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef NET_SERVER_H
#define NET_SERVER_H


#include <SupportDefs.h>

#include <net/if.h>


static const uint32 kMsgConfigureInterface = 'COif';

extern bool prepare_request(ifreq& request, const char* name);
extern status_t get_mac_address(const char* device, uint8* address);

#endif	// NET_SERVER_H
