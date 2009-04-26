/*  libasf - An Advanced Systems Format media file parser
 *  Copyright (C) 2006-2007 Juho Vähä-Herttua
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

#include <string.h>
#include "byteio.h"

uint16_t
asf_byteio_getWLE(uint8_t *data)
{
	uint16_t ret;
	int i;

	for (i=1, ret=0; i>=0; i--) {
		ret <<= 8;
		ret |= data[i];
	}

	return ret;
}

uint32_t
asf_byteio_getDWLE(uint8_t *data)
{
	uint32_t ret;
	int i;

	for (i=3, ret=0; i>=0; i--) {
		ret <<= 8;
		ret |= data[i];
	}

	return ret;
}

uint64_t
asf_byteio_getQWLE(uint8_t *data)
{
	uint64_t ret;
	int i;

	for (i=7, ret=0; i>=0; i--) {
		ret <<= 8;
		ret |= data[i];
	}

	return ret;
}

void
asf_byteio_getGUID(asf_guid_t *guid, uint8_t *data)
{
	guid->v1 = asf_byteio_getDWLE(data);
	guid->v2 = asf_byteio_getWLE(data + 4);
	guid->v3 = asf_byteio_getWLE(data + 6);
	memcpy(guid->v4, data + 8, 8);
}

void
asf_byteio_get_string(uint16_t *string, uint16_t strlen, uint8_t *data)
{
	int i;

	for (i=0; i<strlen; i++) {
		string[i] = asf_byteio_getWLE(data + i*2);
	}
}

int
asf_byteio_read(uint8_t *data, int size, asf_iostream_t *iostream)
{
	int read = 0, tmp;

	if (!iostream->read) {
		return ASF_ERROR_INTERNAL;
	}

	while ((tmp = iostream->read(iostream->opaque, data+read, size-read)) > 0) {
		read += tmp;

		if (read == size) {
			return read;
		}
	}

	return (tmp == 0) ? ASF_ERROR_EOF : ASF_ERROR_IO;
}

