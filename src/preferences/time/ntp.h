/*
 * Copyright 2004, pinc Software. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */
#ifndef NTP_H
#define NTP_H


#include <SupportDefs.h>


extern status_t ntp_update_time(const char *host, 
	const char** errorString, int32* errorCode);


#endif	/* NTP_H */
