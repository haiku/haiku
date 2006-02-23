#ifndef __MOREUTF8
#define __MOREUTF8

#include <stdio.h>

static inline bool
IsInsideGlyph(uchar ch)
{
	return (ch & 0xC0) == 0x80;
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


/*	UTF8ToCharCode converts the input that includes potential multibyte chars
	to UTF-32 char codes that can be used by FreeType. The string pointer is
	then advanced to the next character in the string. In case the terminating
	0 is reached, the string pointer is not advanced anymore and spaces are
	returned. This makes it safe to overruns and enables streamed processing
	of UTF8 strings. */
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

					/* A four byte char */
					result += (*bytes)[0] & 0x07;
					result <<= 6;
					result += (*bytes)[1] & 0x3f;
					result <<= 6;
					result += (*bytes)[2] & 0x3f;
					result <<= 6;
					result += (*bytes)[3] & 0x3f;
					(*bytes) += 3;
					return result;
				}

				/* A three byte char */
				result += (*bytes)[0] & 0x0f;
				result <<= 6;
				result += (*bytes)[1] & 0x3f;
				result <<= 6;
				result += (*bytes)[2] & 0x3f;
				(*bytes) += 3;
				return result;
			}

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


/*	UTF8ToLength works like strlen() but takes UTF8 encoded multibyte chars
	into account. It's a quicker version of UTF8CountChars above. */
static inline int32
UTF8ToLength(const char *bytes)
{
	int32 length = 0;
	while (*bytes) {
		length++;

		if (bytes[0] & 0x80) {
			if (bytes[0] & 0x40) {
				if (bytes[0] & 0x20) {
					if (bytes[0] & 0x10) {
						bytes += 4;
						continue;
					}

					bytes += 3;
					continue;
				}

				bytes += 2;
				continue;
			}

			/* Not a startbyte - skip */
		}

		bytes += 1;
	}

	return length;
}


#endif
