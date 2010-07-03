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

#include <stdlib.h>
#include <string.h>

#include "asfint.h"
#include "byteio.h"
#include "data.h"
#include "parse.h"
#include "debug.h"

static int
asf_data_read_packet_fields(asf_packet_t *packet, uint8_t flags,
                            uint8_t *data, uint32_t len)
{
	uint8_t datalen;

	datalen = GETLEN2b((flags >> 1) & 0x03) +
	          GETLEN2b((flags >> 3) & 0x03) +
	          GETLEN2b((flags >> 5) & 0x03) + 6;

	if (datalen > len) {
		return ASF_ERROR_INVALID_LENGTH;
	}

	packet->length = GETVALUE2b((flags >> 5) & 0x03, data);
	data += GETLEN2b((flags >> 5) & 0x03);
	packet->sequence = GETVALUE2b((flags >> 1) & 0x03, data);
	data += GETLEN2b((flags >> 1) & 0x03);
	packet->padding_length = GETVALUE2b((flags >> 3) & 0x03, data);
	data += GETLEN2b((flags >> 3) & 0x03);
	packet->send_time = GetDWLE(data);
	data += 4;
	packet->duration = GetWLE(data);
	data += 2;

	return datalen;
}

static int
asf_data_read_payload_fields(asf_payload_t *payload, uint8_t flags, uint8_t *data, int size)
{
	uint8_t datalen;

	datalen = GETLEN2b(flags & 0x03) +
	          GETLEN2b((flags >> 2) & 0x03) +
	          GETLEN2b((flags >> 4) & 0x03);

	if (datalen > size) {
		return ASF_ERROR_INVALID_LENGTH;
	}

	payload->media_object_number = GETVALUE2b((flags >> 4) & 0x03, data);
	data += GETLEN2b((flags >> 4) & 0x03);
	payload->media_object_offset = GETVALUE2b((flags >> 2) & 0x03, data);
	data += GETLEN2b((flags >> 2) & 0x03);
	payload->replicated_length = GETVALUE2b(flags & 0x03, data);
	data += GETLEN2b(flags & 0x03);

	return datalen;
}

