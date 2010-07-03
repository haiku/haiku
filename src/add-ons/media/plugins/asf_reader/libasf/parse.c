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

#include "asf.h"
#include "asfint.h"
#include "byteio.h"
#include "header.h"
#include "guid.h"
#include "debug.h"

/**
 * Read next object from buffer pointed by data. Notice that
 * no buffer overflow checks are done! This function always
 * expects to have 24 bytes available, which is the size of
 * the object header (GUID + data size)
 */
static void
asf_parse_read_object(asfint_object_t *obj, uint8_t *data)
{
	GetGUID(data, &obj->guid);
	obj->type = asf_guid_get_type(&obj->guid);
	obj->size = GetQWLE(data + 16);
	obj->full_data = data;
	obj->datalen = 0;
	obj->data = NULL;
	obj->next = NULL;

	if (obj->type == GUID_UNKNOWN) {
		debug_printf("unknown object: %x-%x-%x-%02x%02x%02x%02x%02x%02x%02x%02x, %ld bytes",
		             obj->guid.v1, obj->guid.v2, obj->guid.v3, obj->guid.v4[0],
		             obj->guid.v4[1], obj->guid.v4[2], obj->guid.v4[3], obj->guid.v4[4],
		             obj->guid.v4[5], obj->guid.v4[6], obj->guid.v4[7], (long) obj->size);
	}
}

/**
 * Parse header extension object. Takes a pointer to a newly allocated
 * header extension structure, a pointer to the data buffer and the 
 * length of the data buffer as its parameters. Subobject contents are
 * not parsed, but they are added as a linked list to the header object.
 */
static int
asf_parse_headerext(asf_object_headerext_t *header, uint8_t *buf, uint64_t buflen)
{
	int64_t datalen;
	uint8_t *data;

	if (header->size < 46) {
		/* invalide size for headerext */
		return ASF_ERROR_INVALID_OBJECT_SIZE;
	}

	/* Read reserved and datalen fields from the buffer */
	GetGUID(buf + 24, &header->reserved1);
	header->reserved2 = GetWLE(buf + 40);
	header->datalen = GetDWLE(buf + 42);

	if (header->datalen != header->size - 46) {
		/* invalid header extension data length value */
		return ASF_ERROR_INVALID_LENGTH;
	}
	header->data = buf + 46;

	debug_printf("parsing header extension subobjects");

	datalen = header->datalen;
	data = header->data;
	while (datalen > 0) {
		asfint_object_t *current;

		if (datalen < 24) {
			/* not enough data for reading a new object */
			break;
		}

		/* Allocate a new subobject */
		current = malloc(sizeof(asfint_object_t));
		if (!current) {
			return ASF_ERROR_OUTOFMEM;
		}

		asf_parse_read_object(current, data);
		if (current->size > datalen || current->size < 24) {
			/* invalid object size */
			break;
		}
		current->datalen = current->size - 24;
		current->data = data + 24;

		/* add to the list of subobjects */
		if (!header->first) {
			header->first = current;
			header->last = current;
		} else {
			header->last->next = current;
			header->last = current;
		}

		data += current->size;
		datalen -= current->size;
	}

	if (datalen != 0) {
		/* not enough data for reading the whole object */
		return ASF_ERROR_INVALID_LENGTH;
	}

	debug_printf("header extension subobjects parsed successfully");

	return header->size;
}

/**
 * Takes an initialized asf_file_t structure file as a parameter. Allocates
 * a new asf_object_header_t in file->header and uses the file->iostream to
 * read all fields and subobjects into it. Finally calls the
 * asf_parse_header_validate function to validate the values and parse the
 * commonly used values into the asf_file_t struct itself.
 */
