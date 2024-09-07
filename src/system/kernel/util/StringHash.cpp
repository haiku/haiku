/*
 * Copyright 2002-2013, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Copyright 2001, Travis Geiselbrecht. All rights reserved.
 * Distributed under the terms of the NewOS License.
 */


#include <util/StringHash.h>


uint32
hash_hash_string(const char* _string)
{
	const uint8* string = (const uint8*)_string;

	uint32 h = 5381;
	char c;
	while ((c = *string++) != 0)
		h = (h * 33) + c;
	return h;
}


uint32
hash_hash_string_part(const char* _string, size_t maxLength)
{
	const uint8* string = (const uint8*)_string;

	uint32 h = 5381;
	char c;
	while (maxLength-- > 0 && (c = *string++) != 0)
		h = (h * 33) + c;
	return h;
}
