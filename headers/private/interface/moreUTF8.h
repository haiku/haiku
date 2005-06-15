#ifndef __MOREUTF8
#define __MOREUTF8

#include <stdio.h>

static inline bool
IsInsideGlyph(uchar ch)
{
	return (ch & 0xC0) == 0x80;
//	return (ch & 0x80);
}

static inline uint32
UTF8NextCharLenUnsafe(const char *text)
{
	const char *ptr = text;

	do {
		ptr++;
	} while (IsInsideGlyph(*ptr));
				
	return ptr - text;
}

static inline uint32
UTF8NextCharLen(const char *text)
{
	if (text == NULL || *text == 0)
		return 0;

	return UTF8NextCharLenUnsafe(text);
}

static inline uint32
UTF8PreviousCharLen(const char *text, const char *limit)
{
	const char *ptr = text;
	
	if (ptr == NULL || limit == NULL)
		return 0;

	do {
		if (ptr == limit)
			break;
		ptr--;
	} while (IsInsideGlyph(*ptr));
				
	return text - ptr;
}

// TODO: use this function in other places of this file...
static inline uint32
count_utf8_bytes(uchar ch)
{
	// the number of high bits set until the first
	// unset bit determine the count of bytes used for
	// this glyph from this byte on
	uchar bit = 1 << 7;
	uint32 count = 1;
	if (ch & bit) {
		bit = bit >> 1;
		while (ch & bit) {
			count++;
			bit = bit >> 1;
		}
	}
	return count;
}

static inline uint32
UTF8CountBytes(const char *text, uint32 numChars)
{
	if (text) {
		// iterate over numChars glyphs incrementing ptr by the
		// number of bytes for each glyph, which is encoded in
		// the first byte of any glyph.
		const char *ptr = text;
		while (numChars--) {
			ptr += count_utf8_bytes(*ptr);
		}
		return ptr - text;
	}
	return 0;
}

static inline uint32
UTF8CountChars(const char *text, int32 numBytes)
{
	const char* ptr = text;
	const char* last = ptr + numBytes - 1;

	uint32 count = 0;
	while (ptr <= last) {
		ptr += UTF8NextCharLen(ptr);
		count++;
	}

	return count;
}


#endif
