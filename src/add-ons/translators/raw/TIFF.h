/*
 * Copyright 2007, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef TIFF_H
#define TIFF_H


#include <SupportDefs.h>


enum types {
	TIFF_UINT16_TYPE = 3,
	TIFF_UINT32_TYPE,
	TIFF_UFRACTION_TYPE,
	TIFF_INT16_TYPE = 8,
	TIFF_INT32_TYPE,
	TIFF_FRACTION_TYPE,
	TIFF_FLOAT_TYPE,
	TIFF_DOUBLE_TYPE,
};

struct tiff_tag {
	uint16	tag;
	uint16	type;
	uint32	length;
};

#endif	// TIFF_H
