//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_DEFS__H
#define _K_PPP_DEFS__H

#include <Errors.h>


// settings keys
#define PPP_MODE_KEY				"Mode"
#define PPP_DIAL_ON_DEMAND_KEY		"DialOnDemand"
#define PPP_AUTO_REDIAL_KEY			"AutoRedial"
#define PPP_LOAD_MODULE_KEY			"LoadModule"
#define PPP_PROTOCOL_KEY			"Protocol"
#define PPP_DEVICE_KEY				"Device"
#define PPP_AUTHENTICATOR_KEY		"Authenticator"
#define PPP_PEER_AUTHENTICATOR_KEY	"Peer-Authenticator"
#define PPP_MULTILINK_KEY			"Multilink-Protocol"

// settings values
#define PPP_CLIENT_MODE_VALUE		"Client"
#define PPP_SERVER_MODE_VALUE		"Server"

// path defines
#define PPP_MODULES_PATH			"network/ppp-modules"

// built-in protocols
#define PPP_LCP_PROTOCOL			0xC021


#define PPP_ERROR_BASE				B_ERRORS_END

// return values for Send()/Receive() methods in addition to B_ERROR and B_OK
// PPP_UNHANDLED is also used by PPPOptionHandler
enum {
	// B_ERROR means that the packet is corrupted
	// B_OK means the packet was handled correctly
	
	// return values for PPPProtocol and PPPEncapsulator (and PPPOptionHandler)
	PPP_UNHANDLED = PPP_ERROR_BASE,
		// The packet does not belong to this handler.
		// Do not delete the packet when you return this!
		// For PPPOptionHandler: the item is unrecognized
	
	// return values of PPPInterface::Receive()
	PPP_DISCARDED,
		// packet was silently discarded
	PPP_REJECTED,
		// a protocol-reject
	
	PPP_NO_CONNECTION
		// could not send a packet because device is not connected
};

// module key types (used when loading a module)
enum {
	PPP_LOAD_MODULE_TYPE,
	PPP_DEVICE_TYPE,
	PPP_PROTOCOL_TYPE,
	PPP_AUTHENTICATOR_TYPE,
	PPP_PEER_AUTHENTICATOR_TYPE,
	PPP_MULTILINK_TYPE
};

// protocol and encapsulator flags
enum {
	PPP_NO_FLAGS = 0x00,
	PPP_ALWAYS_ALLOWED = 0x01,
		// protocol may send/receive in PPP_ESTABLISHMENT_PHASE
	PPP_NEEDS_DOWN = 0x02,
		// protocol needs a Down() in addition to a Reset() to
		// terminate the connection properly (losing the connection
		// still results in a Reset() only)
	PPP_NOT_IMPORTANT = 0x03
		// if this protocol fails to go up we do not disconnect
};

// phase when the protocol is brought up
enum PPP_PHASE {
	// the following may be used by protocols
	PPP_AUTHENTICATION_PHASE = 15,
	PPP_NCP_PHASE = 20,
	PPP_ESTABLISHED_PHASE = 25,
		// only use PPP_ESTABLISHED_PHASE if
		// you want to activate this protocol after
		// the normal protocols like IP (i.e., IPCP)
	
	// the following must not be used by protocols!
	PPP_DOWN_PHASE = 0,
	PPP_TERMINATION_PHASE = 1,
		// this is the selected phase when we are GOING down
	PPP_ESTABLISHMENT_PHASE = 2
		// in this phase some protocols (with PPP_ALWAYS_ALLOWED
		// flag set) may be used
};

// this defines the order in which the packets get encapsulated
enum PPP_ENCAPSULATION_LEVEL {
	PPP_MULTILINK_LEVEL = 0,
	PPP_ENCRYPTION_LEVEL = 5,
	PPP_COMPRESSION_LEVEL = 10
};

// we can be a ppp client or a ppp server interface
enum PPP_MODE {
	PPP_CLIENT_MODE = 0,
	PPP_SERVER_MODE
};

// report types
enum PPP_REPORT {
	PPP_CONNECTION_REPORT = 0x01,
	PPP_ERROR_REPORT = 0x02,
};

// authentication status
enum PPP_AUTHENTICATION_STATUS {
	PPP_AUTHENTICATION_FAILED = -1,
	PPP_NOT_AUTHENTICATED = 0,
	PPP_AUTHENTICATION_SUCCESSFUL = 1,
	PPP_AUTHENTICATING = 0xFF
};

// PPP states as defined in RFC 1661
enum PPP_STATE {
	PPP_INITIAL_STATE,
	PPP_STARTING_STATE,
	PPP_CLOSED_STATE,
	PPP_STOPPED_STATE,
	PPP_CLOSING_STATE,
	PPP_STOPPING_STATE,
	PPP_REQ_SENT_STATE,
	PPP_ACK_RCVD_STATE,
	PPP_ACK_SENT_STATE,
	PPP_OPENED_STATE
};

// PPP actions as defined in RFC 1661
enum PPP_ACTION {
	PPP_ILLEGAL_EVENT_ACTION,
	PPP_THIS_LAYER_UP_ACTION,
	PPP_THIS_LAYER_DOWN_ACTION,
	PPP_THIS_LAYER_STARTED_ACTION,
	PPP_THIS_LAYER_FINISHED_ACTION,
	PPP_INIT_RESTART_COUNT_ACTION,
	PPP_ZERO_RESTART_COUNT_ACTION,
	PPP_SEND_CONF_REQ_ACTION,
	PPP_SEND_CONF_ACK_ACTION,
	PPP_SEND_CONF_NAK_ACTION,
	PPP_SEND_TERM_REQ_ACTION,
	PPP_SEND_TERM_ACK_ACTION,
	PPP_SEND_CODE_REJ_ACTION,
	PPP_SEND_ECHO_REPLY_ACTION
};

// PPP events as defined in RFC 1661 (with one exception: PPP_UP_FAILED_EVENT)
enum PPP_EVENT {
	PPP_UP_FAILED_EVENT,
	PPP_UP_EVENT,
	PPP_DOWN_EVENT,
	PPP_OPEN_EVENT,
	PPP_CLOSE_EVENT,
	PPP_TO_GOOD_EVENT,
	PPP_TO_BAD_EVENT,
	PPP_RCR_GOOD_EVENT,
	PPP_RCR_BAD_EVENT,
	PPP_RCA_EVENT,
	PPP_RCN_EVENT,
	PPP_RTR_EVENT,
	PPP_RTA_EVENT,
	PPP_RUC_EVENT,
	PPP_RXJ_GOOD_EVENT,
	PPP_RXJ_BAD_EVENT,
	PPP_RXR_EVENT
};

// LCP protocol types as defined in RFC 1661
// ToDo: add LCP extensions
enum PPP_LCP_TYPE {
	PPP_CONFIGURE_REQUEST = 1,
	PPP_CONFIGURE_ACK = 2,
	PPP_CONFIGURE_NAK = 3,
	PPP_CONFIGURE_REJECT = 4,
	PPP_TERMINATE_REQUEST = 5,
	PPP_TERMINATE_ACK = 6,
	PPP_CODE_REJECT = 7,
	PPP_PROTOCOL_REJECT = 8,
	PPP_ECHO_REQUEST = 9,
	PPP_ECHO_REPLY = 10,
	PPP_DISCARD_REQUEST = 11
};

#define PPP_MIN_LCP_CODE PPP_CONFIGURE_REQUEST
#define PPP_MAX_LCP_CODE PPP_DISCARD_REQUEST

#endif
