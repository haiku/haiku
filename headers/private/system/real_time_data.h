/*
 * Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KERNEL_REAL_TIME_DATA_H
#define _KERNEL_REAL_TIME_DATA_H


#include <StorageDefs.h>
#include <SupportDefs.h>

#include <arch_real_time_data.h>


struct real_time_data {
	struct arch_real_time_data	arch_data;
};


#endif	/* _KERNEL_REAL_TIME_DATA_H */
