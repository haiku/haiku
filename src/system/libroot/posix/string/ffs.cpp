/*
 * Copyright 2020, Adrien Destugues <pulkomandy@pulkomandy.tk>.
 * Distributed under the terms of the MIT License.
 */


// find first (least significant) set bit
extern "C" int
ffs(int value)
{
#ifdef __riscv
	// TODO: As of this writing, gcc 8.x seems
	// to have an issue with infinite recursion.
	// Re-examine in future GCC updates
	int bit;
	if (value == 0)
		return 0;
	for (bit = 1; !(value & 1); bit++)
		value >>= 1;
	return bit;
#else
	return __builtin_ffs(value);
#endif
}
