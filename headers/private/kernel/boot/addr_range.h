/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/
#ifndef KERNEL_BOOT_ADDR_RANGE_H
#define KERNEL_BOOT_ADDR_RANGE_H


#include <SupportDefs.h>


typedef struct addr_range {
	addr_t start;
	addr_t size;
} addr_range;

#endif	/* KERNEL_BOOT_ADDR_RANGE_H */
