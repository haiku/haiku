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

class PPPoEDevice;


#define PPPoE_HEADER_SIZE		6
	// without ethernet header
#define PPPoE_TIMEOUT			3000000
	// 3 seconds
#define PPPoE_MAX_ATTEMPTS		2

#define PPPoE_VERSION			0x1
#define PPPoE_TYPE				0x1

#define PPPoE_INTERFACE_KEY		"interface"

extern struct core_module_info *core;


typedef struct pppoe_header {
	uint8 version : 4;
	uint8 type : 4;
	uint8 code;
	uint16 sessionID;
	uint16 length;
	uint8 data[0];
} pppoe_header _PACKED;

typedef struct complete_pppoe_header {
	struct ether_header ethernetHeader;
	pppoe_header pppoeHeader;
} complete_pppoe_header;


// defined in pppoe.cpp
uint32 NewHostUniq();
void add_device(PPPoEDevice *device);
void remove_device(PPPoEDevice *device);

// defined in PPPoEDevice.cpp
void dump_packet(struct mbuf *packet);


#endif
