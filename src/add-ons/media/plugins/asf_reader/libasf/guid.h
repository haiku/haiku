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

#ifndef GUID_H
#define GUID_H

#include "asf.h"

struct asf_guid_s {
	uint32_t v1;
	uint32_t v2;
	uint16_t v3;
	uint8_t  v4[8];
};
typedef struct asf_guid_s asf_guid_t;

typedef enum {
	GUID_UNKNOWN,

	GUID_HEADER,
	GUID_DATA,
	GUID_INDEX,

	GUID_FILE_PROPERTIES,
	GUID_STREAM_PROPERTIES,
	GUID_CONTENT_DESCRIPTION,
	GUID_HEADER_EXTENSION,
	GUID_MARKER,
	GUID_CODEC_LIST,
	GUID_STREAM_BITRATE_PROPERTIES,
	GUID_PADDING,
	GUID_EXTENDED_CONTENT_DESCRIPTION,

	GUID_METADATA,
	GUID_LANGUAGE_LIST,
	GUID_EXTENDED_STREAM_PROPERTIES,
	GUID_ADVANCED_MUTUAL_EXCLUSION,
	GUID_STREAM_PRIORITIZATION,

	GUID_STREAM_TYPE_AUDIO,
	GUID_STREAM_TYPE_VIDEO,
	GUID_STREAM_TYPE_COMMAND,
	GUID_STREAM_TYPE_EXTENDED,
	GUID_STREAM_TYPE_EXTENDED_AUDIO
} guid_type_t;


int asf_guid_equals(const asf_guid_t *guid1, const asf_guid_t *guid2);
guid_type_t asf_guid_get_object_type(const asf_guid_t *guid);
guid_type_t asf_guid_get_stream_type(const asf_guid_t *guid);
guid_type_t asf_guid_get_type(const asf_guid_t *guid);

#endif

