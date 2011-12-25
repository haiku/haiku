/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <string.h>
#include <SupportDefs.h>


/* From Bit twiddling hacks:
	http://graphics.stanford.edu/~seander/bithacks.html */
#define HasZeroByte(value) ((value - 0x01010101) & ~value & 0x80808080)


size_t
strlen(const char* s)
{
	size_t length = 0;
	uint32* valuePointer;

	/* Align access for four byte reads */
	for (; (((addr_t) s + length) & 3) != 0; length++)
		if (s[length] == '\0')
			return length;

	/* Check four bytes for zero char */
	for (valuePointer = (uint32*) (s + length); !HasZeroByte(*valuePointer);
		valuePointer++);

	/* Find the exact length */
	for (length = ((char*) valuePointer) - s; s[length] != '\0'; length++);

	return length;
}
