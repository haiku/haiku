//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_DEFS__H
#define _PPP_DEFS__H

#include <SupportDefs.h>

#include "net_module.h"


typedef uint32 ppp_interface_id;


// settings keys
#define PPP_DIALING_NOTIFICATION_KEY		"DialingNotification"
	// userland ppp_server handles this key
#define PPP_DISONNECT_AFTER_IDLE_SINCE_KEY	"DisonnectAfterIdleSince"
#define PPP_MODE_KEY						"Mode"
#define PPP_DIAL_RETRIES_LIMIT_KEY			"DialRetriesLimit"
#define PPP_DIAL_RETRY_DELAY_KEY			"DialRetryDelay"
#define PPP_DIAL_ON_DEMAND_KEY				"DialOnDemand"
#define PPP_AUTO_REDIAL_KEY					"AutoRedial"
#define PPP_REDIAL_DELAY_KEY				"RedialDelay"
#define PPP_LOAD_MODULE_KEY					"LoadModule"
#define PPP_PROTOCOL_KEY					"Protocol"
#define PPP_DEVICE_KEY						"Device"
#define PPP_AUTHENTICATOR_KEY				"Authenticator"
#define PPP_MULTILINK_KEY					"Multilink-Protocol"

// settings values
#define PPP_CLIENT_MODE_VALUE				"Client"
#define PPP_SERVER_MODE_VALUE				"Server"

// path defines
#define PPP_MODULES_PATH					NETWORK_MODULES_ROOT "ppp"
#define PPP_INTERFACE_SETTINGS_PATH	"/boot/home/config/settings/kernel/drivers/pppidf"
	// should be: /etc/ppp

// built-in protocols
#define PPP_LCP_PROTOCOL					0xC021


#define PPP_ERROR_BASE						B_ERRORS_END + 1


// this is used when talking to the interface manager
enum ppp_interface_filter {
	PPP_ALL_INTERFACES,
	PPP_REGISTERED_INTERFACES,
	PPP_UNREGISTERED_INTERFACES
};

// return values for Send()/Receive() methods in addition to B_ERROR and B_OK
// PPP_UNHANDLED is also used by KPPPOptionHandler
enum {
	// B_ERROR means that the packet is corrupted
	// B_OK means the packet was handled correctly
	
	// return values for KPPPProtocol
	PPP_UNHANDLED = PPP_ERROR_BASE,
		// The packet does not belong to this protocol.
		// Do not delete the packet when you return this!
	
	// return values of KPPPInterface::Receive()
	PPP_DISCARDED,
		// packet was silently discarded
	PPP_REJECTED,
		// a protocol-reject was sent
	
	PPP_NO_CONNECTION
		// could not send a packet because device is not connected
};

// PFC options
enum {
	PPP_REQUEST_PFC = 0x01,
		// try to request PFC (does not fail if not successful)
	PPP_ALLOW_PFC = 0x02,
		// allow PFC if other side requests it
	PPP_FORCE_PFC_REQUEST = 0x04,
		// if PFC request fails the connection attempt will terminate
	PPP_FREEZE_PFC_OPTIONS = 0x80
		// the options cannot be changed if this flag is set (mainly used by KPPPDevice)
};

enum ppp_pfc_state {
	PPP_PFC_DISABLED,
	PPP_PFC_ACCEPTED,
	PPP_PFC_REJECTED
		// not used for peer state
};

// protocol flags
enum {
	PPP_NO_FLAGS = 0x00,
	PPP_ALWAYS_ALLOWED = 0x01,
		// protocol may send/receive in Phase() >= PPP_ESTABLISHMENT_PHASE,
		// but only LCP is allowed in State() != PPP_OPENED_STATE!
	PPP_NOT_IMPORTANT = 0x02,
		// if this protocol fails to go up we do not disconnect
	PPP_INCLUDES_NCP = 0x04,
		// This protocol includes the corresponding NCP protocol (e.g.: IPCP + IP).
		// All protocol values will also be checked against Protocol() & 0x7FFF.
};

// phase when the protocol is brought up
enum ppp_phase {
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
enum ppp_level {
	PPP_DEVICE_LEVEL = 0,
	PPP_INTERFACE_LEVEL = 1,
		// only used by KPPPInterface
	PPP_MULTILINK_LEVEL = 2,
	PPP_ENCRYPTION_LEVEL = 5,
	PPP_COMPRESSION_LEVEL = 10,
	PPP_PROTOCOL_LEVEL = 1000
		// highest level possible; use for protocols that do not encapsulate
};

// we can be a ppp client or a ppp server interface
enum ppp_mode {
	PPP_CLIENT_MODE = 0,
	PPP_SERVER_MODE
};

// which side the protocol works for
enum ppp_side {
	PPP_NO_SIDE,
	PPP_LOCAL_SIDE,
	PPP_PEER_SIDE,
	PPP_BOTH_SIDES
};

// authentication status
enum ppp_authentication_status {
	PPP_AUTHENTICATION_FAILED = -1,
	PPP_NOT_AUTHENTICATED = 0,
	PPP_AUTHENTICATION_SUCCESSFUL = 1,
	PPP_AUTHENTICATING = 0xFF
};

// PPP states as defined in RFC 1661
enum ppp_state {
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

#endif
