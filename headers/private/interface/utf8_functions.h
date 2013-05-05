/*
 * Copyright 2004-2010, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _UTF8_FUNCTIONS_H
#define _UTF8_FUNCTIONS_H


#include <SupportDefs.h>


static inline bool
IsInsideGlyph(uchar ch)
{
	return (ch & 0xc0) == 0x80;
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
UTF8NextCharLen(const char *bytes, size_t length)
{
	if (bytes == NULL || length == 0 || bytes[0] == 0)
		return 0;

	if ((bytes[0] & 0x80) == 0) {
		// A single ASCII char - or so...
		return 1;
	}

	if (IsInsideGlyph(bytes[0])) {
		// Not a proper multibyte start.
		return 0;
	}

	// We already know that we have the upper two bits set due to the above
	// two checks.
	uint8 mask = 0x20;
	size_t bytesExpected = 2;
	while ((bytes[0] & mask) != 0) {
		if (mask == 0x02) {
			// Seven byte char - invalid.
			return 0;
		}

		bytesExpected++;
		mask >>= 1;
	}

	// There would need to be more bytes to satisfy the char.
	if (bytesExpected > length)
		return 0;

	// We already know the first byte is fine, check the rest.
	for (size_t i = 1; i < bytesExpected; i++) {
		if (!IsInsideGlyph(bytes[i])) {
			// The sequence is incomplete.
			return 0;
		}
	}

	// Puh, everything's fine.
	return bytesExpected;
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


/*!	UTF8CountBytes gets the length (in bytes) of a UTF8 string. Up to
	numChars characters are read. If numChars is a negative value it is ignored
	and the string is read up to the terminating 0.
*/
static inline uint32
UTF8CountBytes(const char *bytes, int32 numChars)
{
	if (bytes == NULL)
		return 0;

	if (numChars < 0)
		numChars = INT_MAX;

	const char *base = bytes;
	while (bytes[0] != '\0') {
		if ((bytes[0] & 0xc0) != 0x80) {
			if (--numChars < 0)
				break;
		}
		bytes++;
	}

	return bytes - base;
}


/*!	UTF8CountChars gets the length (in characters) of a UTF8 string. Up to
	numBytes bytes are read. If numBytes is a negative value it is ignored
	and the string is read up to the terminating 0.
*/
static inline uint32
UTF8CountChars(const char *bytes, int32 numBytes)
{
	if (bytes == NULL)
		return 0;

	uint32 length = 0;
	const char *last;
	if (numBytes < 0)
		last = (const char *)SIZE_MAX;
	else
		last = bytes + numBytes - 1;

	while (bytes[0] && bytes <= last) {
		if ((bytes++[0] & 0xc0) != 0x80)
			length++;
	}

	return length;
}


/*!	UTF8ToCharCode converts the input that includes potential multibyte chars
	to UTF-32 char codes that can be used by FreeType. The string pointer is
	then advanced to the next character in the string. In case the terminating
	0 is reached, the string pointer is not advanced anymore and nulls are
	returned. This makes it safe to overruns and enables streamed processing
	of UTF8 strings.
*/
static inline uint32
UTF8ToCharCode(const char **bytes)
{
	#define UTF8_SUBSTITUTE_CHARACTER	0xfffd

	uint32 result;
	if (((*bytes)[0] & 0x80) == 0) {
		// a single byte character
		result = (*bytes)[0];
		if (result != '\0') {
			// do not advance beyond the terminating '\0'
			(*bytes)++;
		}

		return result;
	}

	if (((*bytes)[0] & 0xc0) == 0x80) {
		// not a proper multibyte start
		(*bytes)++;
		return UTF8_SUBSTITUTE_CHARACTER;
	}

	// start of a multibyte character
	uint8 mask = 0x80;
	result = (uint32)((*bytes)[0] & 0xff);
	(*bytes)++;

	while (result & mask) {
		if (mask == 0x02) {
			// seven byte char - invalid
			return UTF8_SUBSTITUTE_CHARACTER;
		}

		result &= ~mask;
		mask >>= 1;
	}

	while (((*bytes)[0] & 0xc0) == 0x80) {
		result <<= 6;
		result += (*bytes)[0] & 0x3f;
		(*bytes)++;

		mask <<= 1;
		if (mask == 0x40)
			return result;
	}

	if (mask == 0x40)
		return result;

	if ((*bytes)[0] == '\0') {
		// string terminated within multibyte char
		return 0x00;
	}

	// not enough bytes in multibyte char
	return UTF8_SUBSTITUTE_CHARACTER;

	#undef UTF8_SUBSTITUTE_CHARACTER
}

#endif	// _UTF8_FUNCTIONS_H
