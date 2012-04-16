/*
 * Copyright 2010-2011, Ryan Leavengood. All Rights Reserved.
 * Copyright 2004-2009, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */

#include "ntp.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <OS.h>

#include <Catalog.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Time"


/* This structure and its data fields are described in RFC 1305
 * "Network Time Protocol (Version 3)" in appendix A.
 */

struct fixed32 {
	int16	integer;
	uint16	fraction;

	void
	SetTo(int16 integer, uint16 fraction = 0)
	{
		this->integer = htons(integer);
		this->fraction = htons(fraction);
	}

	int16 Integer() { return htons(integer); }
	uint16 Fraction() { return htons(fraction); }
};

struct ufixed64 {
	uint32	integer;
	uint32	fraction;

	void
	SetTo(uint32 integer, uint32 fraction = 0)
	{
		this->integer = htonl(integer);
		this->fraction = htonl(fraction);
	}

	uint32 Integer() { return htonl(integer); }
	uint32 Fraction() { return htonl(fraction); }
};

struct ntp_data {
	uint8		mode : 3;
	uint8		version : 3;
	uint8		leap_indicator : 2;

	uint8		stratum;
	int8		poll;
	int8		precision;	/* in seconds of the nearest power of two */

	fixed32		root_delay;
	fixed32		root_dispersion;
	uint32		root_identifier;

	ufixed64	reference_timestamp;
	ufixed64	originate_timestamp;
	ufixed64	receive_timestamp;
	ufixed64	transmit_timestamp;

	/* optional authenticator follows (96 bits) */
};

#define NTP_PORT		123
#define NTP_VERSION_3	3

enum ntp_leap_warnings {
	LEAP_NO_WARNING = 0,
	LEAP_LAST_MINUTE_61_SECONDS,
	LEAP_LAST_MINUTE_59_SECONDS,
	LEAP_CLOCK_NOT_IN_SYNC,
};

enum ntp_modes {
	MODE_RESERVED = 0,
	MODE_SYMMETRIC_ACTIVE,
	MODE_SYMMETRIC_PASSIVE,
	MODE_CLIENT,
	MODE_SERVER,
	MODE_BROADCAST,
	MODE_NTP_CONTROL_MESSAGE,
};


const uint32 kSecondsBetween1900And1970 = 2208988800UL;


uint32
seconds_since_1900(void)
{
	return kSecondsBetween1900And1970 + real_time_clock();
}


status_t
ntp_update_time(const char* hostname, const char** errorString,
	int32* errorCode)
{
	hostent	*server = gethostbyname(hostname);

	if (server == NULL) {
	
		*errorString = B_TRANSLATE("Could not contact server");
		return B_ENTRY_NOT_FOUND;
	}

	ntp_data message;
	memset(&message, 0, sizeof(ntp_data));

	message.leap_indicator = LEAP_CLOCK_NOT_IN_SYNC;
	message.version = NTP_VERSION_3;
	message.mode = MODE_CLIENT;

	message.stratum = 1;	// primary reference
	message.precision = -5;	// 2^-5 ~ 32-64 Hz precision

	message.root_delay.SetTo(1);	// 1 sec
	message.root_dispersion.SetTo(1);

	message.transmit_timestamp.SetTo(seconds_since_1900());

	int connection = socket(AF_INET, SOCK_DGRAM, 0);
	if (connection < 0) {
		*errorString = B_TRANSLATE("Could not create socket");
		*errorCode = errno;
		return B_ERROR;
	}

	struct sockaddr_in address;
	address.sin_family = AF_INET;
	address.sin_port = htons(NTP_PORT);
	address.sin_addr.s_addr = *(uint32 *)server->h_addr_list[0];

	if (sendto(connection, (char *)&message, sizeof(ntp_data),
			0, (struct sockaddr *)&address, sizeof(address)) < 0) {
		*errorString = B_TRANSLATE("Sending request failed");
		*errorCode = errno;
		return B_ERROR;
	}

	fd_set waitForReceived;
	FD_ZERO(&waitForReceived);
	FD_SET(connection, &waitForReceived);

	struct timeval timeout;
	timeout.tv_sec = 3;
	timeout.tv_usec = 0;
	// we'll wait 3 seconds for the answer

	if (select(connection + 1, &waitForReceived, NULL, NULL, &timeout) <= 0) {
		*errorString = B_TRANSLATE("Waiting for answer failed");
		*errorCode = errno;
		return B_ERROR;
	}

	message.transmit_timestamp.SetTo(0);

	socklen_t addressSize = sizeof(address);
	if (recvfrom(connection, (char *)&message, sizeof(ntp_data), 0,
			(sockaddr *)&address, &addressSize) < (ssize_t)sizeof(ntp_data)) {
		*errorString = B_TRANSLATE("Message receiving failed");
		*errorCode = errno;
		close(connection);
		return B_ERROR;
	}

	close(connection);

	if (message.transmit_timestamp.Integer() == 0) {
		*errorString = B_TRANSLATE("Received invalid time");
		return B_BAD_VALUE;
	}

	time_t now = message.transmit_timestamp.Integer() - kSecondsBetween1900And1970;
	set_real_time_clock(now);
	return B_OK;
}
