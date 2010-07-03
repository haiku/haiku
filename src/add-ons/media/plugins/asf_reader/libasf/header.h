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

#ifndef HEADER_H
#define HEADER_H

#include "asf.h"

int asf_parse_header_validate(asf_file_t *file, asf_object_header_t *header);
void asf_free_header(asf_object_header_t *header);
asf_metadata_t *asf_header_metadata(asf_object_header_t *header);
void asf_header_free_metadata(asf_metadata_t *metadata);

#endif
