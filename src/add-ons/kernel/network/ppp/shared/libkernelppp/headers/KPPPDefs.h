/*
 * Copyright 2003-2004, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _K_PPP_DEFS__H
#define _K_PPP_DEFS__H

#include <KernelExport.h>
#include <PPPDefs.h>


// debugging macros
#define ERROR(format, args...)	dprintf(format, ## args)
#ifdef DEBUG
#define TRACE(format, args...)	dprintf(format, ## args)
#else
#define TRACE(format, args...)
#endif


extern struct core_module_info *core;
	// needed by core quick-access function defines


// various constants
#define PPP_PULSE_RATE						500000
	//!< Rate at which Pulse() is called (in microseconds).
#define PPP_RESPONSE_TEST_CODE		'_3PT'
	// private code used to test if ppp_up did not crash


//!	Module key types used when loading a module.
enum ppp_module_key_type {
	PPP_UNDEFINED_KEY_TYPE = -1,
	PPP_LOAD_MODULE_KEY_TYPE = 0,
	PPP_DEVICE_KEY_TYPE,
	PPP_PROTOCOL_KEY_TYPE,
	PPP_AUTHENTICATOR_KEY_TYPE,
	PPP_MULTILINK_KEY_TYPE
};

//!	PPP events as defined in RFC 1661 (with one exception: PPP_UP_FAILED_EVENT).
enum ppp_event {
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

//!	LCP protocol codes as defined in RFC 1661.
enum ppp_lcp_code {
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
	
	// ToDo: add LCP extensions
};

#define PPP_MIN_LCP_CODE PPP_CONFIGURE_REQUEST
#define PPP_MAX_LCP_CODE PPP_DISCARD_REQUEST


#endif
