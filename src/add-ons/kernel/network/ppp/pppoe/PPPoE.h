//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef PPPoE__H
#define PPPoE__H

#include <SupportDefs.h>
#include <net/if.h>
#include <netinet/if_ether.h>


#define PPPoE_HEADER_SIZE		14
	// including ethernet header
#define PPPoE_TIMEOUT			3000000
	// 3 seconds

#define PPPoE_VERSION			0x1
#define PPPoE_TYPE				0x1

#define PPPoE_INTERFACE_KEY		"interface"

extern struct core_module_info *core;


typedef struct pppoe_header {
	struct ether_header ethernetHeader;
	uint8 version : 4;
	uint8 type : 4;
	uint8 code;
	uint16 sessionID;
	uint16 length;
	uint8 data[0];
} pppoe_header _PACKED;


uint32 NewHostUniq();
	// defined in pppoe.cpp


#endif
