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

#include <stdlib.h>
#include <stdio.h>

#include "asf.h"
#include "asfint.h"
#include "fileio.h"
#include "byteio.h"
#include "header.h"
#include "parse.h"
#include "data.h"
#include "debug.h"

asf_file_t *
asf_open_file(const char *filename)
{
	asf_file_t *file;
	asf_iostream_t stream;
	FILE *fstream;

	fstream = fopen(filename, "r");
	if (!fstream)
		return NULL;

	stream.read = asf_fileio_read_cb;
	stream.write = asf_fileio_write_cb;
	stream.seek = asf_fileio_seek_cb;
	stream.opaque = fstream;

	file = asf_open_cb(&stream);
	if (!file)
		return NULL;

	file->filename = filename;

	return file;
}

asf_file_t *
asf_open_cb(asf_iostream_t *iostream)
{
	asf_file_t *file;
	int i;

	if (!iostream)
		return NULL;

	file = calloc(1, sizeof(asf_file_t));
	if (!file)
		return NULL;

	file->filename = NULL;
	file->iostream.read = iostream->read;
	file->iostream.write = iostream->write;
	file->iostream.seek = iostream->seek;
	file->iostream.opaque = iostream->opaque;

	file->header = NULL;
	file->data = NULL;
	file->index = NULL;

	for (i=0; i < ASF_MAX_STREAMS; i++) {
		file->streams[i].type = ASF_STREAM_TYPE_NONE;
		file->streams[i].flags = ASF_STREAM_FLAG_NONE;
		file->streams[i].properties = NULL;
		file->streams[i].extended = NULL;
	}

	return file;
}

int
asf_init(asf_file_t *file)
{
	int tmp;

	if (!file)
		return ASF_ERROR_INTERNAL;

	tmp = asf_parse_header(file);
	if (tmp < 0) {
		debug_printf("error parsing header: %d", tmp);
		return tmp;
	}
	file->position += tmp;
	file->data_position = file->position;

	tmp = asf_parse_data(file);
	if (tmp < 0) {
		debug_printf("error parsing data object: %d", tmp);
		return tmp;
	}
	file->position += tmp;

	if (file->flags & ASF_FLAG_SEEKABLE && file->iostream.seek) {
		int64_t seek_position;

		file->index_position = file->data_position +
		                       file->data->size;

		seek_position = file->iostream.seek(file->iostream.opaque,
		                                    file->index_position);

		/* if first seek fails, we can try to recover and just ignore seeking */
		if (seek_position >= 0) {
			while (seek_position == file->index_position &&
			       file->index_position < file->file_size && !file->index) {
				tmp = asf_parse_index(file);
				if (tmp < 0) {
					debug_printf("Error finding index object! %d", tmp);
					break;
				}

				/* The object read was something else than index */
				if (!file->index)
					file->index_position += tmp;

				seek_position = file->iostream.seek(file->iostream.opaque,
								  file->index_position);
			}

			if (!file->index) {
				debug_printf("Couldn't find an index object");
				file->index_position = 0;
			}

			seek_position = file->iostream.seek(file->iostream.opaque,
							  file->data->packets_position);
			if (seek_position != file->data->packets_position) {
				/* Couldn't seek back to packets position, this is fatal! */
				return ASF_ERROR_SEEK;
			}
		}
	}

	for (tmp = 0; tmp < ASF_MAX_STREAMS; tmp++) {
		if (file->streams[tmp].type != ASF_STREAM_TYPE_NONE) {
			debug_printf("stream %d of type %d found!", tmp, file->streams[tmp].type);
		}
	}

	return 0;
}

void
asf_close(asf_file_t *file)
{
	if (file) {
		int i;

		asf_free_header(file->header);
		free(file->data);
		if (file->index)
			free(file->index->entries);
		free(file->index);

		if (file->filename)
			fclose(file->iostream.opaque);

		for (i=0; i < ASF_MAX_STREAMS; i++) {
			free(file->streams[i].properties);
			free(file->streams[i].extended);
		}

		free(file);
	}
}

asf_packet_t *
asf_packet_create()
{
	asf_packet_t *ret;

	ret = malloc(sizeof(asf_packet_t));
	if (!ret)
		return NULL;

	asf_data_init_packet(ret);

	return ret;
}

int
asf_get_packet(asf_file_t *file, asf_packet_t *packet)
{
	int tmp;

	if (!file || !packet)
		return ASF_ERROR_INTERNAL;

	if (file->packet >= file->data_packets_count) {
		return 0;
	}

	tmp = asf_data_get_packet(packet, file);
	if (tmp < 0) {
		return tmp;
	}

	file->position += tmp;
	file->packet++;

	return tmp;
}

