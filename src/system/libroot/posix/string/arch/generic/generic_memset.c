/*
 * Copyright 2025, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT license.
 */


#include <stdint.h>
#include <string.h>


#define MISALIGNMENT(PTR, TYPE) ((addr_t)(PTR) & (sizeof(TYPE) - 1))

void*
memset(void* dest, int _value, size_t count)
{
	uint8_t* d = (uint8_t*)dest;
	uint8_t value = _value;

	if (count > sizeof(size_t)) {
		while (count > 0 && MISALIGNMENT(d, size_t) != 0) {
			*d = value;
			d++;
			count--;
		}

		const size_t valuesz = value * (SIZE_MAX / UINT8_MAX);
		size_t* dsz = (size_t*)d;

		while (count >= sizeof(size_t)) {
			*dsz = valuesz;
			dsz++;
			count -= sizeof(size_t);
		}
		d = (uint8_t*)dsz;
	}

	while (count > 0) {
		*d = value;
		d++;
		count--;
	}

	return dest;
}
