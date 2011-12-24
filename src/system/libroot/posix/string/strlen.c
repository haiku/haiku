/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <SupportDefs.h>

// From Bit twiddling hacks: http://graphics.stanford.edu/~seander/bithacks.html
#define hasZeroByte(value) (value - 0x01010101) & ~value & 0x80808080

size_t
strlen(char const *s)
{
	size_t i = 0;
	uint32 *value;

	//Make sure we use aligned access
	while (((uint32) s + i) & 3) {
		if (!s[i]) return i;
		i++;
	}

	//Check four bytes at once
	value = (uint32 *) (s + i);
	while (!(hasZeroByte(*value)))
		value++;

	//Find the exact length
	i = ((char *) value) - s;
	while (s[i])
		i++;

	return i;
}