static int
asf_data_read_payloads(asf_packet_t *packet,
                       uint64_t preroll,
                       uint8_t multiple,
                       uint8_t flags,
                       uint8_t *data,
                       uint32_t datalen)
{
	asf_payload_t pl;
        uint32_t skip;
	int i, tmp;

	skip = 0, i = 0;
	while (i < packet->payload_count) {
		uint8_t pts_delta = 0;
		int compressed = 0;

		if (skip+1 > datalen) {
			return ASF_ERROR_INVALID_LENGTH;
		}
		pl.stream_number = data[skip] & 0x7f;
		pl.key_frame = !!(data[skip] & 0x80);
		skip++;

		tmp = asf_data_read_payload_fields(&pl, flags, data + skip, datalen - skip);
		if (tmp < 0) {
			return tmp;
		}
		skip += tmp;

		if (pl.replicated_length > 1) {
			if (pl.replicated_length < 8 || pl.replicated_length + skip > datalen) {
				/* not enough data */
				return ASF_ERROR_INVALID_LENGTH;
			}
			pl.replicated_data = data + skip;
			skip += pl.replicated_length;

			pl.pts = GetDWLE(pl.replicated_data + 4);
		} else if (pl.replicated_length == 1) {
			if (skip+1 > datalen) {
				/* not enough data */
				return ASF_ERROR_INVALID_LENGTH;
			}

			/* in compressed payload object offset is actually pts */
			pl.pts = pl.media_object_offset;
			pl.media_object_offset = 0;

			pl.replicated_length = 0;
			pl.replicated_data = NULL;

			pts_delta = data[skip];
			skip++;
			compressed = 1;
		} else {
			pl.pts = packet->send_time;
			pl.replicated_data = NULL;
		}

		/* substract preroll value from pts since it's counted in */
		if (pl.pts > preroll) {
			pl.pts -= preroll;
		} else {
			pl.pts = 0;
		}

		if (multiple) {
			if (skip + 2 > datalen) {
				/* not enough data */
				return ASF_ERROR_INVALID_LENGTH;
			}
			pl.datalen = GetWLE(data + skip);
			skip += 2;
		} else {
			pl.datalen = datalen - skip;
		}

		if (compressed) {
			uint32_t start = skip, used = 0;
			int payloads, idx;

			/* count how many compressed payloads this payload includes */
			for (payloads=0; used < pl.datalen; payloads++) {
				used += 1 + data[start + used];
			}

			if (used != pl.datalen) {
				/* invalid compressed data size */
				return ASF_ERROR_INVALID_LENGTH;
			}

			/* add additional payloads excluding the already allocated one */
			packet->payload_count += payloads - 1;
			if (packet->payload_count > packet->payloads_size) {
				void *tempptr;

				tempptr = realloc(packet->payloads,
				                  packet->payload_count * sizeof(asf_payload_t));
				if (!tempptr) {
					return ASF_ERROR_OUTOFMEM;
				}
				packet->payloads = tempptr;
				packet->payloads_size = packet->payload_count;
			}

			for (idx = 0; idx < payloads; idx++) {
				pl.datalen = data[skip];
				skip++;

				/* Set data correctly */
				pl.data = data + skip;
				skip += pl.datalen;

				/* Copy the final payload and update the PTS */
				memcpy(&packet->payloads[i], &pl, sizeof(asf_payload_t));
				packet->payloads[i].pts = pl.pts + idx * pts_delta;
				i++;

				debug_printf("payload(%d/%d) stream: %d, key frame: %d, object: %d, offset: %d, pts: %d, datalen: %d",
					     i, packet->payload_count, pl.stream_number, (int) pl.key_frame, pl.media_object_number,
					     pl.media_object_offset, pl.pts + idx * pts_delta, pl.datalen);
			}
		} else {
			pl.data = data + skip;
			memcpy(&packet->payloads[i], &pl, sizeof(asf_payload_t));

			/* update the skipped data amount and payload index */
			skip += pl.datalen;
			i++;

			debug_printf("payload(%d/%d) stream: %d, key frame: %d, object: %d, offset: %d, pts: %d, datalen: %d",
				     i, packet->payload_count, pl.stream_number, (int) pl.key_frame, pl.media_object_number,
				     pl.media_object_offset, pl.pts, pl.datalen);
		}
	}

	return skip;
}

void
asf_data_init_packet(asf_packet_t *packet)
{
	packet->ec_length = 0;
	packet->ec_data = NULL;

	packet->length = 0;
	packet->padding_length = 0;
	packet->send_time = 0;
	packet->duration = 0;

	packet->payload_count = 0;
	packet->payloads = NULL;
	packet->payloads_size = 0;

	packet->payload_data_len = 0;
	packet->payload_data = NULL;

	packet->data = NULL;
	packet->data_size = 0;
}

