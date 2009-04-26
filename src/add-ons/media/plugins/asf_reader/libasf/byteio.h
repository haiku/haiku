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

#ifndef BYTEIO_H
#define BYTEIO_H

#include "asf.h"
#include "guid.h"

uint16_t asf_byteio_getWLE(uint8_t *data);
uint32_t asf_byteio_getDWLE(uint8_t *data);
uint64_t asf_byteio_getQWLE(uint8_t *data);
void asf_byteio_getGUID(asf_guid_t *guid, uint8_t *data);
void asf_byteio_get_string(uint16_t *string, uint16_t strlen, uint8_t *data);

int asf_byteio_read(uint8_t *data, int size, asf_iostream_t *iostream);

#endif
