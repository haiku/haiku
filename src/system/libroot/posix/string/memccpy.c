/*
 * Copyright 2005, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT license.
 */ 


#include <SupportDefs.h>

#include <string.h>


void *
memccpy(void *_dest, const void *_source, int stopByte, size_t length)
{
	if (length) {
		const uint8 *source = (const uint8 *)_source;
		uint8 *dest = (uint8 *)_dest;

		do {
			if ((*dest++ = *source++) == (uint8)stopByte)
				return dest;
		} while (--length != 0);
	}

	return NULL;
}