int
asf_data_get_packet(asf_packet_t *packet, asf_file_t *file)
{
	asf_iostream_t *iostream;
	uint32_t read = 0;
	int packet_flags, packet_property;
	void *tmpptr;
	int tmp;

	iostream = &file->iostream;
	if (file->packet_size == 0) {
		return ASF_ERROR_INVALID_LENGTH;
	}

	/* If the internal data is not allocated, allocate it */
	if (packet->data_size < file->packet_size) {
		tmpptr = realloc(packet->data, file->packet_size);
		if (!tmpptr) {
			return ASF_ERROR_OUTOFMEM;
		}
		packet->data = tmpptr;
		packet->data_size = file->packet_size;
	}

	tmp = asf_byteio_read(iostream, packet->data, file->packet_size);
	if (tmp < 0) {
		/* Error reading packet data */
		return tmp;
	}

	tmp = packet->data[read++];
	if (tmp & 0x80) {
		uint8_t opaque_data, ec_length_type;

		packet->ec_length = tmp & 0x0f;
		opaque_data = (tmp >> 4) & 0x01;
		ec_length_type = (tmp >> 5) & 0x03;

		if (ec_length_type != 0x00 ||
		    opaque_data != 0 ||
		    packet->ec_length != 0x02) {
			/* incorrect error correction flags */
			return ASF_ERROR_INVALID_VALUE;
		}

		if (read+packet->ec_length > file->packet_size) {
			return ASF_ERROR_INVALID_LENGTH;
		}
		packet->ec_data = &packet->data[read];
		read += packet->ec_length;
	} else {
		packet->ec_length = 0;
		packet->ec_data = NULL;
	}

	if (read+2 > file->packet_size) {
		return ASF_ERROR_INVALID_LENGTH;
	}
	packet_flags = packet->data[read++];
	packet_property = packet->data[read++];

	tmp = asf_data_read_packet_fields(packet, packet_flags,
	                                  packet->data + read,
	                                  file->packet_size - read);
	if (tmp < 0) {
		return tmp;
	}
	read += tmp;

	/* this is really idiotic, packet length can (and often will) be
	 * undefined and we just have to use the header packet size as the size
	 * value */
	if (((packet_flags >> 5) & 0x03) == 0) {
		packet->length = file->packet_size;
	}
	
	/* this is also really idiotic, if packet length is smaller than packet
	 * size, we need to manually add the additional bytes into padding length
	 * because the padding bytes only count up to packet length value */
	if (packet->length < file->packet_size) {
		packet->padding_length += file->packet_size - packet->length;
		packet->length = file->packet_size;
	}

	if (packet->length != file->packet_size) {
		/* packet with invalid length value */
		return ASF_ERROR_INVALID_LENGTH;
	}

	/* check if we have multiple payloads */
	if (packet_flags & 0x01) {
		int payload_length_type;

		if (read+1 > packet->length) {
			return ASF_ERROR_INVALID_LENGTH;
		}
		tmp = packet->data[read++];

		packet->payload_count = tmp & 0x3f;
		payload_length_type = (tmp >> 6) & 0x03;

		if (packet->payload_count == 0) {
			/* there should always be at least one payload */
			return ASF_ERROR_INVALID_VALUE;
		}

		if (payload_length_type != 0x02) {
			/* in multiple payloads datalen should always be a word */
			return ASF_ERROR_INVALID_VALUE;
		}
	} else {
		packet->payload_count = 1;
	}
	packet->payload_data_len = packet->length - read;

	if (packet->payload_count > packet->payloads_size) {
		tmpptr = realloc(packet->payloads,
		                 packet->payload_count * sizeof(asf_payload_t));
		if (!tmpptr) {
			return ASF_ERROR_OUTOFMEM;
		}
		packet->payloads = tmpptr;
		packet->payloads_size = packet->payload_count;
	}

	packet->payload_data = &packet->data[read];
	read += packet->payload_data_len;

	/* The return value will be consumed bytes, not including the padding */
	tmp = asf_data_read_payloads(packet, file->preroll, packet_flags & 0x01,
	                             packet_property, packet->payload_data,
	                             packet->payload_data_len - packet->padding_length);
	if (tmp < 0) {
		return tmp;
	}

	debug_printf("packet read %d bytes, eclen: %d, length: %d, padding: %d, time %d, duration: %d, payloads: %d",
	             read, packet->ec_length, packet->length, packet->padding_length, packet->send_time,
	             packet->duration, packet->payload_count);

	return read;
}

void
asf_data_free_packet(asf_packet_t *packet)
{
	if (!packet)
		return;

	free(packet->payloads);
	free(packet->data);

	packet->ec_data = NULL;
	packet->payloads = NULL;
	packet->payload_data = NULL;
	packet->data = NULL;
}
