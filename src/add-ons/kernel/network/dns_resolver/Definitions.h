/*
 * Copyright 2012 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Paweł Dziepak, pdziepak@quarnos.org
 */
#ifndef DEFINITIONS_H
#define DEFINITIONS_H


const char* kPortNameReq = "dns_resolver_req";
const char* kPortNameRpl = "dns_resolver_rpl";

enum MsgCodes {
	MsgReply,
	MsgError,
	MsgGetAddrInfo,
};


#endif	//	DEFINITIONS_H

