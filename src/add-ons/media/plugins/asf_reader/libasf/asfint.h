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

#ifndef ASFINT_H
#define ASFINT_H

#include "asf.h"
#include "guid.h"

/* DO NOT MODIFY THE FIRST 3 VARIABLES, BECAUSE THEY ARE
 * ALSO DEFINED IN asf.h HEADER AND WILL BREAK THINGS */
#define ASF_OBJECT_COMMON        \
	asf_guid_t   guid;       \
	uint64_t     size;       \
	uint8_t      *full_data; \
	uint64_t     datalen;    \
	uint8_t      *data;      \
	guid_type_t  type;       \
	struct asfint_object_s *next; 

struct asfint_object_s {
	ASF_OBJECT_COMMON
};
typedef struct asfint_object_s asfint_object_t;

struct asf_object_headerext_s {
	ASF_OBJECT_COMMON
	asf_guid_t   reserved1;
	uint16_t     reserved2;
	struct asfint_object_s *first;
	struct asfint_object_s *last;
};
typedef struct asf_object_headerext_s asf_object_headerext_t;

struct asf_object_header_s {
	ASF_OBJECT_COMMON
	uint16_t     subobjects;
	uint8_t      reserved1; /* 0x01, but could be safely ignored */
	uint8_t      reserved2; /* 0x02, if not must failed to source the contain */
	asf_object_headerext_t *ext; /* this is here just for convenience */
	struct asfint_object_s    *first;
	struct asfint_object_s    *last;
};
typedef struct asf_object_header_s asf_object_header_t;

struct asf_object_data_s {
	ASF_OBJECT_COMMON
	asf_guid_t file_id;
	uint64_t   total_data_packets;
	uint16_t   reserved;
	uint64_t   packets_position;
};
typedef struct asf_object_data_s asf_object_data_t;

struct asf_index_entry_s {
	uint32_t packet_index;
	uint16_t packet_count;
};
typedef struct asf_index_entry_s asf_index_entry_t;

struct asf_object_index_s {
	ASF_OBJECT_COMMON
	asf_guid_t file_id;
	uint64_t   entry_time_interval;
	uint32_t   max_packet_count;
	uint32_t   entry_count;
	asf_index_entry_t *entries;
};
typedef struct asf_object_index_s asf_object_index_t;

#define ASF_FLAG_BROADCAST 1
#define ASF_FLAG_SEEKABLE  2

#define ASF_MAX_STREAMS 128

struct asf_file_s {
	const char *filename;
	asf_iostream_t iostream;

	uint64_t position;
	uint64_t packet;

	/* Top level objects */
	struct asf_object_header_s *header;
	struct asf_object_data_s   *data;
	struct asf_object_index_s  *index;

	/* position (in bytes) of data and index objects */
	uint64_t data_position;
	uint64_t index_position;

	asf_guid_t file_id;
	uint64_t   file_size;
	uint64_t   creation_date;
	uint64_t   data_packets_count;
	uint64_t   play_duration;
	uint64_t   send_duration;
	uint64_t   preroll;
	uint16_t   flags;
	uint32_t   packet_size;
	uint32_t   max_bitrate;

	asf_stream_t streams[ASF_MAX_STREAMS];
};

#endif
