/*
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the Haiku License.
*/
#ifndef KERNEL_BOOT_ADDR_RANGE_H
#define KERNEL_BOOT_ADDR_RANGE_H


#include <SupportDefs.h>


typedef struct addr_range {
	addr_t start;
	addr_t size;
} addr_range;

#ifdef __cplusplus
extern "C"
#endif
status_t insert_address_range(addr_range *ranges, uint32 *_numRanges, uint32 maxRanges,
	addr_t start, uint32 size);

#endif	/* KERNEL_BOOT_ADDR_RANGE_H */
