//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef MODEM__H
#define MODEM__H

#include <SupportDefs.h>
#include <net/if.h>
#include <netinet/if_ether.h>

class ModemDevice;


#define CONTROL_ESCAPE		0x7d
#define FLAG_SEQUENCE		0x7e
#define ALL_STATIONS		0xff
#define UI					0x03

#define ESCAPE_DELAY			1000000
#define ESCAPE_SEQUENCE			"+++"
#define AT_HANG_UP				"ATH0"

#define MODEM_MTU				1502
#define PACKET_OVERHEAD			8
#define MODEM_TIMEOUT			3000000
	// 3 seconds
#define MODEM_INTERFACE_KEY		"interface"
#define MODEM_INIT_KEY			"init"
#define MODEM_DIAL_KEY			"dial"

extern struct core_module_info *core;


#if DEBUG
// defined in ModemDevice.cpp
void dump_packet(struct mbuf *packet);
#endif // DEBUG


#endif
