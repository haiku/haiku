//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _PPP_CONTROL__H
#define _PPP_CONTROL__H

#include <Drivers.h>
#include <PPPDefs.h>


// starting values and other values for control ops
#define PPP_RESERVE_OPS_COUNT				0xFFFF
#define PPP_OPS_START						B_DEVICE_OP_CODES_END + 1
#define PPP_DEVICE_OPS_START				PPP_OPS_START + 2 * PPP_RESERVE_OPS_COUNT
#define PPP_PROTOCOL_OPS_START				PPP_OPS_START + 3 * PPP_RESERVE_OPS_COUNT
#define PPP_ENCAPSULATOR_OPS_START			PPP_OPS_START + 4 * PPP_RESERVE_OPS_COUNT
#define PPP_OPTION_HANDLER_OPS_START		PPP_OPS_START + 5 * PPP_RESERVE_OPS_COUNT
#define PPP_LCP_EXTENSION_OPS_START			PPP_OPS_START + 6 * PPP_RESERVE_OPS_COUNT
#define PPP_COMMON_PROTO_ENCAPS_OPS_START	PPP_OPS_START + 10 * PPP_RESERVE_OPS_COUNT
#define PPP_USER_OPS_START					PPP_OPS_START + 32 * PPP_RESERVE_OPS_COUNT


enum PPP_CONTROL_OPS {
	// -----------------------------------------------------
	// PPPInterface
	PPPC_GET_STATUS = PPP_OPS_START,
	PPPC_GET_MRU,
	PPPC_SET_MRU,
	PPPC_GET_LINK_MTU,
	PPPC_SET_LINK_MTU,
	PPPC_GET_DIAL_ON_DEMAND,
	PPPC_SET_DIAL_ON_DEMAND,
	PPPC_GET_AUTO_REDIAL,
	PPPC_SET_AUTO_REDIAL,
	PPPC_GET_PROTOCOLS_COUNT,
	PPPC_GET_ENCAPSULATORS_COUNT,
	PPPC_GET_OPTION_HANDLERS_COUNT,
	PPPC_GET_LCP_EXTENSIONS_COUNT,
	PPPC_GET_CHILDREN_COUNT,
	PPPC_CONTROL_PROTOCOL,
	PPPC_CONTROL_ENCAPSULATOR,
	PPPC_CONTROL_OPTION_HANDLER,
	PPPC_CONTROL_LCP_EXTENSION,
	PPPC_CONTROL_CHILD,
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPDevice
	PPPC_GET_MTU = PPP_DEVICE_OPS_START,
	PPPC_SET_MTU,
	PPPC_GET_PREFERRED_MTU,
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPProtocol
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPEncapsulator
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPOptionHandler
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPLCPExtension
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPProtocol and PPPEncapsulator
	PPPC_GET_NAME,
	PPPC_GET_ENABLED = PPP_COMMON_PROTO_ENCAPS_OPS_START,
	PPPC_SET_ENABLED,
	// -----------------------------------------------------
	
	PPP_CONTROL_OPS_END = B_DEVICE_OP_CODES_END + 0xFFFF
};


typedef struct ppp_control_structure {
	uint32 index;
		// index of interface/protocol/encapsulator/etc.
	uint32 op;
		// the Control()/ioctl() opcode
	
	union {
		void *data;
		ppp_control_struct *subcontrol;
	} pointer;
		// either a pointer to the data or a pointer to a control structure for
		// accessing protocols/encapsulators/etc.
	
	size_t length;
		// not always needed
} ppp_control_structure;


typedef struct ppp_status_structure {
	PPP_MODE mode;
	PPP_STATE state;
	PPP_PHASE phase;
	PPP_AUTHENTICATION_STATUS authenticationStatus, peerAuthenticationStatus;
	
	bigtime_t idle_since;
	
	uint8 _reserved_[64 - (sizeof(PPP_MODE) + sizeof(PPP_STATE) + sizeof(PPP_PHASE)
						+ 2 * sizeof(PPP_AUTHENTICATION_STATUS) + sizeof(bigtime_t))];
} ppp_status_structure;


#endif
