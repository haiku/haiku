/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

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
#define MODEM_PORT_KEY		"Port"
#define MODEM_INIT_KEY			"Init"
#define MODEM_DIAL_KEY			"Dial"

extern struct core_module_info *core;


#if DEBUG
// defined in ModemDevice.cpp
extern void dump_packet(struct mbuf *packet);
#endif // DEBUG


#endif
