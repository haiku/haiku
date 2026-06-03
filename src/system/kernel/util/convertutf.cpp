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


#include <util/convertutf.h>


#include <ByteOrder.h>
#include <Errors.h>
#include <StorageDefs.h>


/*!	Returns true if passed a valid UTF-8 character continuation byte.

*/
static inline bool
is_utf8_contin_byte(uint8 byte)
{
	return byte >= 0x80 && byte <= 0xBF;
}


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


/*!	Determine length of a UTF-8 character, given the first byte of the UTF-8 encoding.
	@return If the input is not an initial UTF-8 byte, returns 1.
*/
static inline size_t
encoded_glyph_length(u_char c)
{
	if (c < 0xC0)
		return 1;
	else if (c < 0xE0)
		return 2;
	else if (c < 0xF0)
		return 3;
	else if (c < 0xF8)
		return 4;
	else if (c < 0xFC)
		return 5;
	else
		return 6;
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


/*! Convert a single UTF-8 character to UTF-16.
	@param utf8 Pointer to first byte of a UTF-8 character.
	@param glyphLength Byte length of the input character.
	@param utf16 Pointer to sufficient pre-allocated memory to hold the UTF-16 output.
	@return The bytes of utf8 actually processed; may be less than glyphLength if the input
	was found to be malformed.
	@post *utf16 is populated with the hostendian conversion output. If the input was invalid or
	unsupported, *utf16 is set to the special replacement character (Unicode point 0xFFFD). If
	utf16 points to two words of memory and the output consists of only one unit, utf16[1] may be
	left in its initial state (therefore, the client should zero it beforehand).
*/
static size_t
decode_glyph(const u_char* utf8, size_t glyphLength, uint16* utf16)
{
	size_t utf8Advance = glyphLength;

	if (is_utf8_contin_byte(utf8[0])) {
		// a UTF-8 continuation byte, where we expected an initial byte
		utf16[0] = 0xFFFD;
		return 1;
	}

	switch (glyphLength) {
		case 1:
			utf16[0] = utf8[0];
			break;
		case 2:
			if (utf8[0] <= 0xC1) {
				// overlong encoding
				utf16[0] = 0xFFFD;
			} else {
				utf16[0] = (utf8[0] & 0x1F) << 6;
				utf16[0] |= utf8[1] & 0x3F;
			}
			// verify that byte 2 is a proper continuation byte
			if (!is_utf8_contin_byte(utf8[1])) {
				utf8Advance = 1;
				utf16[0] = 0xFFFD;
			}
			break;
		case 3:
			if (utf8[0] == 0xE0 && utf8[1] < 0xA0) {
				// overlong encoding
				utf16[0] = 0xFFFD;
			} else {
				utf16[0] = (utf8[0] & 0xF) << 12;
				utf16[0] |= (utf8[1] & 0x3F) << 6;
				utf16[0] |= utf8[2] & 0x3F;
				if (utf16[0] >= 0xD800 && utf16[0] <= 0xDFFF) {
					// UTF-16 surrogate - not a valid Unicode character in itself
					utf16[0] = 0xFFFD;
				} else if (utf16[0] == 0xFFFE || utf16[0] == 0xFFFF) {
					// not a valid Unicode character
					utf16[0] = 0xFFFD;
				}
			}
			for (uint32 i = 1; i < 3; ++i) {
				if (!is_utf8_contin_byte(utf8[i])) {
					utf8Advance = i;
					utf16[0] = 0xFFFD;
					break;
				}
			}
			break;
		case 4:
		{
			uint32 unicodePoint = (utf8[0] & 0x7) << 18;
			unicodePoint |= (utf8[1] & 0x3F) << 12;
			unicodePoint |= (utf8[2] & 0x3F) << 6;
			unicodePoint |= utf8[3] & 0x3F;
			if (unicodePoint > 0x10FFFF) {
				// outside the range of Unicode and UTF-16
				utf16[0] = 0xFFFD;
			} else if (unicodePoint < 0x10000) {
				// overlong encoding
				utf16[0] = 0xFFFD;
			} else {
				uint32 codePointPrime = unicodePoint - 0x10000;
				utf16[0] = 0xD800 + ((codePointPrime & 0xFFC00) >> 10);
				utf16[1] = 0xDC00 + (codePointPrime & 0x3FF);
			}
			for (uint32 i = 1; i < 4; ++i) {
				if (!is_utf8_contin_byte(utf8[i])) {
					utf8Advance = i;
					utf16[0] = 0xFFFD;
					utf16[1] = 0;
					break;
				}
			}
		} break;
		case 5:
			// outside the range of Unicode and UTF-16
			utf16[0] = 0xFFFD;
			for (uint32 i = 1; i < 5; ++i) {
				if (!is_utf8_contin_byte(utf8[i])) {
					utf8Advance = i;
					break;
				}
			}
			break;
		case 6:
			// outside the range of Unicode and UTF-16
			utf16[0] = 0xFFFD;
			for (uint32 i = 1; i < 6; ++i) {
				if (!is_utf8_contin_byte(utf8[i])) {
					utf8Advance = i;
					break;
				}
			}
			break;
		default:
			utf16[0] = 0xFFFD;
	}

	return utf8Advance;
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


static ssize_t
utf8_to_utf16(const u_char* source, size_t* sourceLength, uint16* target, size_t targetUnits,
	uint16* surrogateOverflow, bool nullTerminate, bool toLittleEndian)
{
	status_t status = B_OK;
	const u_char* sourcePtr = source;
	uint16* targetPtr = target;

	if (surrogateOverflow != NULL)
		*surrogateOverflow = 0;

	// exit the while loop earlier if we need to leave space for a 0
	if (nullTerminate)
		--targetUnits;

	while (sourcePtr < source + *sourceLength && targetPtr < target + targetUnits) {
		size_t glyphLength = encoded_glyph_length(*sourcePtr);

		uint16 utf16[2] = {0, 0};

		if (sourcePtr + glyphLength <= source + *sourceLength) {
			glyphLength = decode_glyph(sourcePtr, glyphLength, utf16);
		} else {
			// malformed input - claimed glyph length exceeds remaining source length
			for (uint32 i = 1; i < glyphLength; ++i) {
				if (sourcePtr + i >= source + *sourceLength
					|| !is_utf8_contin_byte(*(sourcePtr + i))) {
					glyphLength = i;
					break;
				}
			}
			utf16[0] = 0xFFFD;
		}

		if (utf16[1] == 0) {
			// output is one unit
			if (toLittleEndian)
				*targetPtr++ = B_HOST_TO_LENDIAN_INT16(utf16[0]);
			else
				*targetPtr++ = B_HOST_TO_BENDIAN_INT16(utf16[0]);
		} else {
			// two units
			if (targetPtr + 1 < target + targetUnits) {
				// room for both units in target
				if (toLittleEndian) {
					*targetPtr++ = B_HOST_TO_LENDIAN_INT16(utf16[0]);
					*targetPtr++ = B_HOST_TO_LENDIAN_INT16(utf16[1]);
				} else {
					*targetPtr++ = B_HOST_TO_BENDIAN_INT16(utf16[0]);
					*targetPtr++ = B_HOST_TO_BENDIAN_INT16(utf16[1]);
				}
			} else {
				// room for only one unit in target
				if (surrogateOverflow != NULL) {
					// write high surrogate to target and low surrogate to surrogateOverflow
					if (toLittleEndian) {
						*targetPtr++ = B_HOST_TO_LENDIAN_INT16(utf16[0]);
						*surrogateOverflow = B_HOST_TO_LENDIAN_INT16(utf16[1]);
					} else {
						*targetPtr++ = B_HOST_TO_BENDIAN_INT16(utf16[0]);
						*surrogateOverflow = B_HOST_TO_BENDIAN_INT16(utf16[1]);
					}
				} else {
					// can't handle two-unit output - halt conversion with one unit still
					// unused in target
					break;
				}
			}
		}
		sourcePtr += glyphLength;
	}

	if (nullTerminate)
		*targetPtr++ = '\0';

	*sourceLength = sourcePtr - source;

	return status == B_OK ? targetPtr - target : status;
}


/*!
	@param source A UTF-8 C string.
	@param sourceLength The strlen of the source.
	@param target Preallocated space to hold UTF-16 output.
	@param targetUnits Size, in words, of the target buffer.
	@param surrogateOverflow One word of preallocated space to hold an unpaired surrogate, in the
	event that the target buffer runs out of space after writing the first unit of a two-unit
	character. If NULL, conversion will stop when the target buffer does not have space to fully
	store the next input character. This parameter is useful if the client desires to fill up the
	target buffer completely, even if it means writing a partial character.
	@param nullTerminate. Controls whether output will be null-terminated wide string or a just an
	array of wide characters.
	@return Count of UTF-16 units written to target (this *does* include the terminating NULL,
	if requested), or an error code.
	@post If no error was returned, target holds the UTF-16 conversion in little-endian byte order.
	sourceLength is set to the byte length of the converted portion of source. If overflow != NULL,
	*surrogateOverflow is either zeroed or it holds the little-endian second half of a two-unit
	character that was half-written to target. In the latter case, the count returned in
	sourceLength includes all UTF-8 bytes of the character that overflowed target.
*/
ssize_t
utf8_to_utf16le(const char* source, size_t* sourceLength, uint16* target, size_t targetUnits,
	uint16* surrogateOverflow, bool nullTerminate)
{
	return utf8_to_utf16(reinterpret_cast<const u_char*>(source), sourceLength, target, targetUnits,
		surrogateOverflow, nullTerminate, true);
}


ssize_t
utf8_to_utf16be(const char* source, size_t* sourceLength, uint16* target, size_t targetUnits,
	uint16* surrogateOverflow, bool nullTerminate)
{
	return utf8_to_utf16(reinterpret_cast<const u_char*>(source), sourceLength, target, targetUnits,
		surrogateOverflow, nullTerminate, false);
}
