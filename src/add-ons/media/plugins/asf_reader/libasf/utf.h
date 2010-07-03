/*  libasf - An Advanced Systems Format media file parser
 *  Copyright (C) 2006-2010 Juho Vähä-Herttua
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef UTF_H
#define UTF_H

#include <stdlib.h>

#include "asfint.h"

/**
 * Decode UTF-16LE text from buffer of buflen size and
 * allocate a new buffer containing the same string
 * encoded as UTF-8. Supports characters outside of BMP
 * encoded as an UTF-16 surrogate pair. Returns NULL in 
 * case of allocation failure or invalid surrogate pair.
 * Buflen is in bytes.
 */
static char *
asf_utf8_from_utf16le(uint8_t *buf, uint16_t buflen)
{
	uint32_t length, pos;
	char *ret;
	int i;

	length = 0;
	for (i=0; i<buflen/2; i++) {
		uint16_t wchar1, wchar2;

		wchar1 = buf[i*2] | (buf[i*2+1] << 8);
		if (wchar1 >= 0xD800 && wchar1 < 0xDC00) {
			i++;

			if (i*2 >= buflen) {
				/* unexpected end of buffer */
				return NULL;
			}
			wchar2 = buf[i*2] | (buf[i*2+1] << 8);
			if (wchar2 < 0xDB00 || wchar2 > 0xDFFF) {
				/* invalid surrogate pair */
				return NULL;
			}
			length += 4;
		} else if (wchar1 > 0x07FF) {
			length += 3;
		} else if (wchar1 > 0x7F) {
			length += 2;
		} else {
			length++;
		}
	}

	ret = malloc(length + 1);
	if (!ret) {
		return NULL;
	}

	pos = 0;
	for (i=0; i<buflen/2; i++) {
		uint16_t wchar1, wchar2;
		uint32_t codepoint;

		wchar1 = buf[i*2] | (buf[i*2+1] << 8);
		if (wchar1 >= 0xD800 && wchar1 < 0xDC00) {
			i++;
			wchar2 = buf[i*2] | (buf[i*2+1] << 8);
			codepoint = 0x10000;
			codepoint += ((wchar1 & 0x03FF) << 10);
			codepoint |=  (wchar2 & 0x03FF);
		} else {
			codepoint = wchar1;
		}

		if (codepoint > 0xFFFF) {
			ret[pos++] = 0xF0 | ((codepoint >> 18) & 0x07);
			ret[pos++] = 0x80 | ((codepoint >> 12) & 0x3F);
			ret[pos++] = 0x80 | ((codepoint >> 6)  & 0x3F);
			ret[pos++] = 0x80 |  (codepoint & 0x3F);
		} else if (codepoint > 0x07FF) {
			ret[pos++] = 0xE0 |  (codepoint >> 12);
			ret[pos++] = 0x80 | ((codepoint >> 6) & 0x3F);
			ret[pos++] = 0x80 |  (codepoint & 0x3F);
		} else if (codepoint > 0x7F) {
			ret[pos++] = 0xC0 |  (codepoint >> 6);
			ret[pos++] = 0x80 |  (codepoint & 0x3F);
		} else {
			ret[pos++] = codepoint;
		}
	}

	ret[length] = '\0';
	return ret;
}

#endif
