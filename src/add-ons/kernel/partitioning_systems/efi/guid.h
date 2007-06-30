/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef GUID_H
#define GUID_H


#include <SupportDefs.h>


typedef struct guid {
	uint32	a;
	uint16	b;
	uint16	c;
	uint8	d[8];
} _PACKED guid_t;

#endif	/* GUID_H */

