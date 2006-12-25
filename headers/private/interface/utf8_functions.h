/*
 * Copyright 2004-2006, Haiku, Inc.
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
	and the string is read up to the terminating NULL.
*/
static inline uint32
UTF8CountBytes(const char *bytes, int32 numChars)
{
	if (!bytes)
		return 0;

	if (numChars < 0)
		numChars = INT_MAX;

	const char *base = bytes;
	while (*bytes && numChars-- > 0) {
		if (bytes[0] & 0x80) {
			if (bytes[0] & 0x40) {
				if (bytes[0] & 0x20) {
					if (bytes[0] & 0x10) {
						if (bytes[1] == 0 || bytes[2] == 0 || bytes[3] == 0)
							return (bytes - base);

						bytes += 4;
						continue;
					}

					if (bytes[1] == 0 || bytes[2] == 0)
						return (bytes - base);

					bytes += 3;
					continue;
				}

				if (bytes[1] == 0)
					return (bytes - base);

				bytes += 2;
				continue;
			}

			/* Not a startbyte - skip */
			bytes += 1;
			continue;
		}

		bytes += 1;
	}

	return (bytes - base);
}


/*!	UTF8CountChars gets the length (in characters) of a UTF8 string. Up to
	numBytes bytes are read. If numBytes is a negative value it is ignored
	and the string is read up to the terminating NULL.
*/
static inline uint32
UTF8CountChars(const char *bytes, int32 numBytes)
{
	if (!bytes)
		return 0;

	uint32 length = 0;
	const char *last = bytes + numBytes - 1;
	if (numBytes < 0)
		last = (const char *)UINT_MAX;

	while (*bytes && bytes <= last) {
		if (bytes[0] & 0x80) {
			if (bytes[0] & 0x40) {
				if (bytes[0] & 0x20) {
					if (bytes[0] & 0x10) {
						if (bytes[1] == 0 || bytes[2] == 0 || bytes[3] == 0)
							return length;

						bytes += 4;
						length++;
						continue;
					}

					if (bytes[1] == 0 || bytes[2] == 0)
						return length;

					bytes += 3;
					length++;
					continue;
				}

				if (bytes[1] == 0)
					return length;

				bytes += 2;
				length++;
				continue;
			}

			/* Not a startbyte - skip */
			bytes += 1;
			continue;
		}

		bytes += 1;
		length++;
	}

	return length;
}


/*!	UTF8ToCharCode converts the input that includes potential multibyte chars
	to UTF-32 char codes that can be used by FreeType. The string pointer is
	then advanced to the next character in the string. In case the terminating
	0 is reached, the string pointer is not advanced anymore and spaces are
	returned. This makes it safe to overruns and enables streamed processing
	of UTF8 strings.
*/
static inline uint32
UTF8ToCharCode(const char **bytes)
{
	register uint32 result = 0;

	if ((*bytes)[0] & 0x80) {
		if ((*bytes)[0] & 0x40) {
			if ((*bytes)[0] & 0x20) {
				if ((*bytes)[0] & 0x10) {
					if ((*bytes)[0] & 0x08) {
						/*	A five byte char?!
							Something's wrong, substitute. */
						result += 0x20;
						(*bytes)++;
						return result;
					}

					if ((*bytes)[1] == 0 || (*bytes)[2] == 0 || (*bytes)[3] == 0)
						return 0x00;

					/* A four byte char */
					result += (*bytes)[0] & 0x07;
					result <<= 6;
					result += (*bytes)[1] & 0x3f;
					result <<= 6;
					result += (*bytes)[2] & 0x3f;
					result <<= 6;
					result += (*bytes)[3] & 0x3f;
					(*bytes) += 4;
					return result;
				}

				if ((*bytes)[1] == 0 || (*bytes)[2] == 0)
					return 0x00;

				/* A three byte char */
				result += (*bytes)[0] & 0x0f;
				result <<= 6;
				result += (*bytes)[1] & 0x3f;
				result <<= 6;
				result += (*bytes)[2] & 0x3f;
				(*bytes) += 3;
				return result;
			}

			if ((*bytes)[1] == 0)
				return 0x00;

			/* A two byte char */
			result += (*bytes)[0] & 0x1f;
			result <<= 6;
			result += (*bytes)[1] & 0x3f;
			(*bytes) += 2;
			return result;
		}

		/*	This (10) is not a startbyte.
			Substitute with a space. */
		result += 0x20;
		(*bytes)++;
		return result;
	}

	if ((*bytes)[0] == 0) {
		/*	We do not advance beyond the terminating 0. */
		return 0x00;
	}

	result += (*bytes)[0];
	(*bytes)++;
	return result;
}

#endif	// _UTF8_FUNCTIONS_H
