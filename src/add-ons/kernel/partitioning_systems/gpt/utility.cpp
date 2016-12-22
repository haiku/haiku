/*
 * Copyright 2007-2013, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch. All rights reserved.
 *
 * Distributed under the terms of the MIT License.
 */


#include "utility.h"

#include <string.h>

#ifndef _BOOT_MODE
#	include <utf8_functions.h>
#endif

#include "gpt_known_guids.h"


const guid_t kEmptyGUID = {0, 0, 0, {0, 0, 0, 0, 0, 0, 0, 0}};


static void
put_utf8_byte(char*& to, size_t& left, char c)
{
	if (left <= 1)
		return;

	*(to++) = c;
	left--;
}


// #pragma mark -


size_t
to_utf8(const uint16* from, size_t maxFromLength, char* to, size_t toSize)
{
	const char* start = to;
	for (uint32 i = 0; i < maxFromLength; i++) {
		// Decoding UTF-16LE
		uint32 c = 0;
		uint16 w1 = B_LENDIAN_TO_HOST_INT16(from[i]);
		if (!w1)
			break;

		bool valid = false;
		if (w1 < 0xD800 || w1 > 0xDFFF) {
			c = w1;
			valid = true;
		}

		if (!valid && (w1 >= 0xD800 && w1 <= 0xDBFF)) {
			if (i + 1 < maxFromLength) {
				uint16 w2 = B_LENDIAN_TO_HOST_INT16(from[i + 1]);
				if (w2 >= 0xDC00 && w2 <= 0xDFFF) {
					c = ((w1 & 0x3FF) << 10) | (w2 & 0x3FF);
					c += 0x10000;
					++i;
					valid = true;
				}
			}
		}

		if (!valid) break;

		if (c < 0x80)
			put_utf8_byte(to, toSize, c);
		else if (c < 0x800) {
			put_utf8_byte(to, toSize, 0xc0 | (c >> 6));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		} else if (c < 0x10000) {
			put_utf8_byte(to, toSize, 0xe0 | (c >> 12));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 6) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		} else if (c <= 0x10ffff) {
			put_utf8_byte(to, toSize, 0xf0 | (c >> 18));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 12) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | ((c >> 6) & 0x3f));
			put_utf8_byte(to, toSize, 0x80 | (c & 0x3f));
		}
	}

	if (toSize > 0)
		*to++ = '\0';

	return to - start;
}


#ifndef _BOOT_MODE
size_t
to_ucs2(const char* from, size_t fromLength, uint16* to, size_t maxToLength)
{
	size_t index = 0;
	while (from[0] != '\0' && index < maxToLength) {
		uint32 c = UTF8ToCharCode(&from);

		// Encoding UTF-16LE
		if (c > 0x10FFFF) break; // invalid
		if (c < 0x10000) {
			to[index++] = B_HOST_TO_LENDIAN_INT16(c);
		} else {
			if (index + 1 >= maxToLength) break;
			uint32 c2 = c - 0x10000;
			uint16 w1 = 0xD800, w2 = 0xDC00;
			w1 = w1 + ((c2 >> 10) & 0x3FF);
			w2 = w2 + (c2 & 0x3FF);
			to[index++] = B_HOST_TO_LENDIAN_INT16(w1);
			to[index++] = B_HOST_TO_LENDIAN_INT16(w2);
		}
	}

	if (index < maxToLength)
		to[index++] = '\0';

	return index;
}
#endif // !_BOOT_MODE


const char*
get_partition_type(const guid_t& guid)
{
	for (uint32 i = 0; i < sizeof(kTypeMap) / sizeof(kTypeMap[0]); i++) {
		if (kTypeMap[i].guid == guid)
			return kTypeMap[i].type;
	}

	return NULL;
}


#ifndef _BOOT_MODE
bool
get_guid_for_partition_type(const char* type, guid_t& guid)
{
	for (uint32 i = 0; i < sizeof(kTypeMap) / sizeof(kTypeMap[0]); i++) {
		if (strcmp(kTypeMap[i].type, type) == 0) {
			guid = kTypeMap[i].guid;
			return true;
		}
	}

	return false;
}
#endif // !_BOOT_MODE
