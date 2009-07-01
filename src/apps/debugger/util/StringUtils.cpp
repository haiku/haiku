/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */

#include "StringUtils.h"


// from the Dragon Book: a slightly modified hashpjw()
/*static*/ uint32
StringUtils::HashValue(const char* string)
{
	if (string == NULL)
		return 0;

	uint32 h = 0;

	for (; *string; string++) {
		uint32 g = h & 0xf0000000;
		if (g)
			h ^= g >> 24;
		h = (h << 4) + *string;
	}

	return h;
}
