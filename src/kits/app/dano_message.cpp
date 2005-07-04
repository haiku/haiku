/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler
 */


#include "dano_message.h"

#include <MessageUtils.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


struct dano_header {
	uint32	code;
	int32	size;
	uint32	what;
	uint32	_unknown;
};

struct dano_field_header {
	uint32	code;
	int32	size;
};

struct dano_field {
	uint32	type;
	int32	data_size;
	uint8	name_size;
	char	name[0];
} _PACKED;

static const uint32 kMessageFormat = 'FOB2';
static const uint32 kMessageFormatSwapped = '2BOF';

static const uint32 kDataField = 'SGDa';


size_t
BPrivate::dano_message_size(const char* buffer)
{
	dano_header* header = (dano_header*)buffer;

	if (header->code == kMessageFormatSwapped)
		return __swap_int32(header->size);

	return header->size;
}


status_t
BPrivate::unflatten_dano_message(uint32 magic, BDataIO& stream, BMessage& message)
{
	TReadHelper reader(&stream);
	message.MakeEmpty();

	if (magic == kMessageFormatSwapped)
		reader.SetSwap(true);

	int32 size;
	reader(size);
	reader(message.what);

	int32 value;
	reader(value);	// unknown

	size -= sizeof(dano_header);
	int32 offset = 0;

	while (offset < size) {
		dano_field_header fieldHeader;
		reader(fieldHeader);
printf("field.code = %.4s (%lx), field.size = %ld\n",
	(char*)&fieldHeader.code, fieldHeader.code, fieldHeader.size);

		if (fieldHeader.size + offset > size)
			return B_BAD_DATA;

		uint32 fieldSize = fieldHeader.size - sizeof(dano_field_header);
		char* fieldBuffer = (char*)malloc(fieldSize);
		if (fieldBuffer == NULL)
			throw (status_t)B_NO_MEMORY;

		reader(fieldBuffer, fieldSize);

		if (fieldHeader.code == kDataField) {
			dano_field* field = (dano_field*)fieldBuffer;

			int32 dataOffset = sizeof(dano_field) + field->name_size + 1;
			dataOffset = (dataOffset + 7) & ~7;
				// padding

			if (field->data_size + dataOffset + offset > size)
				return B_BAD_DATA;

			// ToDo: support fixed size for real
			bool fixedSize = true;
			if (field->type == B_STRING_TYPE)
				fixedSize = false;

			status_t status = message.AddData(field->name, field->type,
				fieldBuffer + dataOffset, field->data_size, fixedSize);

			if (status < B_OK) {
				free(fieldBuffer);
				throw status;
			}
		}

		free(fieldBuffer);
		offset += fieldHeader.size;
printf("offset = %ld, size = %ld\n", offset, size);
	}
	message.PrintToStream();
	return B_OK;
}

