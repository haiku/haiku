/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include "dano_message.h"

#include <MessageUtils.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


enum {
	SECTION_MESSAGE_HEADER = 'FOB2',
	SECTION_OFFSET_TABLE = 'STof',
	SECTION_TARGET_INFORMATION = 'ENwh',
	SECTION_SINGLE_ITEM_DATA = 'SGDa',
	SECTION_FIXED_SIZE_ARRAY_DATA = 'FADa',
	SECTION_VARIABLE_SIZE_ARRAY_DATA = 'VADa',
	SECTION_SORTED_INDEX_TABLE = 'DXIn',
	SECTION_END_OF_DATA = 'DDEn'
};


struct section_header {
	uint32		code;
	ssize_t		size;
	uint8		data[0];
} _PACKED;

struct message_header {
	int32		what;
	int32		padding;
} _PACKED;

typedef struct offset_table_s {
	int32		indexTable;
	int32		endOfData;
	int64		padding;
} OffsetTable;

struct single_item {
	type_code	type;
	ssize_t		item_size;
	uint8		name_length;
	char		name[0];
} _PACKED;

struct fixed_size_array {
	type_code	type;
	ssize_t		size_per_item;
	uint8		name_length;
	char		name[0];
} _PACKED;


struct variable_size_array {
	type_code	type;
	int32		padding;
	uint8		name_length;
	char		name[0];
} _PACKED;


static const uint32 kMessageFormatSwapped = '2BOF';


inline int32
pad_to_8(int32 value)
{
	return (value + 7) & ~7;
}


ssize_t
BPrivate::dano_message_flattened_size(const char *buffer)
{
	section_header *header = (section_header *)buffer;

	if (header->code == kMessageFormatSwapped)
		return __swap_int32(header->size);

	return header->size;
}


status_t
BPrivate::unflatten_dano_message(uint32 format, BDataIO &stream,
	BMessage &message)
{
	TReadHelper reader(&stream);
	message.MakeEmpty();

	if (format == kMessageFormatSwapped)
		reader.SetSwap(true);

	ssize_t size;
	reader(size);

	message_header header;
	reader(header);
	message.what = header.what;

	size -= sizeof(section_header) + sizeof(message_header);
	int32 offset = 0;

	while (offset < size) {
		section_header sectionHeader;
		reader(sectionHeader);

		// be safe. this shouldn't be necessary but in some testcases it was.
		sectionHeader.size = pad_to_8(sectionHeader.size);

		if (offset + sectionHeader.size > size)
			return B_BAD_DATA;

		ssize_t fieldSize = sectionHeader.size - sizeof(section_header);
		uint8 *fieldBuffer = NULL;
		if (fieldSize > 0) {
			// there may be no data. we shouldn't fail because of that
			fieldBuffer = (uint8 *)malloc(fieldSize);
			if (fieldBuffer == NULL)
				throw (status_t)B_NO_MEMORY;

			reader(fieldBuffer, fieldSize);
		}

		switch (sectionHeader.code) {
			case SECTION_OFFSET_TABLE: break; /* discard */
			case SECTION_TARGET_INFORMATION: break; /* discard */
			case SECTION_SORTED_INDEX_TABLE: break; /* discard */
			case SECTION_END_OF_DATA: break; /* discard */

			case SECTION_SINGLE_ITEM_DATA: {
				single_item *field = (single_item *)fieldBuffer;

				int32 dataOffset = sizeof(single_item) + field->name_length + 1;
				dataOffset = pad_to_8(dataOffset);

				if (offset + dataOffset + field->item_size > size)
					return B_BAD_DATA;

				// support for fixed size is not possible with a single item
				bool fixedSize = false;
				switch (field->type) {
					case B_RECT_TYPE:
					case B_POINT_TYPE:
					case B_INT8_TYPE:
					case B_INT16_TYPE:
					case B_INT32_TYPE:
					case B_INT64_TYPE:
					case B_BOOL_TYPE:
					case B_FLOAT_TYPE:
					case B_DOUBLE_TYPE:
					case B_POINTER_TYPE:
					case B_MESSENGER_TYPE:
						fixedSize = true;
						break;
					default:
						break;
				}

				status_t result = message.AddData(field->name, field->type,
					fieldBuffer + dataOffset, field->item_size, fixedSize);

				if (result < B_OK) {
					free(fieldBuffer);
					throw result;
				}
				break;
			}

			case SECTION_FIXED_SIZE_ARRAY_DATA: {
				fixed_size_array *field = (fixed_size_array *)fieldBuffer;

				int32 dataOffset = sizeof(fixed_size_array) + field->name_length + 1;
				dataOffset = pad_to_8(dataOffset);
				int32 count = *(int32 *)(fieldBuffer + dataOffset);
				dataOffset += 8; /* count and padding */

				if (offset + dataOffset + count * field->size_per_item > size)
					return B_BAD_DATA;

				status_t result = B_OK;
				for (int32 i = 0; i < count; i++) {
					result = message.AddData(field->name, field->type,
						fieldBuffer + dataOffset, field->size_per_item, true,
						count);

					if (result < B_OK) {
						free(fieldBuffer);
						throw result;
					}

					dataOffset += field->size_per_item;
				}
				break;
			}

			case SECTION_VARIABLE_SIZE_ARRAY_DATA: {
				variable_size_array *field = (variable_size_array *)fieldBuffer;

				int32 dataOffset = sizeof(variable_size_array) + field->name_length + 1;
				dataOffset = pad_to_8(dataOffset);
				int32 count = *(int32 *)(fieldBuffer + dataOffset);
				dataOffset += sizeof(int32);
				ssize_t totalSize = *(ssize_t *)(fieldBuffer + dataOffset);
				dataOffset += sizeof(ssize_t);

				int32 *endPoints = (int32 *)(fieldBuffer + dataOffset
					+ totalSize);

				status_t result = B_OK;
				for (int32 i = 0; i < count; i++) {
					int32 itemOffset = (i > 0 ? pad_to_8(endPoints[i - 1]) : 0);

					result = message.AddData(field->name, field->type,
						fieldBuffer + dataOffset + itemOffset,
						endPoints[i] - itemOffset, false, count);

					if (result < B_OK) {
						free(fieldBuffer);
						throw result;
					}
				}
				break;
			}
		}

		free(fieldBuffer);
		offset += sectionHeader.size;
	}

	return B_OK;
}
