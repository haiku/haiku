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

#ifndef COMPAT_H
#define COMPAT_H

#ifdef __GNUC__
# define INLINE __inline__
#else
# define INLINE
#endif

static INLINE uint16_t
GetWLE(const void *pointer)
{
	const uint8_t *data = pointer;
	return ((uint16_t)data[1] << 8) |
	       ((uint16_t)data[0]);
}

static INLINE uint32_t
GetDWLE(const void *pointer)
{
	const uint8_t *data = pointer;
	return ((uint32_t)data[3] << 24) |
	       ((uint32_t)data[2] << 16) |
	       ((uint32_t)data[1] <<  8) |
	       ((uint32_t)data[0]);
}

static INLINE uint64_t
GetQWLE(const void *pointer)
{
	const uint8_t *data = pointer;
	return ((uint64_t)data[7] << 56) |
	       ((uint64_t)data[6] << 48) |
	       ((uint64_t)data[5] << 40) |
	       ((uint64_t)data[4] << 32) |
	       ((uint64_t)data[3] << 24) |
	       ((uint64_t)data[2] << 16) |
	       ((uint64_t)data[1] <<  8) |
	       ((uint64_t)data[0]);
}

#endif
