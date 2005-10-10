/*
 * Copyright 2003-2005, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */

#ifndef _PPP_REPORT_DEFS__H
#define _PPP_REPORT_DEFS__H

#include <OS.h>


#define PPP_REPORT_TIMEOUT			100

#define PPP_REPORT_DATA_LIMIT		128
	// how much optional data can be added to the report
#define PPP_REPORT_CODE				'_3PR'
	// the code of receive_data() must have this value

//!	Report flags.
enum ppp_report_flags {
	PPP_REMOVE_AFTER_REPORT = 0x01,
	PPP_REGISTER_SUBITEMS = 0x02
};

// report types
// the first 15 report types are reserved for the interface manager
#define PPP_INTERFACE_REPORT_TYPE_MIN	16
enum ppp_report_type {
	PPP_ALL_REPORTS = -1,
		// used only when disabling reports
	PPP_MANAGER_REPORT = 1,
	PPP_DESTRUCTION_REPORT = 16,
		// the interface is being destroyed (no code is needed)
		// this report is sent even if it was not requested
	PPP_CONNECTION_REPORT = 17
};


// report codes (type-specific)
enum ppp_manager_report_codes {
	// the interface id is added to the following reports
	PPP_REPORT_INTERFACE_CREATED = 0
};


enum ppp_connection_report_codes {
	// the interface id is added to the following reports
	PPP_REPORT_GOING_UP = 0,
	PPP_REPORT_UP_SUCCESSFUL = 1,
	PPP_REPORT_DOWN_SUCCESSFUL = 2,
	PPP_REPORT_DEVICE_UP_FAILED = 3,
	PPP_REPORT_AUTHENTICATION_REQUESTED = 4,
	PPP_REPORT_AUTHENTICATION_FAILED = 5,
	PPP_REPORT_CONNECTION_LOST = 6
};


//!	This is the structure of a report message.
typedef struct ppp_report_packet {
	int32 type;
	int32 code;
	uint8 length;
		//!< Length of the additional data.
	char data[PPP_REPORT_DATA_LIMIT];
} ppp_report_packet;


//!	Private structure used for storing report requests.
typedef struct ppp_report_request {
	ppp_report_type type;
	thread_id thread;
	int32 flags;
} ppp_report_request;


#endif
