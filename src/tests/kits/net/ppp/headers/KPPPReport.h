#ifndef _K_PPP_REPORT__H
#define _K_PPP_REPORT__H

#define PPP_REPORT_DATA_LIMIT		128
	// how much optional data can be added to the report
#define PPP_REPORT_CODE				'_3PR'
	// the code field of read_port


// report flags
enum PPP_REPORT_FLAGS {
	PPP_NO_REPORT_FLAGS = 0,
	PPP_WAIT_FOR_REPLY = 0x1,
	PPP_REMOVE_AFTER_REPORT = 0x2
};

// report types
enum PPP_REPORT_TYPE {
	PPP_DESTRUCTION_REPORT = 0,
		// the interface is being destroyed (no code is needed)
	PPP_CONNECTION_REPORT = 1
};

// report codes (type-specific)
enum PPP_CONNECTION_REPORT_CODES {
	PPP_UP_SUCCESSFUL = 1,
	PPP_DOWN_SUCCESSFUL = 2,
	PPP_UP_ABORTED = 3,
	PPP_UP_FAILED = 4,
	PPP_AUTHENTICATION_FAILED = 5,
	PPP_CONNECTION_LOST = 6
};

typedef struct ppp_report_packet {
	int32 type;
	int32 code;
	uint8 len;
	char data[PPP_REPORT_DATA_LIMIT];
} ppp_report_packet;



//***********
// private
//***********
#define PPP_REPORT_TIMEOUT				10

typedef struct ppp_report_request {
	port_id port;
	int32 type;
	int32 flags;
} ppp_report_request;


#endif
