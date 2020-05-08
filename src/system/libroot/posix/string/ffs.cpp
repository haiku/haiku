/*
 * Copyright 2020, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


// find first (least significant) set bit
extern "C" int
ffs(int value)
{
	return __builtin_ffs(value);
}
