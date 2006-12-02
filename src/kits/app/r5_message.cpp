/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "r5_message.h"

#include <DataIO.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <TokenSpace.h>

#define R5_MESSAGE_FLAG_VALID			0x01
#define R5_MESSAGE_FLAG_INCLUDE_TARGET	0x02
#define R5_MESSAGE_FLAG_INCLUDE_REPLY	0x04
#define R5_MESSAGE_FLAG_SCRIPT_MESSAGE	0x08

#define R5_FIELD_FLAG_VALID				0x01
#define R5_FIELD_FLAG_MINI_DATA			0x02
#define R5_FIELD_FLAG_FIXED_SIZE		0x04
#define R5_FIELD_FLAG_SINGLE_ITEM		0x08

namespace BPrivate {

static const uint32 kR5MessageMagic = 'FOB1';
static const uint32 kR5MessageMagicSwapped = '1BOF';


struct r5_message_header {
	uint32	magic;	// kR5MessageMagic
	uint32	checksum;
	ssize_t	flattened_size;
	int32	what;
	uint8	flags;
} _PACKED;


inline int32
pad_to_8(int32 value)
{
	return (value + 7) & ~7;
}


ssize_t
r5_message_flattened_size(const BMessage *message)
{
	BMessage::Private messagePrivate((BMessage *)message);
	BMessage::message_header* header = messagePrivate.GetMessageHeader();

	// header size (variable, depending on the flags)

	ssize_t flattenedSize = sizeof(r5_message_header);

	if (header->target != B_NULL_TOKEN)
		flattenedSize += sizeof(int32);

	if (header->reply_port >= 0 && header->reply_target != B_NULL_TOKEN
		&& header->reply_team >= 0) {
		// reply info + big flags
		flattenedSize += sizeof(port_id) + sizeof(int32) + sizeof(team_id) + 4;
	}

	// field size

	uint8 *data = messagePrivate.GetMessageData();
	BMessage::field_header* field = messagePrivate.GetMessageFields();
	for (int32 i = 0; i < header->field_count; i++, field++) {
		// flags and type
		flattenedSize += 1 + sizeof(type_code);

#if 0
		bool miniData = field->dataSize <= 255 && field->count <= 255;
#else
		// ToDo: we don't know the R5 dataSize yet (padding)
		bool miniData = false;
#endif

		// item count
		if (field->count > 1)
			flattenedSize += (miniData ? sizeof(uint8) : sizeof(uint32));

		// data size
		flattenedSize += (miniData ? sizeof(uint8) : sizeof(size_t));

		// name length and name
		flattenedSize += 1 + min_c(field->name_length - 1, 255);

		// data
		if (field->flags & FIELD_FLAG_FIXED_SIZE)
			flattenedSize += field->data_size;
		else {
			uint8 *source = data + field->offset + field->name_length;

			for (int32 i = 0; i < field->count; i++) {
				ssize_t itemSize = *(ssize_t *)source + sizeof(ssize_t);
				flattenedSize += pad_to_8(itemSize);
				source += itemSize;
			}
		}
	}

	// pseudo field with flags 0
	return flattenedSize + 1;
}


status_t
flatten_r5_message(const BMessage *message, char *buffer, ssize_t size)
{
	BMessage::Private messagePrivate((BMessage *)message);
	BMessage::message_header* header = messagePrivate.GetMessageHeader();
	uint8* data = messagePrivate.GetMessageData();

	r5_message_header* r5header = (r5_message_header *)buffer;
	uint8* pointer = (uint8 *)buffer + sizeof(r5_message_header);

	r5header->magic = kR5MessageMagic;
	r5header->what = message->what;
	r5header->checksum = 0;

	uint8 flags = R5_MESSAGE_FLAG_VALID;
	if (header->target != B_NULL_TOKEN) {
		*(int32 *)pointer = header->target;
		pointer += sizeof(int32);
		flags |= R5_MESSAGE_FLAG_INCLUDE_TARGET;
	}

	if (header->reply_port >= 0 && header->reply_target != B_NULL_TOKEN
		&& header->reply_team >= 0) {
		// reply info
		*(port_id *)pointer = header->reply_port;
		pointer += sizeof(port_id);
		*(int32 *)pointer = header->reply_target;
		pointer += sizeof(int32);
		*(team_id *)pointer = header->reply_team;
		pointer += sizeof(team_id);

		// big flags
		*pointer = (header->reply_target == B_PREFERRED_TOKEN ? 1 : 0);
		pointer++;

		*pointer = (header->flags & MESSAGE_FLAG_REPLY_REQUIRED ? 1 : 0);
		pointer++;

		*pointer = (header->flags & MESSAGE_FLAG_REPLY_DONE ? 1 : 0);
		pointer++;

		*pointer = (header->flags & MESSAGE_FLAG_IS_REPLY ? 1 : 0);
		pointer++;

		flags |= R5_MESSAGE_FLAG_INCLUDE_REPLY;
	}

	if (header->flags & MESSAGE_FLAG_HAS_SPECIFIERS)
		flags |= R5_MESSAGE_FLAG_SCRIPT_MESSAGE;

	r5header->flags = flags;

	// store the header size - used for the checksum later
	ssize_t headerSize = (uint32)pointer - (uint32)buffer;

	// collect and add the data
	BMessage::field_header* field = messagePrivate.GetMessageFields();
	for (int32 i = 0; i < header->field_count; i++, field++) {
		flags = R5_FIELD_FLAG_VALID;

		if (field->count == 1)
			flags |= R5_FIELD_FLAG_SINGLE_ITEM;
		// ToDo: we don't really know the data size now (padding missing)
		if (field->data_size <= 255 && field->count <= 255)
			;//flags |= R5_FIELD_FLAG_MINI_DATA;
		if (field->flags & FIELD_FLAG_FIXED_SIZE)
			flags |= R5_FIELD_FLAG_FIXED_SIZE;

		*pointer = flags;
		pointer++;

		*(type_code *)pointer = field->type;
		pointer += sizeof(type_code);

		if (!(flags & R5_FIELD_FLAG_SINGLE_ITEM)) {
			if (flags & R5_FIELD_FLAG_MINI_DATA) {
				*pointer = (uint8)field->count;
				pointer++;
			} else {
				*(int32 *)pointer = field->count;
				pointer += sizeof(int32);
			}
		}

		// we may have to adjust this to account for padding later
		uint8 *fieldSize = pointer;
		if (flags & R5_FIELD_FLAG_MINI_DATA) {
			*pointer = (uint8)field->data_size;
			pointer++;
		} else {
			*(ssize_t *)pointer = field->data_size;
			pointer += sizeof(ssize_t);
		}

		// name
		int32 nameLength = min_c(field->name_length - 1, 255);
		*pointer = (uint8)nameLength;
		pointer++;

		strncpy((char *)pointer, (char *)data + field->offset, nameLength);
		pointer += nameLength;

		// data
		uint8 *source = data + field->offset + field->name_length;
		if (flags & R5_FIELD_FLAG_FIXED_SIZE) {
			memcpy(pointer, source, field->data_size);
			pointer += field->data_size;
		} else {
			uint8 *previous = pointer;
			for (int32 i = 0; i < field->count; i++) {
				ssize_t itemSize = *(ssize_t *)source + sizeof(ssize_t);
				memcpy(pointer, source, itemSize);
				pointer += pad_to_8(itemSize);
				source += itemSize;
			}

			// adjust the field size to the padded value
			if (flags & R5_FIELD_FLAG_MINI_DATA)
				*fieldSize = (uint8)(pointer - previous);
			else
				*(ssize_t *)fieldSize = (pointer - previous);
		}
	}

	// terminate the fields with a pseudo field with flags 0 (not valid)
	*pointer = 0;
	pointer++;

	// calculate the flattened size from the pointers
	r5header->flattened_size = (uint32)pointer - (uint32)buffer;
	r5header->checksum = BPrivate::CalculateChecksum((uint8 *)(buffer + 8),
		headerSize - 8);
	return B_OK;
}


status_t
flatten_r5_message(const BMessage *message, BDataIO *stream, ssize_t *_size)
{
	// ToDo: This is not very cheap...
	ssize_t size = r5_message_flattened_size(message);
	char *buffer = (char *)malloc(size);
	if (!buffer)
		return B_NO_MEMORY;

	status_t result = flatten_r5_message(message, buffer, size);
	if (result < B_OK) {
		free(buffer);
		return result;
	}

	ssize_t written = stream->Write(buffer, size);
	if (written != size) {
		free(buffer);
		return (written >= 0 ? B_ERROR : written);
	}

	if (_size)
		*_size = size;

	free(buffer);
	return B_OK;
}


status_t
unflatten_r5_message(uint32 format, BMessage *message, const char *flatBuffer)
{
	r5_message_header *r5header = (r5_message_header *)flatBuffer;
	BMemoryIO stream(flatBuffer + 4, r5header->flattened_size - 4);
	return unflatten_r5_message(format, message, &stream);
}


status_t
unflatten_r5_message(uint32 format, BMessage *message, BDataIO *stream)
{
	message->MakeEmpty();

	TReadHelper reader(stream);
	BMessage::Private messagePrivate(message);
	BMessage::message_header *header = messagePrivate.GetMessageHeader();

	if (format == kR5MessageMagicSwapped)
		reader.SetSwap(true);

	// the stream is already advanced by the size of the "format"
	r5_message_header r5header;
	reader(((uint8 *)&r5header) + sizeof(uint32),
		sizeof(r5header) - sizeof(uint32));

	header->what = message->what = r5header.what;
	if (r5header.flags & R5_MESSAGE_FLAG_INCLUDE_TARGET)
		reader(header->target);

	if (r5header.flags & R5_MESSAGE_FLAG_INCLUDE_REPLY) {
		// reply info
		reader(header->reply_port);
		reader(header->reply_target);
		reader(header->reply_team);

		// big flags
		uint8 bigFlag;
		reader(bigFlag);
		if (bigFlag)
			header->reply_target = B_PREFERRED_TOKEN;

		reader(bigFlag);
		if (bigFlag)
			header->flags |= MESSAGE_FLAG_REPLY_REQUIRED;

		reader(bigFlag);
		if (bigFlag)
			header->flags |= MESSAGE_FLAG_REPLY_DONE;

		reader(bigFlag);
		if (bigFlag)
			header->flags |= MESSAGE_FLAG_IS_REPLY;
	}

	if (r5header.flags & R5_MESSAGE_FLAG_SCRIPT_MESSAGE)
		header->flags |= MESSAGE_FLAG_HAS_SPECIFIERS;

	uint8 flags;
	reader(flags);
	while (flags & R5_FIELD_FLAG_VALID) {
		bool fixedSize = flags & R5_FIELD_FLAG_FIXED_SIZE;
		bool miniData = flags & R5_FIELD_FLAG_MINI_DATA;
		bool singleItem = flags & R5_FIELD_FLAG_SINGLE_ITEM;

		type_code type;
		reader(type);

		int32 itemCount;
		if (!singleItem) {
			if (miniData) {
				uint8 miniCount;
				reader(miniCount);
				itemCount = miniCount;
			} else
				reader(itemCount);
		} else
			itemCount = 1;

		ssize_t dataSize;
		if (miniData) {
			uint8 miniSize;
			reader(miniSize);
			dataSize = miniSize;
		} else
			reader(dataSize);

		if (dataSize <= 0)
			return B_ERROR;

		// name
		uint8 nameLength;
		reader(nameLength);

		char nameBuffer[256];
		reader(nameBuffer, nameLength);
		nameBuffer[nameLength] = '\0';

		uint8 *buffer = (uint8 *)malloc(dataSize);
		uint8 *pointer = buffer;
		reader(buffer, dataSize);

		status_t result = B_OK;
		ssize_t itemSize = 0;
		if (fixedSize)
			itemSize = dataSize / itemCount;

		for (int32 i = 0; i < itemCount; i++) {
			if (!fixedSize) {
				itemSize = *(ssize_t *)pointer;
				pointer += sizeof(ssize_t);
			}

			result = message->AddData(nameBuffer, type, pointer, itemSize,
				fixedSize, itemCount);

			if (result < B_OK) {
				free(buffer);
				return result;
			}

			if (fixedSize)
				pointer += itemSize;
			else
				pointer += pad_to_8(itemSize + sizeof(ssize_t)) - sizeof(ssize_t);
		}

		free(buffer);

		// flags of next field or termination byte
		reader(flags);
	}

	return B_OK;
}

} // namespace BPrivate
