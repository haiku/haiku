/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#include <strings.h>

// find first (least significant) set bit
int
ffs(int value)
{
	if (!value)
		return 0;

	// ToDo: This can certainly be optimized (e.g. by binary search). Or not
	// unlikely there's a single assembler instruction...
	for (int i = 1; i <= (int)sizeof(value) * 8; i++, value >>= 1) {
		if (value & 1)
			return i;
	}

	// never gets here
	return 0;
}
