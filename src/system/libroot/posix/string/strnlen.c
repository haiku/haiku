/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <SupportDefs.h>


// From Bit twiddling hacks: http://graphics.stanford.edu/~seander/bithacks.html
#define hasZeroByte(value) (value - 0x01010101) & ~value & 0x80808080

size_t
strnlen(char const *s, size_t count)
{
	size_t i = 0;
	uint32 *value;

	//Make sure we use aligned access
	while (((uint32) s + i) & 3) {
		if (i == count || !s[i]) return i;
		i++;
	}


	const uint32 * end = (uint32 *) s + count;
	//Check four bytes at once
	value = (uint32 *) (s + i);
	while (value < end && !(hasZeroByte(*value)) )
		value++;

	//Find the exact length
	i = ((char *) value) - s;
	while (s[i] && i < count )
		i++;

	return i;
}