int
asf_parse_header(asf_file_t *file)
{
	asf_object_header_t *header;
	asf_iostream_t *iostream;
	uint8_t hdata[30];
	int tmp;

	file->header = NULL;
	iostream = &file->iostream;

	/* object minimum is 24 bytes and header needs to have
	 * the subobject count field and two reserved fields */
	tmp = asf_byteio_read(iostream, hdata, 30);
	if (tmp < 0) {
		/* not enough data to read the header object */
		return tmp;
	}

	file->header = malloc(sizeof(asf_object_header_t));
	header = file->header;
	if (!header) {
		return ASF_ERROR_OUTOFMEM;
	}

	/* read the object and check its size value */
	asf_parse_read_object((asfint_object_t *) header, hdata);
	if (header->size < 30) {
		/* invalid size for header object */
		return ASF_ERROR_INVALID_OBJECT_SIZE;
	}

	/* read header object specific compulsory fields */
	header->subobjects = GetDWLE(hdata + 24);
	header->reserved1 = hdata[28];
	header->reserved2 = hdata[29];

	/* clear header extension object and subobject list */
	header->ext = NULL;
	header->first = NULL;
	header->last = NULL;

	/* the header data needs to be allocated for reading */
	header->datalen = header->size - 30;
	header->data = malloc(header->datalen * sizeof(uint8_t));
	if (!header->data) {
		return ASF_ERROR_OUTOFMEM;
	}

	tmp = asf_byteio_read(iostream, header->data, header->datalen);
	if (tmp < 0) {
		return tmp;
	}

	if (header->subobjects > 0) {
		uint64_t datalen;
		uint8_t *data;
		int i;

		debug_printf("starting to read subobjects");

		/* use temporary variables for use during the read */
		datalen = header->datalen;
		data = header->data;
		for (i=0; i<header->subobjects; i++) {
			asfint_object_t *current;

			if (datalen < 24) {
				/* not enough data for reading object */
				break;
			}

			current = malloc(sizeof(asfint_object_t));
			if (!current) {
				return ASF_ERROR_OUTOFMEM;
			}

			asf_parse_read_object(current, data);
			if (current->size > datalen || current->size < 24) {
				/* invalid object size */
				break;
			}

			/* Check if the current subobject is a header extension
			 * object or just a normal subobject */
			if (current->type == GUID_HEADER_EXTENSION && !header->ext) {
				int ret;
				asf_object_headerext_t *headerext;

				/* we handle header extension separately because it has
				 * some subobjects as well */
				current = realloc(current, sizeof(asf_object_headerext_t));
				headerext = (asf_object_headerext_t *) current;
				headerext->first = NULL;
				headerext->last = NULL;
				ret = asf_parse_headerext(headerext, data, datalen);

				if (ret < 0) {
					/* error parsing header extension */
					return ret;
				}

				header->ext = headerext;
			} else {
				if (current->type == GUID_HEADER_EXTENSION) {
					debug_printf("WARNING! Second header extension object found, ignoring it!");
				}

				current->datalen = current->size - 24;
				current->data = data + 24;

				/* add to list of subobjects */
				if (!header->first) {
					header->first = current;
					header->last = current;
				} else {
					header->last->next = current;
					header->last = current;
				}
			}

			data += current->size;
			datalen -= current->size;
		}

		if (i != header->subobjects || datalen != 0) {
			/* header data size doesn't match given subobject count */
			return ASF_ERROR_INVALID_VALUE;
		}

		debug_printf("%d subobjects read successfully", i);
	}

	tmp = asf_parse_header_validate(file, file->header);
	if (tmp < 0) {
		/* header read ok but doesn't validate correctly */
		return tmp;
	}

	debug_printf("header validated correctly");

	return header->size;
}

/**
 * Takes an initialized asf_file_t structure file as a parameter. Allocates
 * a new asf_object_data_t in file->data and uses the file->iostream to
 * read all its compulsory fields into it. Notice that the actual data is
 * not read in any way, because we need to be able to work with non-seekable
 * streams as well.
 */
