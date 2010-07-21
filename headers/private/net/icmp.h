/*
 * Copyright 2006-2010, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef ICMP_H
#define ICMP_H


// TODO: this will go private soon


// RFC 792
#define ICMP_TYPE_ECHO_REPLY		0
#define ICMP_TYPE_UNREACH			3
#define ICMP_TYPE_SOURCE_QUENCH		4
#define ICMP_TYPE_REDIRECT			5
#define ICMP_TYPE_ECHO_REQUEST		8
#define ICMP_TYPE_TIME_EXCEEDED		11
#define ICMP_TYPE_PARAM_PROBLEM		12
#define ICMP_TYPE_TIMESTAMP_REQUEST	13
#define ICMP_TYPE_TIMESTAMP_REPLY	14
#define ICMP_TYPE_INFO_REQUEST		15
#define ICMP_TYPE_INFO_REPLY		16
// RFC 950
#define ICMP_TYPE_ADDR_MASK_REQUEST	17
#define ICMP_TYPE_ADDR_MASK_REPLY	18

#define ICMP_CODE_TIMEEX_IN_TRANSIT	0
#define ICMP_CODE_TIMEEX_FRAG		1
#define ICMP_CODE_PARAM_PROBLEM		0
#define ICMP_CODE_SOURCE_QUENCH		0
#define ICMP_CODE_NET_UNREACH		0
#define ICMP_CODE_HOST_UNREACH		1
#define ICMP_CODE_PROTO_UNREACH		2
#define ICMP_CODE_PORT_UNREACH		3
#define ICMP_CODE_FRAG_NEEDED		4
#define ICMP_CODE_SOURCE_ROUTE_FAIL	5
#define ICMP_CODE_REDIRECT_NET		0
#define ICMP_CODE_REDIRECT_HOST		1
#define ICMP_CODE_REDIRECT_TOS_NET	2
#define ICMP_CODE_REDIRECT_TOS_HOST	3


static inline void
icmp_decode(uint32 errorCode, uint8 &type, uint8 &code)
{
	type = errorCode >> 8;
	code = errorCode & 0x0FF;
}


static inline uint32
icmp_encode(uint8 type, uint8 code)
{
	return ((uint32)type << 8) + code;
}


#endif	// ICMP_H
