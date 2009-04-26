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

#include <stdio.h>

#include "asf.h"

/**
 * Implement callback functions for basic file I/O operations,
 * so that users wouldn't need to reimplement them.
 */


int
asf_fileio_read_cb(FILE *stream, void *buffer, int size)
{
	int ret;

	ret = fread(buffer, 1, size, stream);
	if (!ret && !feof(stream))
		ret = -1;

	return ret;
}

int
asf_fileio_write_cb(FILE *stream, void *buffer, int size)
{
	int ret;

	ret = fwrite(buffer, 1, size, stream);
	if (!ret && !feof(stream))
		ret = -1;

	return ret;
}

int64_t
asf_fileio_seek_cb(FILE *stream, int64_t offset)
{
	int ret;

	ret = fseek(stream, offset, SEEK_SET);
	if (ret < 0)
		return -1;

	return offset;
}
