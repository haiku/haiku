/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_REAL_TIME_DATA_H
#define _KERNEL_REAL_TIME_DATA_H


#include <SupportDefs.h>


struct real_time_data {
	uint64	boot_time;
	uint32	system_time_conversion_factor;
};

#endif	/* _KRENEL_REAL_TIME_DATA_H */