void
asf_packet_destroy(asf_packet_t *packet)
{
	asf_data_free_packet(packet);
	free(packet);
}

int64_t
asf_seek_to_msec(asf_file_t *file, int64_t msec)
{
	uint64_t packet;
	uint64_t new_position;
	uint64_t new_msec;
	int64_t seek_position;

	if (!file)
		return ASF_ERROR_INTERNAL;

	if (!(file->flags & ASF_FLAG_SEEKABLE) || !file->iostream.seek) {
		return ASF_ERROR_SEEKABLE;
	}

	/* Index structure is missing, check if we can still seek */
	if (file->index == NULL) {
		int i, audiocount;

		audiocount = 0;
		for (i=0; i<ASF_MAX_STREAMS; i++) {
			if (file->streams[i].type == ASF_STREAM_TYPE_NONE)
				continue;

			/* Non-audio files are not seekable without index */
			if (file->streams[i].type != ASF_STREAM_TYPE_AUDIO)
				return ASF_ERROR_SEEKABLE;
			else
				audiocount++;
		}

		/* Audio files with more than one audio track are not seekable
		 * without index */
		if (audiocount != 1)
			return ASF_ERROR_SEEKABLE;
	}

	if (msec > (file->play_duration / 10000)) {
		return ASF_ERROR_SEEK;
	}

	if (file->index) {
		uint32_t index_entry;

		/* Fetch current packet from index entry structure */
		index_entry = msec * 10000 / file->index->entry_time_interval;
		if (index_entry >= file->index->entry_count) {
			return ASF_ERROR_SEEK;
		}
		packet = file->index->entries[index_entry].packet_index;

		/* the correct msec time isn't known before reading the packet */
		new_msec = msec;
	} else {
		/* convert msec into bytes per second and divide with packet_size */
		packet = msec * file->max_bitrate / 8000 / file->packet_size;

		/* calculate the resulting position in the audio stream */
		new_msec = packet * file->packet_size * 8000 / file->max_bitrate;
	}

	/* calculate new position to be in the beginning of the current frame */
	new_position = file->data->packets_position + packet * file->packet_size;

	seek_position = file->iostream.seek(file->iostream.opaque, new_position);
	if (seek_position < 0 || seek_position != new_position) {
		return ASF_ERROR_SEEK;
	}

	/* update current file position information */
	file->position = new_position;
	file->packet = packet;

	return new_msec;
}

asf_metadata_t *
asf_header_get_metadata(asf_file_t *file)
{
	if (!file || !file->header)
		return NULL;

	return asf_header_metadata(file->header);
}

void
asf_header_destroy(asf_file_t *file)
{
	if (!file)
		return;

	asf_free_header(file->header);
	file->header = NULL;
}

void
asf_metadata_destroy(asf_metadata_t *metadata)
{
	asf_header_free_metadata(metadata);
}

uint8_t
asf_get_stream_count(asf_file_t *file)
{
	uint8_t ret = 0;
	int i;

	for (i = 0; i < ASF_MAX_STREAMS; i++) {
		if (file->streams[i].type != ASF_STREAM_TYPE_NONE)
			ret = i;
	}

	return ret;
}

int
asf_is_broadcast(asf_file_t *file) {
	return (file->flags & ASF_FLAG_BROADCAST);
}

int
asf_is_seekable(asf_file_t *file) {
	return (file->flags & ASF_FLAG_SEEKABLE);
}

asf_stream_t *
asf_get_stream(asf_file_t *file, uint8_t track)
{
	if (!file || track >= ASF_MAX_STREAMS)
		return NULL;

	return &file->streams[track];
}

uint64_t
asf_get_file_size(asf_file_t *file)
{
	if (!file)
		return 0;

	return file->file_size;
}

uint64_t
asf_get_creation_date(asf_file_t *file)
{
	if (!file)
		return 0;

	return file->creation_date;
}

uint64_t
asf_get_data_packets(asf_file_t *file)
{
	if (!file)
		return 0;

	return file->data_packets_count;
}

uint64_t
asf_get_duration(asf_file_t *file)
{
	if (!file)
		return 0;

	return file->play_duration;
}

uint32_t
asf_get_max_bitrate(asf_file_t *file)
{
	if (!file)
		return 0;

	return file->max_bitrate;
}