int
asf_parse_data(asf_file_t *file)
{
	asf_object_data_t *data;
	asf_iostream_t *iostream;
	uint8_t ddata[50];
	int tmp;

	file->data = NULL;
	iostream = &file->iostream;

	/* object minimum is 24 bytes and data object needs to have
	 * 26 additional bytes for its internal fields */
	tmp = asf_byteio_read(iostream, ddata, 50);
	if (tmp < 0) {
		return tmp;
	}

	file->data = malloc(sizeof(asf_object_data_t));
	data = file->data;
	if (!data) {
		return ASF_ERROR_OUTOFMEM;
	}

	/* read the object and check its size value */
	asf_parse_read_object((asfint_object_t *) data, ddata);
	if (data->size < 50) {
		/* invalid size for data object */
		return ASF_ERROR_INVALID_OBJECT_SIZE;
	}

	/* read data object specific compulsory fields */
	GetGUID(ddata + 24, &data->file_id);
	data->total_data_packets = GetQWLE(ddata + 40);
	data->reserved = GetWLE(ddata + 48);
	data->packets_position = file->position + 50;

	/* If the file_id GUID in data object doesn't match the
	 * file_id GUID in headers, the file is corrupted */
	if (!asf_guid_equals(&data->file_id, &file->file_id)) {
		return ASF_ERROR_INVALID_VALUE;
	}

	/* if data->total_data_packets is non-zero (not a stream) and
	   the data packets count doesn't match, return error */
	if (data->total_data_packets &&
	    data->total_data_packets != file->data_packets_count) {
		return ASF_ERROR_INVALID_VALUE;
	}

	return 50;
}

/**
 * Takes an initialized asf_file_t structure file as a parameter. Allocates
 * a new asf_object_index_t in file->index and uses the file->iostream to
 * read all its compulsory fields into it. Notice that the actual data is
 * not read in any way, because we need to be able to work with non-seekable
 * streams as well.
 */
int
asf_parse_index(asf_file_t *file)
{
	asf_object_index_t *index;
	asf_iostream_t *iostream;
	uint8_t idata[56];
	uint64_t entry_data_size;
	uint8_t *entry_data = NULL;
	int tmp, i;

	file->index = NULL;
	iostream = &file->iostream;

	/* read the raw data of an index header */
	tmp = asf_byteio_read(iostream, idata, 56);
	if (tmp < 0) {
		printf("Could not read index header\n");
		return tmp;
	}

	/* allocate the index object */
	index = malloc(sizeof(asf_object_index_t));
	if (!index) {
		return ASF_ERROR_OUTOFMEM;
	}

	asf_parse_read_object((asfint_object_t *) index, idata);
	if (index->type != GUID_INDEX) {
		tmp = index->size;
		free(index);

		/* The guid type was wrong, just return the bytes to skip */
		return tmp;
	}

	if (index->size < 56) {
		/* invalid size for index object */
		free(index);
		return ASF_ERROR_INVALID_OBJECT_SIZE;
	}

	GetGUID(idata + 24, &index->file_id);
	index->entry_time_interval = GetQWLE(idata + 40);
	index->max_packet_count = GetDWLE(idata + 48);
	index->entry_count = GetDWLE(idata + 52);

	printf("INDEX\n");
	printf("Total Index Entries %d\n",index->entry_count);
	printf("Index Size in bytes %Ld\n",index->size);
	printf("Index Max Packet Count %d\n",index->max_packet_count);
	printf("Index Entry Time Interval %Ld\n",index->entry_time_interval);
	
	if (index->entry_count == 0) {
		printf("Index has no entries\n");
		file->index = index;
		return index->size;
	}

	if (index->entry_count * 6 + 56 > index->size) {
		free(index);
		return ASF_ERROR_INVALID_LENGTH;
	}

	entry_data_size = index->entry_count * 6;
	entry_data = malloc(entry_data_size * sizeof(uint8_t));
	if (!entry_data) {
		free(index);
		return ASF_ERROR_OUTOFMEM;
	}
	tmp = asf_byteio_read(iostream, entry_data, entry_data_size);
	if (tmp < 0) {
		printf("Could not read entry data\n");
		free(index);
		free(entry_data);
		return tmp;
	}

	index->entries = malloc(index->entry_count * sizeof(asf_index_entry_t));
	if (!index->entries) {
		free(index);
		free(entry_data);
		return ASF_ERROR_OUTOFMEM;
	}

	for (i=0; i<index->entry_count; i++) {
		index->entries[i].packet_index = GetDWLE(entry_data + i*6);
		index->entries[i].packet_count = GetWLE(entry_data + i*6 + 4);
	}

	free(entry_data);
	file->index = index;

	return index->size;
}
