/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <sys/types.h>
#include <string.h>
#include <SupportDefs.h>


/* From Bit twiddling hacks:
	http://graphics.stanford.edu/~seander/bithacks.html */
#define LACKS_ZERO_BYTE(value) \
	(((value - 0x01010101) & ~value & 0x80808080) == 0)


char*
strncpy(char* dest, const char* src, size_t count)
{
	char* tmp = dest;

	// Align destination buffer for four byte writes.
	while (((addr_t)dest & 3) != 0 && count != 0) {
		count--;
		if ((*dest++ = *src++) == '\0') {
			memset(dest, '\0', count);
			return tmp;
		}
	}

	if (count == 0)
		return tmp;

	if (((addr_t)src & 3) == 0) {
		// If the source and destination are aligned, copy a word
		// word at a time
		uint32* alignedSrc = (uint32*)src;
		uint32* alignedDest = (uint32*)dest;
		size_t alignedCount = count / 4;
		count -= alignedCount * 4;

		for (; alignedCount != 0 && LACKS_ZERO_BYTE(*alignedSrc);
				alignedCount--)
			*alignedDest++ = *alignedSrc++;

		count += alignedCount * 4;
		src = (char*)alignedSrc;
		dest = (char*)alignedDest;
	}

	// Deal with the remainder.
	while (count-- != 0) {
		if ((*dest++ = *src++) == '\0') {
			memset(dest, '\0', count);
			return tmp;
		}
	}

	return tmp;
}

