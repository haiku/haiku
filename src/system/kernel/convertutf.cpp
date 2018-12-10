/*
 * Copyright 2014 Jonathan Schleifer <js@webkeks.org>
 * Copyright 2014 Haiku, Inc. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Jonathan Schleifer, js@webkeks.org
 *		John Scipione, jscipione@gmail.com
 */


#include "convertutf.h"


#include <ByteOrder.h>
#include <Errors.h>
#include <StorageDefs.h>


static inline size_t
glyph_length(uint32 glyph)
{
	if (glyph < 0x80)
		return 1;
	else if (glyph < 0x800)
		return 2;
	else if (glyph < 0x10000)
		return 3;
	else if (glyph < 0x110000)
		return 4;

	return 0;
}


static void
encode_glyph(uint32 glyph, size_t glyphLength, char* buffer)
{
	if (glyphLength == 1) {
		*buffer = glyph;
	} else if (glyphLength == 2) {
		*buffer++ = 0xC0 | (glyph >> 6);
		*buffer = 0x80 | (glyph & 0x3F);
	} else if (glyphLength == 3) {
		*buffer++ = 0xE0 | (glyph >> 12);
		*buffer++ = 0x80 | (glyph >> 6 & 0x3F);
		*buffer = 0x80 | (glyph & 0x3F);
	} else if (glyphLength == 4) {
		*buffer++ = 0xF0 | (glyph >> 18);
		*buffer++ = 0x80 | (glyph >> 12 & 0x3F);
		*buffer++ = 0x80 | (glyph >> 6 & 0x3F);
		*buffer = 0x80 | (glyph & 0x3F);
	}
}


static ssize_t
utf16_to_utf8(const uint16* source, size_t sourceCodeUnitCount, char* target,
	size_t targetLength, bool isLittleEndian)
{
	if (source == NULL || sourceCodeUnitCount == 0
		|| target == NULL || targetLength == 0) {
		return B_BAD_VALUE;
	}

	ssize_t outLength = 0;

	for (size_t i = 0; i < sourceCodeUnitCount; i++) {
		uint32 glyph = isLittleEndian
			? B_LENDIAN_TO_HOST_INT32(source[i])
			: B_BENDIAN_TO_HOST_INT32(source[i]);

		if ((glyph & 0xFC00) == 0xDC00) {
			// missing high surrogate
			return B_BAD_VALUE;
		}

		if ((glyph & 0xFC00) == 0xD800) {
			if (sourceCodeUnitCount <= i + 1) {
				// high surrogate at end of string
				return B_BAD_VALUE;
			}

			uint32 low = isLittleEndian
				? B_LENDIAN_TO_HOST_INT32(source[i + 1])
				: B_BENDIAN_TO_HOST_INT32(source[i + 1]);
			if ((low & 0xFC00) != 0xDC00) {
				// missing low surrogate
				return B_BAD_VALUE;
			}

			glyph = (((glyph & 0x3FF) << 10) | (low & 0x3FF)) + 0x10000;
			i++;
		}

		size_t glyphLength = glyph_length(glyph);
		if (glyphLength == 0)
			return B_BAD_VALUE;
		else if (outLength + glyphLength >= targetLength
			|| outLength + glyphLength >= B_FILE_NAME_LENGTH) {
			// NUL terminate the string so the caller can use the
			// abbreviated version in this case. Since the length
			// isn't returned the caller will need to call strlen()
			// to get the length of the string.
			target[outLength] = '\0';
			return B_NAME_TOO_LONG;
		}

		encode_glyph(glyph, glyphLength, target + outLength);
		outLength += glyphLength;
	}

	target[outLength] = '\0';

	return outLength;
}


ssize_t
utf16le_to_utf8(const uint16* source, size_t sourceCodeUnitCount,
	char* target, size_t targetLength)
{
	return utf16_to_utf8(source, sourceCodeUnitCount, target, targetLength,
		true);
}


ssize_t
utf16be_to_utf8(const uint16* source, size_t sourceCodeUnitCount,
	char* target, size_t targetLength)
{
	return utf16_to_utf8(source, sourceCodeUnitCount, target, targetLength,
		false);
}
