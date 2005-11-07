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


typedef struct section_header_s {
	uint32		code;
	ssize_t		size;
	uint8		data[0];
} SectionHeader;


typedef struct message_header_s {
		int32		what;
		int32		padding;
} MessageHeader;


typedef struct offset_table_s {
	int32		indexTable;
	int32		endOfData;
	int64		padding;
} OffsetTable;


typedef struct single_item_s {
	type_code	type;
	ssize_t		itemSize;
	uint8		nameLength;
	char		name[0];
} _PACKED SingleItem;


typedef struct fixed_size_array_s {
	type_code	type;
	ssize_t		sizePerItem;
	uint8		nameLength;
	char		name[0];
} _PACKED FixedSizeArray;


typedef struct variable_size_array_s {
	type_code	type;
	int32		padding;
	uint8		nameLength;
	char		name[0];
} _PACKED VariableSizeArray;


static const uint32 kMessageFormatSwapped = '2BOF';


inline int32
pad_to_8(int32 value)
{
	return (value + 7) & ~7;
}


ssize_t
BPrivate::dano_message_size(const char *buffer)
{
	SectionHeader *header = (SectionHeader *)buffer;

	if (header->code == kMessageFormatSwapped)
		return __swap_int32(header->size);

	return header->size;
}


status_t
BPrivate::unflatten_dano_message(uint32 magic, BDataIO &stream,
	BMessage &message)
{
	TReadHelper reader(&stream);
	message.MakeEmpty();

	if (magic == kMessageFormatSwapped)
		reader.SetSwap(true);

	ssize_t size;
	reader(size);

	MessageHeader header;
	reader(header);
	message.what = header.what;

	size -= sizeof(SectionHeader) + sizeof(MessageHeader);
	int32 offset = 0;

	while (offset < size) {
		SectionHeader sectionHeader;
		reader(sectionHeader);

		if (offset + sectionHeader.size > size)
			return B_BAD_DATA;

		ssize_t fieldSize = sectionHeader.size - sizeof(SectionHeader);
		uint8 *fieldBuffer = (uint8 *)malloc(fieldSize);
		if (fieldBuffer == NULL)
			throw (status_t)B_NO_MEMORY;

		reader(fieldBuffer, fieldSize);

		switch (sectionHeader.code) {
			case SECTION_OFFSET_TABLE: break; /* discard */
			case SECTION_TARGET_INFORMATION: break; /* discard */
			case SECTION_SORTED_INDEX_TABLE: break; /* discard */
			case SECTION_END_OF_DATA: break; /* discard */

			case SECTION_SINGLE_ITEM_DATA: {
				SingleItem *field = (SingleItem *)fieldBuffer;

				int32 dataOffset = sizeof(SingleItem) + field->nameLength + 1;
				dataOffset = pad_to_8(dataOffset);

				if (offset + dataOffset + field->itemSize > size)
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
					fieldBuffer + dataOffset, field->itemSize, fixedSize);

				if (result < B_OK) {
					free(fieldBuffer);
					throw result;
				}
				break;
			}

			case SECTION_FIXED_SIZE_ARRAY_DATA: {
				FixedSizeArray *field = (FixedSizeArray *)fieldBuffer;

				int32 dataOffset = sizeof(FixedSizeArray) + field->nameLength + 1;
				dataOffset = pad_to_8(dataOffset);
				int32 count = *(int32 *)(fieldBuffer + dataOffset);
				dataOffset += 8; /* count and padding */

				if (offset + dataOffset + count * field->sizePerItem > size)
					return B_BAD_DATA;

				status_t result = B_OK;
				for (int32 i = 0; i < count; i++) {
					result = message.AddData(field->name, field->type,
						fieldBuffer + dataOffset, field->sizePerItem, true,
						count);

					if (result < B_OK) {
						free(fieldBuffer);
						throw result;
					}

					dataOffset += field->sizePerItem;
				}
				break;
			}

			case SECTION_VARIABLE_SIZE_ARRAY_DATA: {
				VariableSizeArray *field = (VariableSizeArray *)fieldBuffer;

				int32 dataOffset = sizeof(VariableSizeArray) + field->nameLength + 1;
				dataOffset = pad_to_8(dataOffset);
				int32 count = *(int32 *)(fieldBuffer + dataOffset);
				dataOffset += sizeof(int32);
				ssize_t totalSize = *(ssize_t *)fieldBuffer + dataOffset;
				dataOffset += sizeof(ssize_t);

				int32 *endPoints = (int32 *)fieldBuffer + dataOffset + totalSize;

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

	message.PrintToStream();
	return B_OK;
}
