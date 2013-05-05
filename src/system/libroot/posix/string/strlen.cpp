/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <SupportDefs.h>


/* From Bit twiddling hacks:
	http://graphics.stanford.edu/~seander/bithacks.html */
#define LACKS_ZERO_BYTE(value) \
	(((value - 0x01010101) & ~value & 0x80808080) == 0)


size_t
strlen(const char* string)
{
	size_t length = 0;

	/* Align access for four byte reads */
	for (; (((addr_t)string + length) & 3) != 0; length++) {
		if (string[length] == '\0')
			return length;
	}

	/* Check four bytes for zero char */
	uint32* valuePointer = (uint32*)(string + length);
	for (; LACKS_ZERO_BYTE(*valuePointer); valuePointer++)
		;

	/* Find the exact length */
	for (length = ((char*)valuePointer) - string; string[length] != '\0';
		length++)
		;

	return length;
}
