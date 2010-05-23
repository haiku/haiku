/*
 * Copyright 2005-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#include <MessageAdapter.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>

#include <stdlib.h>


namespace BPrivate {

#define R5_MESSAGE_FLAG_VALID			0x01
#define R5_MESSAGE_FLAG_INCLUDE_TARGET	0x02
#define R5_MESSAGE_FLAG_INCLUDE_REPLY	0x04
#define R5_MESSAGE_FLAG_SCRIPT_MESSAGE	0x08

#define R5_FIELD_FLAG_VALID				0x01
#define R5_FIELD_FLAG_MINI_DATA			0x02
#define R5_FIELD_FLAG_FIXED_SIZE		0x04
#define R5_FIELD_FLAG_SINGLE_ITEM		0x08


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


struct r5_message_header {
	uint32	magic;
	uint32	checksum;
	int32	flattened_size;
	int32	what;
	uint8	flags;
} _PACKED;


struct dano_section_header {
	uint32		code;
	int32		size;
	uint8		data[0];
} _PACKED;


struct dano_message_header {
	int32		what;
	int32		padding;
} _PACKED;


typedef struct offset_table_s {
	int32		indexTable;
	int32		endOfData;
	int64		padding;
} OffsetTable;


struct dano_single_item {
	type_code	type;
	int32		item_size;
	uint8		name_length;
	char		name[0];
} _PACKED;


struct dano_fixed_size_array {
	type_code	type;
	int32		size_per_item;
	uint8		name_length;
	char		name[0];
} _PACKED;


struct dano_variable_size_array {
	type_code	type;
	int32		padding;
	uint8		name_length;
	char		name[0];
} _PACKED;


inline int32
pad_to_8(int32 value)
{
	return (value + 7) & ~7;
}


ssize_t
MessageAdapter::FlattenedSize(uint32 format, const BMessage *from)
{
	switch (format) {
		case MESSAGE_FORMAT_R5:
		case MESSAGE_FORMAT_R5_SWAPPED:
			return _R5FlattenedSize(from);
	}

	return -1;
}


status_t
MessageAdapter::Flatten(uint32 format, const BMessage *from, char *buffer,
	ssize_t *size)
{
	switch (format) {
		case MESSAGE_FORMAT_R5:
		case MESSAGE_FORMAT_R5_SWAPPED:
			return _FlattenR5Message(format, from, buffer, size);
	}

	return B_ERROR;
}


status_t
MessageAdapter::Flatten(uint32 format, const BMessage *from, BDataIO *stream,
	ssize_t *size)
{
	switch (format) {
		case MESSAGE_FORMAT_R5:
		case MESSAGE_FORMAT_R5_SWAPPED:
		{
			ssize_t flattenedSize = _R5FlattenedSize(from);
			char *buffer = (char *)malloc(flattenedSize);
			if (!buffer)
				return B_NO_MEMORY;

			status_t result = _FlattenR5Message(format, from, buffer,
				&flattenedSize);
			if (result < B_OK) {
				free(buffer);
				return result;
			}

			ssize_t written = stream->Write(buffer, flattenedSize);
			if (written != flattenedSize) {
				free(buffer);
				return (written >= 0 ? B_ERROR : written);
			}

			if (size)
				*size = flattenedSize;

			free(buffer);
			return B_OK;
		}
	}

	return B_ERROR;
}


status_t
MessageAdapter::Unflatten(uint32 format, BMessage *into, const char *buffer)
{
	if (format == KMessage::kMessageHeaderMagic) {
		KMessage message;
		status_t result = message.SetTo(buffer,
			((KMessage::Header *)buffer)->size);
		if (result != B_OK)
			return result;

		return _ConvertKMessage(&message, into);
	}

	try {
		switch (format) {
			case MESSAGE_FORMAT_R5:
			{
				r5_message_header *header = (r5_message_header *)buffer;
				BMemoryIO stream(buffer + sizeof(uint32),
					header->flattened_size - sizeof(uint32));
				return _UnflattenR5Message(format, into, &stream);
			}

			case MESSAGE_FORMAT_R5_SWAPPED:
			{
				r5_message_header *header = (r5_message_header *)buffer;
				BMemoryIO stream(buffer + sizeof(uint32),
					__swap_int32(header->flattened_size) - sizeof(uint32));
				return _UnflattenR5Message(format, into, &stream);
			}

			case MESSAGE_FORMAT_DANO:
			case MESSAGE_FORMAT_DANO_SWAPPED:
			{
				dano_section_header *header = (dano_section_header *)buffer;
				ssize_t size = header->size;
				if (header->code == MESSAGE_FORMAT_DANO_SWAPPED)
					size = __swap_int32(size);

				BMemoryIO stream(buffer + sizeof(uint32), size - sizeof(uint32));
				return _UnflattenDanoMessage(format, into, &stream);
			}
		}
	} catch (status_t error) {
		into->MakeEmpty();
		return error;
	}

	return B_NOT_A_MESSAGE;
}


status_t
MessageAdapter::Unflatten(uint32 format, BMessage *into, BDataIO *stream)
{
	try {
		switch (format) {
			case MESSAGE_FORMAT_R5:
			case MESSAGE_FORMAT_R5_SWAPPED:
				return _UnflattenR5Message(format, into, stream);

			case MESSAGE_FORMAT_DANO:
			case MESSAGE_FORMAT_DANO_SWAPPED:
				return _UnflattenDanoMessage(format, into, stream);
		}
	} catch (status_t error) {
		into->MakeEmpty();
		return error;
	}

	return B_NOT_A_MESSAGE;
}


status_t
MessageAdapter::_ConvertKMessage(const KMessage *fromMessage,
	BMessage *toMessage)
{
	if (!fromMessage || !toMessage)
		return B_BAD_VALUE;

	// make empty and init what of the target message
	toMessage->MakeEmpty();
	toMessage->what = fromMessage->What();

	BMessage::Private toPrivate(toMessage);
	toPrivate.SetTarget(fromMessage->TargetToken());
	toPrivate.SetReply(B_SYSTEM_TEAM, fromMessage->ReplyPort(),
		fromMessage->ReplyToken());

	// iterate through the fields and import them in the target message
	KMessageField field;
	while (fromMessage->GetNextField(&field) == B_OK) {
		int32 elementCount = field.CountElements();
		if (elementCount > 0) {
			for (int32 i = 0; i < elementCount; i++) {
				int32 size;
				const void *data = field.ElementAt(i, &size);
				status_t result;

				if (field.TypeCode() == B_MESSAGE_TYPE) {
					// message type: if it's a KMessage, convert it
					KMessage message;
					if (message.SetTo(data, size) == B_OK) {
						BMessage bMessage;
						result = _ConvertKMessage(&message, &bMessage);
						if (result < B_OK)
							return result;

						result = toMessage->AddMessage(field.Name(), &bMessage);
					} else {
						// just add it
						result = toMessage->AddData(field.Name(),
							field.TypeCode(), data, size,
							field.HasFixedElementSize(), 1);
					}
				} else {
					result = toMessage->AddData(field.Name(), field.TypeCode(),
						data, size, field.HasFixedElementSize(), 1);
				}

				if (result < B_OK)
					return result;
			}
		}
	}

	return B_OK;
}


ssize_t
MessageAdapter::_R5FlattenedSize(const BMessage *from)
{
	BMessage::Private messagePrivate((BMessage *)from);
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
	BMessage::field_header *field = messagePrivate.GetMessageFields();
	for (uint32 i = 0; i < header->field_count; i++, field++) {
		// flags and type
		flattenedSize += 1 + sizeof(type_code);

#if 0
		bool miniData = field->dataSize <= 255 && field->count <= 255;
#else
		// TODO: we don't know the R5 dataSize yet (padding)
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

			for (uint32 i = 0; i < field->count; i++) {
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
MessageAdapter::_FlattenR5Message(uint32 format, const BMessage *from,
	char *buffer, ssize_t *size)
{
	BMessage::Private messagePrivate((BMessage *)from);
	BMessage::message_header *header = messagePrivate.GetMessageHeader();
	uint8 *data = messagePrivate.GetMessageData();

	r5_message_header *r5header = (r5_message_header *)buffer;
	uint8 *pointer = (uint8 *)buffer + sizeof(r5_message_header);

	r5header->magic = MESSAGE_FORMAT_R5;
	r5header->what = from->what;
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
	ssize_t headerSize = (addr_t)pointer - (addr_t)buffer;

	// collect and add the data
	BMessage::field_header *field = messagePrivate.GetMessageFields();
	for (uint32 i = 0; i < header->field_count; i++, field++) {
		flags = R5_FIELD_FLAG_VALID;

		if (field->count == 1)
			flags |= R5_FIELD_FLAG_SINGLE_ITEM;
		// TODO: we don't really know the data size now (padding missing)
//		if (field->data_size <= 255 && field->count <= 255)
//			flags |= R5_FIELD_FLAG_MINI_DATA;
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
			for (uint32 i = 0; i < field->count; i++) {
				ssize_t itemSize = *(ssize_t *)source + sizeof(ssize_t);
				memcpy(pointer, source, itemSize);
				ssize_t paddedSize = pad_to_8(itemSize);
				memset(pointer + itemSize, 0, paddedSize - itemSize);
				pointer += paddedSize;
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
	r5header->flattened_size = (addr_t)pointer - (addr_t)buffer;
	r5header->checksum = CalculateChecksum((uint8 *)(buffer + 8),
		headerSize - 8);

	if (size)
		*size = r5header->flattened_size;

	return B_OK;
}


status_t
MessageAdapter::_UnflattenR5Message(uint32 format, BMessage *into,
	BDataIO *stream)
{
	into->MakeEmpty();

	BMessage::Private messagePrivate(into);
	BMessage::message_header *header = messagePrivate.GetMessageHeader();

	TReadHelper reader(stream);
	if (format == MESSAGE_FORMAT_R5_SWAPPED)
		reader.SetSwap(true);

	// the stream is already advanced by the size of the "format"
	r5_message_header r5header;
	reader(((uint8 *)&r5header) + sizeof(uint32),
		sizeof(r5header) - sizeof(uint32));

	header->what = into->what = r5header.what;
	if (r5header.flags & R5_MESSAGE_FLAG_INCLUDE_TARGET)
		reader(&header->target, sizeof(header->target));

	if (r5header.flags & R5_MESSAGE_FLAG_INCLUDE_REPLY) {
		// reply info
		reader(&header->reply_port, sizeof(header->reply_port));
		reader(&header->reply_target, sizeof(header->reply_target));
		reader(&header->reply_team, sizeof(header->reply_team));

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

		if (format == MESSAGE_FORMAT_R5) {
			for (int32 i = 0; i < itemCount; i++) {
				if (!fixedSize) {
					itemSize = *(ssize_t *)pointer;
					pointer += sizeof(ssize_t);
				}

				result = into->AddData(nameBuffer, type, pointer, itemSize,
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
		} else {
			for (int32 i = 0; i < itemCount; i++) {
				if (!fixedSize) {
					itemSize = __swap_int32(*(ssize_t *)pointer);
					pointer += sizeof(ssize_t);
				}

				swap_data(type, pointer, itemSize, B_SWAP_ALWAYS);
				result = into->AddData(nameBuffer, type, pointer, itemSize,
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
		}

		free(buffer);

		// flags of next field or termination byte
		reader(flags);
	}

	return B_OK;
}


status_t
MessageAdapter::_UnflattenDanoMessage(uint32 format, BMessage *into,
	BDataIO *stream)
{
	into->MakeEmpty();

	TReadHelper reader(stream);
	if (format == MESSAGE_FORMAT_DANO_SWAPPED)
		reader.SetSwap(true);

	ssize_t size;
	reader(size);

	dano_message_header header;
	reader(header);
	into->what = header.what;

	size -= sizeof(dano_section_header) + sizeof(dano_message_header);
	int32 offset = 0;

	while (offset < size) {
		dano_section_header sectionHeader;
		reader(sectionHeader);

		// be safe. this shouldn't be necessary but in some testcases it was.
		sectionHeader.size = pad_to_8(sectionHeader.size);

		if (offset + sectionHeader.size > size || sectionHeader.size < 0)
			return B_BAD_DATA;

		ssize_t fieldSize = sectionHeader.size - sizeof(dano_section_header);
		uint8 *fieldBuffer = NULL;
		if (fieldSize <= 0) {
			// there may be no data. we shouldn't fail because of that
			offset += sectionHeader.size;
			continue;
		}

		fieldBuffer = (uint8 *)malloc(fieldSize);
		if (fieldBuffer == NULL)
			throw (status_t)B_NO_MEMORY;

		reader(fieldBuffer, fieldSize);

		switch (sectionHeader.code) {
			case SECTION_OFFSET_TABLE:
			case SECTION_TARGET_INFORMATION:
			case SECTION_SORTED_INDEX_TABLE:
			case SECTION_END_OF_DATA:
				// discard
				break;

			case SECTION_SINGLE_ITEM_DATA: {
				dano_single_item *field = (dano_single_item *)fieldBuffer;

				int32 dataOffset = sizeof(dano_single_item)
					+ field->name_length + 1;
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

				status_t result = into->AddData(field->name, field->type,
					fieldBuffer + dataOffset, field->item_size, fixedSize);

				if (result < B_OK) {
					free(fieldBuffer);
					throw result;
				}
				break;
			}

			case SECTION_FIXED_SIZE_ARRAY_DATA: {
				dano_fixed_size_array *field
					= (dano_fixed_size_array *)fieldBuffer;

				int32 dataOffset = sizeof(dano_fixed_size_array)
					+ field->name_length + 1;
				dataOffset = pad_to_8(dataOffset);
				int32 count = *(int32 *)(fieldBuffer + dataOffset);
				dataOffset += 8; /* count and padding */

				if (offset + dataOffset + count * field->size_per_item > size)
					return B_BAD_DATA;

				status_t result = B_OK;
				for (int32 i = 0; i < count; i++) {
					result = into->AddData(field->name, field->type,
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
				dano_variable_size_array *field
					= (dano_variable_size_array *)fieldBuffer;

				int32 dataOffset = sizeof(dano_variable_size_array)
					+ field->name_length + 1;
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

					result = into->AddData(field->name, field->type,
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

} // namespace BPrivate
