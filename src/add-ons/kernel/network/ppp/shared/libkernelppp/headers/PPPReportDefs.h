//-----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003-2004 Waldemar Kornewald, Waldemar.Kornewald@web.de
//-----------------------------------------------------------------------

#ifndef _PPP_REPORT_DEFS__H
#define _PPP_REPORT_DEFS__H

#include <OS.h>


#define PPP_REPORT_TIMEOUT			100

#define PPP_REPORT_DATA_LIMIT		128
	// how much optional data can be added to the report
#define PPP_REPORT_CODE				'_3PR'
	// the code of receive_data() must have this value

// report flags
enum ppp_report_flags {
	PPP_WAIT_FOR_REPLY = 0x01,
	PPP_REMOVE_AFTER_REPORT = 0x02,
	PPP_NO_REPLY_TIMEOUT = 0x04,
	PPP_ALLOW_ANY_REPLY_THREAD = 0x08,
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
	PPP_REPORT_UP_ABORTED = 3,
	PPP_REPORT_DEVICE_UP_FAILED = 4,
	PPP_REPORT_LOCAL_AUTHENTICATION_REQUESTED = 5,
	PPP_REPORT_PEER_AUTHENTICATION_REQUESTED = 6,
	PPP_REPORT_LOCAL_AUTHENTICATION_SUCCESSFUL = 7,
	PPP_REPORT_PEER_AUTHENTICATION_SUCCESSFUL = 8,
	PPP_REPORT_LOCAL_AUTHENTICATION_FAILED = 9,
	PPP_REPORT_PEER_AUTHENTICATION_FAILED = 10,
	PPP_REPORT_CONNECTION_LOST = 11
};


typedef struct ppp_report_packet {
	int32 type;
	int32 code;
	uint8 length;
		// length of data
	char data[PPP_REPORT_DATA_LIMIT];
} ppp_report_packet;


typedef struct ppp_report_request {
	ppp_report_type type;
	thread_id thread;
	int32 flags;
} ppp_report_request;


#endif
