/*
 * Copyright 2003-2004, Waldemar Kornewald <wkornew@gmx.net>
 * Distributed under the terms of the MIT License.
 */

#ifndef PPPoE__H
#define PPPoE__H

#include <SupportDefs.h>
#include <net/if.h>
#include <netinet/if_ether.h>

class PPPoEDevice;


#define PPPoE_QUERY_REPORT_SIZE	2048
#define PPPoE_QUERY_REPORT		'PoEQ'
	// the code value used for query reports

#define PPPoE_HEADER_SIZE		6
	// without ethernet header
#define PPPoE_TIMEOUT			3000000
	// 3 seconds
#define PPPoE_MAX_ATTEMPTS		2
	// maximum number of PPPoE's connect-retries

#define PPPoE_VERSION			0x1
#define PPPoE_TYPE				0x1

#define PPPoE_INTERFACE_KEY		"Interface"
#define PPPoE_AC_NAME_KEY		"ACName"
#define PPPoE_SERVICE_NAME_KEY	"ServiceName"

enum pppoe_ops {
	PPPoE_GET_INTERFACES,
	PPPoE_QUERY_SERVICES
};

typedef struct pppoe_query_request {
	const char *interfaceName;
	thread_id receiver;
} pppoe_query_request;

extern struct core_module_info *core;


typedef struct pppoe_header {
	uint8 version : 4;
	uint8 type : 4;
	uint8 code;
	uint16 sessionID;
	uint16 length;
	uint8 data[0];
} _PACKED pppoe_header;

typedef struct complete_pppoe_header {
	struct ether_header ethernetHeader;
	pppoe_header pppoeHeader;
} complete_pppoe_header;


// defined in pppoe.cpp
extern ifnet *FindPPPoEInterface(const char *name);
extern uint32 NewHostUniq();
extern void add_device(PPPoEDevice *device);
extern void remove_device(PPPoEDevice *device);

#if DEBUG
// defined in PPPoEDevice.cpp
extern void dump_packet(struct mbuf *packet);
#endif // DEBUG


#endif
