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


// various constants
#define PPP_HANDLER_NAME_LENGTH_LIMIT		63
	// if the name is longer than this value it will be truncated to fit the structure

// starting values and other values for control ops
#define PPP_RESERVE_OPS_COUNT				0xFFFF
#define PPP_OPS_START						B_DEVICE_OP_CODES_END + 1
#define PPP_INTERFACE_OPS_START				PPP_OPS_START + PPP_RESERVE_OPS_COUNT
#define PPP_DEVICE_OPS_START				PPP_OPS_START + 2 * PPP_RESERVE_OPS_COUNT
#define PPP_PROTOCOL_OPS_START				PPP_OPS_START + 3 * PPP_RESERVE_OPS_COUNT
#define PPP_ENCAPSULATOR_OPS_START			PPP_OPS_START + 4 * PPP_RESERVE_OPS_COUNT
#define PPP_OPTION_HANDLER_OPS_START		PPP_OPS_START + 5 * PPP_RESERVE_OPS_COUNT
#define PPP_LCP_EXTENSION_OPS_START			PPP_OPS_START + 6 * PPP_RESERVE_OPS_COUNT
#define PPP_COMMON_OPS_START				PPP_OPS_START + 10 * PPP_RESERVE_OPS_COUNT
#define PPP_USER_OPS_START					PPP_OPS_START + 32 * PPP_RESERVE_OPS_COUNT


enum ppp_control_ops {
	// -----------------------------------------------------
	// PPPInterface
	PPPC_GET_INTERFACE_INFO = PPP_INTERFACE_OPS_START,
	PPPC_SET_MRU,
	PPPC_SET_DIAL_ON_DEMAND,
	PPPC_SET_AUTO_REDIAL,
	
	// these control ops use the ppp_report_request structure
	PPPC_ENABLE_INTERFACE_REPORTS,
	PPPC_DISABLE_INTERFACE_REPORTS,
		// flags are not used for this control op
	
	// handler access
	PPPC_CONTROL_DEVICE,
	PPPC_CONTROL_PROTOCOL,
	PPPC_CONTROL_ENCAPSULATOR,
	PPPC_CONTROL_OPTION_HANDLER,
	PPPC_CONTROL_LCP_EXTENSION,
	PPPC_CONTROL_CHILD,
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// PPPDevice
	PPPC_GET_DEVICE_INFO = PPP_DEVICE_OPS_START,
	// -----------------------------------------------------
	
	// -----------------------------------------------------
	// Common/mixed ops
	PPPC_GET_HANDLER_INFO = PPP_COMMON_OPS_START,
		// PPPProtocol and PPPEncapsulator
	PPPC_SET_ENABLED,
	PPPC_GET_SIMPLE_HANDLER_INFO,
		// PPPOptionHandler and PPPLCPExtension
	// -----------------------------------------------------
	
	PPP_CONTROL_OPS_END = B_DEVICE_OP_CODES_END + 0xFFFF
};


typedef struct ppp_control_info {
	uint32 index;
		// index of interface/protocol/encapsulator/etc.
	uint32 op;
		// the Control()/ioctl() opcode
	void *data;
	size_t length;
		// should always be set
} ppp_control_info;


// -----------------------------------------------------------
// structures for storing information about interface/handlers
// use the xxx_info_t structures when allocating memory (they
// reserve memory for future implementations)
// -----------------------------------------------------------
#define _PPP_INFO_T_SIZE_								256

typedef struct ppp_interface_info {
	const driver_settings *settings;
	
	ppp_mode mode;
	ppp_state state;
	ppp_phase phase;
	ppp_authentication_status localAuthenticationStatus, peerAuthenticationStatus;
	ppp_pfc_state localPFCState, peerPFCState;
	uint8 pfcOptions;
	
	uint32 protocolsCount, encapsulatorsCount, optionHandlersCount,
		LCPExtensionsCount, childrenCount;
	uint32 MRU, interfaceMTU;
	
	uint32 dialRetry, dialRetriesLimit;
	uint32 dialRetryDelay, redialDelay;
	uint32 idleSince, disconnectAfterIdleSince;
	
	bool doesDialOnDemand, doesAutoRedial, hasDevice, isMultilink, hasParent;
} ppp_interface_info;
typedef struct ppp_interface_info_t {
	ppp_interface_info info;
	uint8 _reserved_[_PPP_INFO_T_SIZE_ - sizeof(ppp_interface_info)];
} ppp_interface_info_t;


// devices are special handlers, so they have their own structure
typedef struct ppp_device_info {
	char name[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
	
	const driver_parameter *settings;
	
	uint32 MTU;
	uint32 inputTransferRate, outputTransferRate, outputBytesCount;
} ppp_device_info;
typedef struct ppp_device_info_t {
	ppp_device_info info;
	uint8 _reserved_[_PPP_INFO_T_SIZE_ - sizeof(ppp_device_info)];
} ppp_device_info_t;


typedef struct ppp_handler_info {
	char name[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
	
	// general
	const driver_parameter *settings;
	
	ppp_phase phase;
	int32 addressFamily, flags;
	ppp_side side;
	uint16 protocol;
	
	bool isEnabled;
	
	// only protocol and encapsulator
	bool isUpRequested;
	ppp_phase connectionStatus;
		// there are four possible states:
		// PPP_ESTABLISHED_PHASE	-		IsUp() == true
		// PPP_DOWN_PHASE			-		IsDown() == true
		// PPP_ESTABLISHMENT_PHASE	-		IsGoingUp() == true
		// PPP_TERMINATION_PHASE	-		IsGoingDown() == true
	
	// only protocol
	char type[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
	
	// only encapsulator
	ppp_encapsulation_level level;
	uint32 overhead;
} ppp_handler_info;
typedef struct ppp_handler_info_t {
	ppp_handler_info info;
	uint8 _reserved_[_PPP_INFO_T_SIZE_ - sizeof(ppp_handler_info)];
} ppp_handler_info_t;


typedef struct ppp_simple_handler_info {
	char name[PPP_HANDLER_NAME_LENGTH_LIMIT + 1];
	
	const driver_parameter *settings;
	bool isEnabled;
	
	uint8 code;
		// only PPPLCPExtension
} ppp_simple_handler_info;
typedef struct ppp_simple_handler_info_t {
	ppp_simple_handler_info info;
	uint8 _reserved_[_PPP_INFO_T_SIZE_ - sizeof(ppp_simple_handler_info)];
} ppp_simple_handler_info_t;


#endif
