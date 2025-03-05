/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <stdint.h>
#include <string.h>


#define MISALIGNMENT(PTR, TYPE) ((addr_t)(PTR) & (sizeof(TYPE) - 1))

void*
memcpy(void* dest, const void* source, size_t count)
{
	if (count == 0 || dest == source)
		return dest;

	uint8_t* d = (uint8_t*)dest;
	const uint8_t* s = (const uint8_t*)source;

	if (MISALIGNMENT(d, size_t) == MISALIGNMENT(s, size_t)) {
		while (count > 0 && MISALIGNMENT(d, size_t) != 0) {
			*d = *s;
			d++;
			s++;
			count--;
		}

		size_t* dsz = (size_t*)d;
		const size_t* ssz = (const size_t*)s;

		while (count >= sizeof(size_t)) {
			*dsz = *ssz;
			dsz++;
			ssz++;
			count -= sizeof(size_t);
		}
		d = (uint8_t*)dsz;
		s = (const uint8_t*)ssz;
	}

	while (count > 0) {
		*d = *s;
		d++;
		s++;
		count--;
	}

	return dest;
}
