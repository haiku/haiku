/*
 * Copyright 2002-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <util/StringHash.h>


uint32
hash_hash_string(const char* string)
{
	uint32 hash = 0;
	char c;

	// we assume hash to be at least 32 bits
	while ((c = *string++) != 0) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= c;
	}

	return hash;
}


uint32
hash_hash_string_part(const char* string, size_t maxLength)
{
	uint32 hash = 0;
	char c;

	// we assume hash to be at least 32 bits
	while (maxLength-- > 0 && (c = *string++) != 0) {
		hash ^= hash >> 28;
		hash <<= 4;
		hash ^= c;
	}

	return hash;
}
