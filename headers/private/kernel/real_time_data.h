/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef _KERNEL_REAL_TIME_DATA_H
#define _KERNEL_REAL_TIME_DATA_H

#include <StorageDefs.h>
#include <SupportDefs.h>


// ToDo: most of this is probably arch dependent. When the PPC port comes
//	to this, it should be properly separated and moved into the arch tree.

struct real_time_data {
	uint64	boot_time;
	uint32	system_time_conversion_factor;
	bigtime_t	timezone_offset;
	bool	dst_observed;
	char 	tzfilename[B_PATH_NAME_LENGTH];
	bool	isGMT;
};

#endif	/* _KERNEL_REAL_TIME_DATA_H */
