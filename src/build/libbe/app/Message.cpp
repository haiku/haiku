/*
 * Copyright 2005-2008, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <Message.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <BlockCache.h>
#include <Entry.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>

#include <AppMisc.h>
#include <MessageAdapter.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include <TokenSpace.h>


const char *B_SPECIFIER_ENTRY = "specifiers";
const char *B_PROPERTY_ENTRY = "property";
const char *B_PROPERTY_NAME_ENTRY = "name";


BBlockCache *BMessage::sMsgCache = NULL;


BMessage::BMessage()
{
	_InitCommon();
}


BMessage::BMessage(BMessage *other)
{
	_InitCommon();
	*this = *other;
}


BMessage::BMessage(uint32 _what)
{
	_InitCommon();
	fHeader->what = what = _what;
}


BMessage::BMessage(const BMessage &other)
{
	_InitCommon();
	*this = other;
}


BMessage::~BMessage()
{
	_Clear();
}


BMessage &
BMessage::operator=(const BMessage &other)
{
	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	memcpy(fHeader, other.fHeader, sizeof(message_header));

	// Clear some header flags inherited from the original message that don't
	// apply to the clone.
	fHeader->flags &= ~(MESSAGE_FLAG_REPLY_REQUIRED | MESSAGE_FLAG_REPLY_DONE
		| MESSAGE_FLAG_IS_REPLY | MESSAGE_FLAG_WAS_DELIVERED
		| MESSAGE_FLAG_WAS_DROPPED | MESSAGE_FLAG_PASS_BY_AREA);
	// Note, that BeOS R5 seems to keep the reply info.

	if (fHeader->fields_size > 0) {
		fFields = (field_header *)malloc(fHeader->fields_size);
		memcpy(fFields, other.fFields, fHeader->fields_size);
	}

	if (fHeader->data_size > 0) {
		fData = (uint8 *)malloc(fHeader->data_size);
		memcpy(fData, other.fData, fHeader->data_size);
	}

	fHeader->shared_area = -1;
	fHeader->fields_available = 0;
	fHeader->data_available = 0;
	fHeader->what = what = other.what;

	return *this;
}


void *
BMessage::operator new(size_t size)
{
	if (!sMsgCache)
		sMsgCache = new BBlockCache(10, sizeof(BMessage), B_OBJECT_CACHE);

	return sMsgCache->Get(size);
}


void *
BMessage::operator new(size_t, void *pointer)
{
	return pointer;
}


void
BMessage::operator delete(void *pointer, size_t size)
{
	sMsgCache->Save(pointer, size);
}


status_t
BMessage::_InitCommon()
{
	what = 0;

	fHeader = NULL;
	fFields = NULL;
	fData = NULL;

	fOriginal = NULL;
	fQueueLink = NULL;

	return _InitHeader();
}


status_t
BMessage::_InitHeader()
{
	fHeader = (message_header *)malloc(sizeof(message_header));
	memset(fHeader, 0, sizeof(message_header) - sizeof(fHeader->hash_table));

	fHeader->format = MESSAGE_FORMAT_HAIKU;
	fHeader->flags = MESSAGE_FLAG_VALID;
	fHeader->what = what;
	fHeader->current_specifier = -1;
	fHeader->shared_area = -1;

	fHeader->target = B_NULL_TOKEN;
	fHeader->reply_target = B_NULL_TOKEN;
	fHeader->reply_port = -1;
	fHeader->reply_team = -1;

	// initializing the hash table to -1 because 0 is a valid index
	fHeader->hash_table_size = MESSAGE_BODY_HASH_TABLE_SIZE;
	memset(&fHeader->hash_table, 255, sizeof(fHeader->hash_table));
	return B_OK;
}


status_t
BMessage::_Clear()
{
	free(fHeader);
	fHeader = NULL;
	free(fFields);
	fFields = NULL;
	free(fData);
	fData = NULL;

	delete fOriginal;
	fOriginal = NULL;

	return B_OK;
}


status_t
BMessage::GetInfo(type_code typeRequested, int32 index, char **nameFound,
	type_code *typeFound, int32 *countFound) const
{
	if (typeRequested == B_ANY_TYPE) {
		if (index >= fHeader->field_count)
			return B_BAD_INDEX;

		if (nameFound)
			*nameFound = (char *)fData + fFields[index].offset;
		if (typeFound)
			*typeFound = fFields[index].type;
		if (countFound)
			*countFound = fFields[index].count;
		return B_OK;
	}

	int32 counter = -1;
	field_header *field = fFields;
	for (int32 i = 0; i < fHeader->field_count; i++, field++) {
		if (field->type == typeRequested)
			counter++;

		if (counter == index) {
			if (nameFound)
				*nameFound = (char *)fData + field->offset;
			if (typeFound)
				*typeFound = field->type;
			if (countFound)
				*countFound = field->count;
			return B_OK;
		}
	}

	if (counter == -1)
		return B_BAD_TYPE;

	return B_BAD_INDEX;
}


status_t
BMessage::GetInfo(const char *name, type_code *typeFound, int32 *countFound)
	const
{
	if (countFound)
		*countFound = 0;

	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || !field)
		return result;

	if (typeFound)
		*typeFound = field->type;
	if (countFound)
		*countFound = field->count;

	return B_OK;
}


status_t
BMessage::GetInfo(const char *name, type_code *typeFound, bool *fixedSize)
	const
{
	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || !field)
		return result;

	if (typeFound)
		*typeFound = field->type;
	if (fixedSize)
		*fixedSize = field->flags & FIELD_FLAG_FIXED_SIZE;

	return B_OK;
}


int32
BMessage::CountNames(type_code type) const
{
	if (type == B_ANY_TYPE)
		return fHeader->field_count;

	int32 count = 0;
	field_header *field = fFields;
	for (int32 i = 0; i < fHeader->field_count; i++, field++) {
		if (field->type == type)
			count++;
	}

	return count;
}


bool
BMessage::IsEmpty() const
{
	return fHeader->field_count == 0;
}


bool
BMessage::IsSystem() const
{
	char a = char(what >> 24);
	char b = char(what >> 16);
	char c = char(what >> 8);
	char d = char(what);

	// The BeBook says:
	//		... we've adopted a strict convention for assigning values to all
	//		Be-defined constants.  The value assigned will always be formed by
	//		combining four characters into a multicharacter constant, with the
	//		characters limited to uppercase letters and the underbar
	// Between that and what's in AppDefs.h, this algo seems like a safe bet:
	if (a == '_' && isupper(b) && isupper(c) && isupper(d))
		return true;

	return false;
}


bool
BMessage::IsReply() const
{
	return fHeader->flags & MESSAGE_FLAG_IS_REPLY;
}


template<typename Type> static uint8 *
print_to_stream_type(uint8* pointer)
{
	Type *item = (Type *)pointer;
	item->PrintToStream();
	return (uint8 *)(item+1);
}


template<typename Type> static uint8 *
print_type(const char* format, uint8* pointer)
{
	Type *item = (Type *)pointer;
	printf(format, *item, *item);
	return (uint8 *)(item+1);
}


void
BMessage::PrintToStream() const
{
	_PrintToStream("");
	printf("}\n");
}


void
BMessage::_PrintToStream(const char* indent) const
{
	int32 value = B_BENDIAN_TO_HOST_INT32(what);
	printf("BMessage(");
	if (isprint(*(char *)&value)) {
		printf("'%.4s'", (char *)&value);
	} else
		printf("0x%lx", what);
	printf(") {\n");

	field_header *field = fFields;
	for (int32 i = 0; i < fHeader->field_count; i++, field++) {
		value = B_BENDIAN_TO_HOST_INT32(field->type);
		ssize_t size = 0;
		if ((field->flags & FIELD_FLAG_FIXED_SIZE) && field->count > 0)
			size = field->data_size / field->count;

		uint8 *pointer = fData + field->offset + field->name_length;
		for (int32 j = 0; j < field->count; j++) {
			if (field->count == 1) {
				printf("%s        %s = ", indent,
					(char *)(fData + field->offset));
			} else {
				printf("%s        %s[%ld] = ", indent,
					(char *)(fData + field->offset), j);
			}

			switch (field->type) {
				case B_RECT_TYPE:
					pointer = print_to_stream_type<BRect>(pointer);
					break;

				case B_POINT_TYPE:
					pointer = print_to_stream_type<BPoint>(pointer);
					break;

				case B_STRING_TYPE:	{
					ssize_t size = *(ssize_t *)pointer;
					pointer += sizeof(ssize_t);
					printf("string(\"%s\", %ld bytes)\n", (char *)pointer,
						(long)size);
					pointer += size;
					break;
				}

				case B_INT8_TYPE:
					pointer = print_type<int8>("int8(0x%hx or %d or \'%.1s\')\n", pointer);
					break;

				case B_INT16_TYPE:
					pointer = print_type<int16>("int16 (0x%x or %d)\n", pointer);
					break;

				case B_INT32_TYPE:
					pointer = print_type<int32>("int32(0x%lx or %ld)\n", pointer);
					break;

				case B_INT64_TYPE:
					pointer = print_type<int64>("int64(0x%Lx or %Ld)\n", pointer);
					break;

				case B_BOOL_TYPE:
					printf("bool(%s)\n", *((bool *)pointer)!= 0 ? "true" : "false");
					pointer += sizeof(bool);
					break;

				case B_FLOAT_TYPE:
					pointer = print_type<float>("float(%.4f)\n", pointer);
					break;

				case B_DOUBLE_TYPE:
					pointer = print_type<double>("double(%.8f)\n", pointer);
					break;

				case B_REF_TYPE: {
					ssize_t size = *(ssize_t *)pointer;
					pointer += sizeof(ssize_t);
					entry_ref ref;
					BPrivate::entry_ref_unflatten(&ref, (char *)pointer, size);

					printf("entry_ref(device=%ld, directory=%lld, name=\"%s\", ",
						(long)ref.device, ref.directory, ref.name);

					BPath path(&ref);
					printf("path=\"%s\")\n", path.Path());
					pointer += size;
					break;
				}

				case B_MESSAGE_TYPE:
				{
					char buffer[1024];
					sprintf(buffer, "%s        ", indent);

					BMessage message;
					const ssize_t size = *(const ssize_t *)pointer;
					pointer += sizeof(ssize_t);
					if (message.Unflatten((const char *)pointer) != B_OK) {
						fprintf(stderr, "couldn't unflatten item %ld\n", i);
						break;
					}
					message._PrintToStream(buffer);
					printf("%s        }\n", indent);
					pointer += size;
					break;
				}

				default:
					printf("(type = '%.4s')(size = %ld)\n", (char *)&value,
						(long)size);
					break;
			}
		}
	}
}


status_t
BMessage::Rename(const char *oldEntry, const char *newEntry)
{
	if (!oldEntry || !newEntry)
		return B_BAD_VALUE;

	uint32 hash = _HashName(oldEntry) % fHeader->hash_table_size;
	int32 *nextField = &fHeader->hash_table[hash];

	while (*nextField >= 0) {
		field_header *field = &fFields[*nextField];

		if (strncmp((const char *)(fData + field->offset), oldEntry,
			field->name_length) == 0) {
			// nextField points to the field for oldEntry, save it and unlink
			int32 index = *nextField;
			*nextField = field->next_field;
			field->next_field = -1;

			hash = _HashName(newEntry) % fHeader->hash_table_size;
			nextField = &fHeader->hash_table[hash];
			while (*nextField >= 0)
				nextField = &fFields[*nextField].next_field;
			*nextField = index;

			int32 newLength = strlen(newEntry) + 1;
			status_t result = _ResizeData(field->offset + 1,
				newLength - field->name_length);
			if (result < B_OK)
				return result;

			memcpy(fData + field->offset, newEntry, newLength);
			field->name_length = newLength;
			return B_OK;
		}

		nextField = &field->next_field;
	}

	return B_NAME_NOT_FOUND;
}


bool
BMessage::WasDelivered() const
{
	return fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED;
}


bool
BMessage::IsSourceWaiting() const
{
	return (fHeader->flags & MESSAGE_FLAG_REPLY_REQUIRED)
		&& !(fHeader->flags & MESSAGE_FLAG_REPLY_DONE);
}


BMessenger
BMessage::ReturnAddress() const
{
	return BMessenger();
}


const BMessage *
BMessage::Previous() const
{
	/* ToDo: test if the "_previous_" field is used in R5 */
	if (!fOriginal) {
		fOriginal = new BMessage();

		if (FindMessage("_previous_", fOriginal) != B_OK) {
			delete fOriginal;
			fOriginal = NULL;
		}
	}

	return fOriginal;
}


bool
BMessage::WasDropped() const
{
	return fHeader->flags & MESSAGE_FLAG_WAS_DROPPED;
}


BPoint
BMessage::DropPoint(BPoint *offset) const
{
	if (offset)
		*offset = FindPoint("_drop_offset_");

	return FindPoint("_drop_point_");
}


ssize_t
BMessage::FlattenedSize() const
{
	return BPrivate::MessageAdapter::FlattenedSize(MESSAGE_FORMAT_R5, this);
}


status_t
BMessage::Flatten(char *buffer, ssize_t size) const
{
	return BPrivate::MessageAdapter::Flatten(MESSAGE_FORMAT_R5, this, buffer,
		&size);
}


status_t
BMessage::Flatten(BDataIO *stream, ssize_t *size) const
{
	return BPrivate::MessageAdapter::Flatten(MESSAGE_FORMAT_R5, this, stream,
		size);
}


ssize_t
BMessage::_NativeFlattenedSize() const
{
	return sizeof(message_header) + fHeader->fields_size + fHeader->data_size;
}


status_t
BMessage::_NativeFlatten(char *buffer, ssize_t size) const
{
	if (!buffer)
		return B_BAD_VALUE;

	if (!fHeader)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	memcpy(buffer, fHeader, min_c(sizeof(message_header), (size_t)size));
	buffer += sizeof(message_header);
	size -= sizeof(message_header);

	memcpy(buffer, fFields, min_c(fHeader->fields_size, size));
	buffer += fHeader->fields_size;
	size -= fHeader->fields_size;

	memcpy(buffer, fData, min_c(fHeader->data_size, size));
	if (size >= fHeader->data_size)
		return B_OK;

	return B_NO_MEMORY;
}


status_t
BMessage::_NativeFlatten(BDataIO *stream, ssize_t *size) const
{
	if (!stream)
		return B_BAD_VALUE;

	if (!fHeader)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	ssize_t result1 = stream->Write(fHeader, sizeof(message_header));
	if (result1 != sizeof(message_header))
		return (result1 >= 0 ? B_ERROR : result1);

	ssize_t result2 = 0;
	if (fHeader->fields_size > 0) {
		result2 = stream->Write(fFields, fHeader->fields_size);
		if (result2 != fHeader->fields_size)
			return (result2 >= 0 ? B_ERROR : result2);
	}

	ssize_t result3 = 0;
	if (fHeader->data_size > 0) {
		result3 = stream->Write(fData, fHeader->data_size);
		if (result3 != fHeader->data_size)
			return (result3 >= 0 ? B_ERROR : result3);
	}

	if (size)
		*size = result1 + result2 + result3;

	return B_OK;
}


status_t
BMessage::Unflatten(const char *flatBuffer)
{
	if (!flatBuffer)
		return B_BAD_VALUE;

	uint32 format = *(uint32 *)flatBuffer;
	if (format != MESSAGE_FORMAT_HAIKU)
		return BPrivate::MessageAdapter::Unflatten(format, this, flatBuffer);

	// native message unflattening

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (!fHeader)
		return B_NO_MEMORY;

	memcpy(fHeader, flatBuffer, sizeof(message_header));
	flatBuffer += sizeof(message_header);

	if (fHeader->format != MESSAGE_FORMAT_HAIKU
		|| !(fHeader->flags & MESSAGE_FLAG_VALID)) {
		free(fHeader);
		fHeader = NULL;
		_InitHeader();
		return B_BAD_VALUE;
	}

	fHeader->fields_available = 0;
	fHeader->data_available = 0;
	what = fHeader->what;

	fHeader->shared_area = -1;

	if (fHeader->fields_size > 0) {
		fFields = (field_header *)malloc(fHeader->fields_size);
		if (!fFields)
			return B_NO_MEMORY;

		memcpy(fFields, flatBuffer, fHeader->fields_size);
		flatBuffer += fHeader->fields_size;
	}

	if (fHeader->data_size > 0) {
		fData = (uint8 *)malloc(fHeader->data_size);
		if (!fData)
			return B_NO_MEMORY;

		memcpy(fData, flatBuffer, fHeader->data_size);
	}

	return B_OK;
}


status_t
BMessage::Unflatten(BDataIO *stream)
{
	if (!stream)
		return B_BAD_VALUE;

	uint32 format = 0;
	stream->Read(&format, sizeof(uint32));
	if (format != MESSAGE_FORMAT_HAIKU)
		return BPrivate::MessageAdapter::Unflatten(format, this, stream);

	// native message unflattening

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (!fHeader)
		return B_NO_MEMORY;

	fHeader->format = format;
	uint8 *header = (uint8 *)fHeader;
	ssize_t result = stream->Read(header + sizeof(uint32),
		sizeof(message_header) - sizeof(uint32));
	result -= sizeof(message_header) - sizeof(uint32);

	if (result != B_OK || fHeader->format != MESSAGE_FORMAT_HAIKU
		|| !(fHeader->flags & MESSAGE_FLAG_VALID)) {
		free(fHeader);
		fHeader = NULL;
		_InitHeader();
		return B_BAD_VALUE;
	}

	fHeader->fields_available = 0;
	fHeader->data_available = 0;
	what = fHeader->what;

	fHeader->shared_area = -1;

	if (result == B_OK && fHeader->fields_size > 0) {
		fFields = (field_header *)malloc(fHeader->fields_size);
		if (!fFields)
			return B_NO_MEMORY;

		result = stream->Read(fFields, fHeader->fields_size);
		result -= fHeader->fields_size;
	}

	if (result == B_OK && fHeader->data_size > 0) {
		fData = (uint8 *)malloc(fHeader->data_size);
		if (!fData)
			return B_NO_MEMORY;

		result = stream->Read(fData, fHeader->data_size);
		result -= fHeader->data_size;
	}

	if (result < B_OK)
		return B_BAD_VALUE;

	return B_OK;
}


status_t
BMessage::AddSpecifier(const char *property)
{
	BMessage message(B_DIRECT_SPECIFIER);
	status_t result = message.AddString(B_PROPERTY_ENTRY, property);
	if (result < B_OK)
		return result;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, int32 index)
{
	BMessage message(B_INDEX_SPECIFIER);
	status_t result = message.AddString(B_PROPERTY_ENTRY, property);
	if (result < B_OK)
		return result;

	result = message.AddInt32("index", index);
	if (result < B_OK)
		return result;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, int32 index, int32 range)
{
	if (range < 0)
		return B_BAD_VALUE;

	BMessage message(B_RANGE_SPECIFIER);
	status_t result = message.AddString(B_PROPERTY_ENTRY, property);
	if (result < B_OK)
		return result;

	result = message.AddInt32("index", index);
	if (result < B_OK)
		return result;

	result = message.AddInt32("range", range);
	if (result < B_OK)
		return result;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, const char *name)
{
	BMessage message(B_NAME_SPECIFIER);
	status_t result = message.AddString(B_PROPERTY_ENTRY, property);
	if (result < B_OK)
		return result;

	result = message.AddString(B_PROPERTY_NAME_ENTRY, name);
	if (result < B_OK)
		return result;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const BMessage *specifier)
{
	status_t result = AddMessage(B_SPECIFIER_ENTRY, specifier);
	if (result < B_OK)
		return result;

	fHeader->current_specifier++;
	fHeader->flags |= MESSAGE_FLAG_HAS_SPECIFIERS;
	return B_OK;
}


status_t
BMessage::SetCurrentSpecifier(int32 index)
{
	if (index < 0)
		return B_BAD_INDEX;

	type_code type;
	int32 count;
	status_t result = GetInfo(B_SPECIFIER_ENTRY, &type, &count);
	if (result < B_OK)
		return result;

	if (index > count)
		return B_BAD_INDEX;

	fHeader->current_specifier = index;
	return B_OK;
}


status_t
BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier, int32 *what,
	const char **property) const
{
	if (index)
		*index = fHeader->current_specifier;

	if (fHeader->current_specifier < 0
		|| !(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_SCRIPT_SYNTAX;

	if (specifier) {
		if (FindMessage(B_SPECIFIER_ENTRY, fHeader->current_specifier,
			specifier) < B_OK)
			return B_BAD_SCRIPT_SYNTAX;

		if (what)
			*what = specifier->what;

		if (property) {
			if (specifier->FindString(B_PROPERTY_ENTRY, property) < B_OK)
				return B_BAD_SCRIPT_SYNTAX;
		}
	}

	return B_OK;
}


bool
BMessage::HasSpecifiers() const
{
	return fHeader->flags & MESSAGE_FLAG_HAS_SPECIFIERS;
}


status_t
BMessage::PopSpecifier()
{
	if (fHeader->current_specifier < 0 ||
		!(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_VALUE;

	if (fHeader->current_specifier >= 0)
		fHeader->current_specifier--;

	return B_OK;
}


status_t
BMessage::_ResizeData(int32 offset, int32 change)
{
	if (change == 0)
		return B_OK;

	/* optimize for the most usual case: appending data */
	if (offset < fHeader->data_size) {
		field_header *field = fFields;
		for (int32 i = 0; i < fHeader->field_count; i++, field++) {
			if (field->offset >= offset)
				field->offset += change;
		}
	}

	if (change > 0) {
		if (fHeader->data_available >= change) {
			if (offset < fHeader->data_size) {
				memmove(fData + offset + change, fData + offset,
					fHeader->data_size - offset);
			}

			fHeader->data_available -= change;
			fHeader->data_size += change;
			return B_OK;
		}

		ssize_t size = fHeader->data_size * 2;
		size = min_c(size, fHeader->data_size + MAX_DATA_PREALLOCATION);
		size = max_c(size, fHeader->data_size + change);

		uint8 *newData = (uint8 *)realloc(fData, size);
		if (size > 0 && !newData)
			return B_NO_MEMORY;

		fData = newData;
		if (offset < fHeader->data_size) {
			memmove(fData + offset + change, fData + offset,
				fHeader->data_size - offset);
		}

		fHeader->data_size += change;
		fHeader->data_available = size - fHeader->data_size;
	} else {
		ssize_t length = fHeader->data_size - offset + change;
		if (length > 0)
			memmove(fData + offset, fData + offset - change, length);

		fHeader->data_size += change;
		fHeader->data_available -= change;

		if (fHeader->data_available > MAX_DATA_PREALLOCATION) {
			ssize_t available = MAX_DATA_PREALLOCATION / 2;
			ssize_t size = fHeader->data_size + available;
			uint8 *newData = (uint8 *)realloc(fData, size);
			if (size > 0 && !newData) {
				// this is strange, but not really fatal
				return B_OK;
			}

			fData = newData;
			fHeader->data_available = available;
		}
	}

	return B_OK;
}


uint32
BMessage::_HashName(const char *name) const
{
	char ch;
	uint32 result = 0;

	while ((ch = *name++) != 0) {
		result = (result << 7) ^ (result >> 24);
		result ^= ch;
	}

	result ^= result << 12;
	return result;
}


status_t
BMessage::_FindField(const char *name, type_code type, field_header **result) const
{
	if (!name)
		return B_BAD_VALUE;

	if (!fHeader || !fFields || !fData)
		return B_NAME_NOT_FOUND;

	uint32 hash = _HashName(name) % fHeader->hash_table_size;
	int32 nextField = fHeader->hash_table[hash];

	while (nextField >= 0) {
		field_header *field = &fFields[nextField];

		if (strncmp((const char *)(fData + field->offset), name,
			field->name_length) == 0) {
			if (type != B_ANY_TYPE && field->type != type)
				return B_BAD_TYPE;

			*result = field;
			return B_OK;
		}

		nextField = field->next_field;
	}

	return B_NAME_NOT_FOUND;
}


status_t
BMessage::_AddField(const char *name, type_code type, bool isFixedSize,
	field_header **result)
{
	if (!fHeader)
		return B_ERROR;

	if (fHeader->fields_available <= 0) {
		int32 count = fHeader->field_count * 2 + 1;
		count = min_c(count, fHeader->field_count + MAX_FIELD_PREALLOCATION);

		field_header *newFields = (field_header *)realloc(fFields,
			count * sizeof(field_header));
		if (count > 0 && !newFields)
			return B_NO_MEMORY;

		fFields = newFields;
		fHeader->fields_available = count - fHeader->field_count;
	}

	uint32 hash = _HashName(name) % fHeader->hash_table_size;
	int32 *nextField = &fHeader->hash_table[hash];
	while (*nextField >= 0)
		nextField = &fFields[*nextField].next_field;
	*nextField = fHeader->field_count;

	field_header *field = &fFields[fHeader->field_count];
	field->type = type;
	field->count = 0;
	field->data_size = 0;
	field->allocated = 0;
	field->next_field = -1;
	field->offset = fHeader->data_size;
	field->name_length = strlen(name) + 1;
	status_t status = _ResizeData(field->offset, field->name_length);
	if (status < B_OK)
		return status;

	memcpy(fData + field->offset, name, field->name_length);
	field->flags = FIELD_FLAG_VALID;
	if (isFixedSize)
		field->flags |= FIELD_FLAG_FIXED_SIZE;

	fHeader->fields_available--;
	fHeader->fields_size += sizeof(field_header);
	fHeader->field_count++;
	*result = field;
	return B_OK;
}


status_t
BMessage::_RemoveField(field_header *field)
{
	status_t result = _ResizeData(field->offset, -(field->data_size
		+ field->name_length));
	if (result < B_OK)
		return result;

	int32 index = ((uint8 *)field - (uint8 *)fFields) / sizeof(field_header);
	int32 nextField = field->next_field;
	if (nextField > index)
		nextField--;

	int32 *value = fHeader->hash_table;
	for (int32 i = 0; i < fHeader->hash_table_size; i++, value++) {
		if (*value > index)
			*value -= 1;
		else if (*value == index)
			*value = nextField;
	}

	field_header *other = fFields;
	for (int32 i = 0; i < fHeader->field_count; i++, other++) {
		if (other->next_field > index)
			other->next_field--;
		else if (other->next_field == index)
			other->next_field = nextField;
	}

	ssize_t size = fHeader->fields_size - (index + 1) * sizeof(field_header);
	memmove(fFields + index, fFields + index + 1, size);
	fHeader->fields_size -= sizeof(field_header);
	fHeader->field_count--;
	fHeader->fields_available++;

	if (fHeader->fields_available > MAX_FIELD_PREALLOCATION) {
		ssize_t available = MAX_FIELD_PREALLOCATION / 2;
		size = fHeader->fields_size + available * sizeof(field_header);
		field_header *newFields = (field_header *)realloc(fFields, size);
		if (size > 0 && !newFields) {
			// this is strange, but not really fatal
			return B_OK;
		}

		fFields = newFields;
		fHeader->fields_available = available;
	}

	return B_OK;
}


status_t
BMessage::AddData(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool isFixedSize, int32 count)
{
	if (numBytes <= 0 || !data)
		return B_BAD_VALUE;

	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);
	if (result == B_NAME_NOT_FOUND)
		result = _AddField(name, type, isFixedSize, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	uint32 offset = field->offset + field->name_length + field->data_size;
	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		if (field->count) {
			ssize_t size = field->data_size / field->count;
			if (size != numBytes)
				return B_BAD_VALUE;
		}

		result = _ResizeData(offset, numBytes);
		if (result < B_OK) {
			if (field->count == 0)
				_RemoveField(field);
			return result;
		}

		memcpy(fData + offset, data, numBytes);
		field->data_size += numBytes;
	} else {
		int32 change = numBytes + sizeof(numBytes);
		result = _ResizeData(offset, change);
		if (result < B_OK) {
			if (field->count == 0)
				_RemoveField(field);
			return result;
		}

		memcpy(fData + offset, &numBytes, sizeof(numBytes));
		memcpy(fData + offset + sizeof(numBytes), data, numBytes);
		field->data_size += change;
	}

	field->count++;
	return B_OK;
}


status_t
BMessage::RemoveData(const char *name, int32 index)
{
	if (index < 0)
		return B_BAD_VALUE;

	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->count == 1)
		return _RemoveField(field);

	uint32 offset = field->offset + field->name_length;
	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		ssize_t size = field->data_size / field->count;
		result = _ResizeData(offset + index * size, -size);
		if (result < B_OK)
			return result;

		field->data_size -= size;
	} else {
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(ssize_t *)pointer + sizeof(ssize_t);
			pointer = fData + offset;
		}

		ssize_t currentSize = *(ssize_t *)pointer + sizeof(ssize_t);
		result = _ResizeData(offset, -currentSize);
		if (result < B_OK)
			return result;

		field->data_size -= currentSize;
	}

	field->count--;
	return B_OK;
}


status_t
BMessage::RemoveName(const char *name)
{
	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	return _RemoveField(field);
}


status_t
BMessage::MakeEmpty()
{
	_Clear();
	_InitHeader();
	return B_OK;
}


status_t
BMessage::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *numBytes) const
{
	if (!data || !numBytes)
		return B_BAD_VALUE;

	*data = NULL;
	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		*numBytes = field->data_size / field->count;
		*data = fData + field->offset + field->name_length + index * *numBytes;
	} else {
		uint8 *pointer = fData + field->offset + field->name_length;

		for (int32 i = 0; i < index; i++)
			pointer += *(ssize_t *)pointer + sizeof(ssize_t);

		*numBytes = *(ssize_t *)pointer;
		*data = pointer + sizeof(ssize_t);
	}

	return B_OK;
}


status_t
BMessage::ReplaceData(const char *name, type_code type, int32 index,
	const void *data, ssize_t numBytes)
{
	if (numBytes <= 0 || !data)
		return B_BAD_VALUE;

	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		ssize_t size = field->data_size / field->count;
		if (size != numBytes)
			return B_BAD_VALUE;

		memcpy(fData + field->offset + field->name_length + index * size, data,
			size);
	} else {
		uint32 offset = field->offset + field->name_length;
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(ssize_t *)pointer + sizeof(ssize_t);
			pointer = fData + offset;
		}

		ssize_t currentSize = *(ssize_t *)pointer;
		int32 change = numBytes - currentSize;
		result = _ResizeData(offset, change);
		if (result < B_OK)
			return result;

		memcpy(fData + offset, &numBytes, sizeof(numBytes));
		memcpy(fData + offset + sizeof(numBytes), data, numBytes);
		field->data_size += change;
	}

	return B_OK;
}


bool
BMessage::HasData(const char *name, type_code type, int32 index) const
{
	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return false;

	if (!field)
		return false;

	if (index >= field->count)
		return false;

	return true;
}


void
BMessage::_StaticCacheCleanup()
{
	delete sMsgCache;
	sMsgCache = NULL;
}


/* Relay functions from here on (Add... -> AddData, Find... -> FindData) */

#define DEFINE_FUNCTIONS(type, typeName, typeCode)							\
status_t																	\
BMessage::Add##typeName(const char *name, type val)							\
{																			\
	return AddData(name, typeCode, &val, sizeof(type), true);				\
}																			\
																			\
status_t																	\
BMessage::Find##typeName(const char *name, type *p) const					\
{																			\
	void *ptr = NULL;														\
	ssize_t bytes = 0;														\
	status_t error = B_OK;													\
																			\
	*p = type();															\
	error = FindData(name, typeCode, 0, (const void **)&ptr, &bytes);		\
																			\
	if (error == B_OK)														\
		memcpy(p, ptr, sizeof(type));										\
																			\
	return error;															\
}																			\
																			\
status_t																	\
BMessage::Find##typeName(const char *name, int32 index, type *p) const		\
{																			\
	void *ptr = NULL;														\
	ssize_t bytes = 0;														\
	status_t error = B_OK;													\
																			\
	*p = type();															\
	error = FindData(name, typeCode, index, (const void **)&ptr, &bytes);	\
																			\
	if (error == B_OK)														\
		memcpy(p, ptr, sizeof(type));										\
																			\
	return error;															\
}																			\
																			\
status_t																	\
BMessage::Replace##typeName(const char *name, type val)						\
{																			\
	return ReplaceData(name, typeCode, 0, &val, sizeof(type));				\
}																			\
																			\
status_t																	\
BMessage::Replace##typeName(const char *name, int32 index, type val)		\
{																			\
	return ReplaceData(name, typeCode, index, &val, sizeof(type));			\
}																			\
																			\
bool																		\
BMessage::Has##typeName(const char *name, int32 index) const				\
{																			\
	return HasData(name, typeCode, index);									\
}

DEFINE_FUNCTIONS(BPoint, Point, B_POINT_TYPE);
DEFINE_FUNCTIONS(BRect, Rect, B_RECT_TYPE);
DEFINE_FUNCTIONS(int8, Int8, B_INT8_TYPE);
DEFINE_FUNCTIONS(int16, Int16, B_INT16_TYPE);
DEFINE_FUNCTIONS(int32, Int32, B_INT32_TYPE);
DEFINE_FUNCTIONS(int64, Int64, B_INT64_TYPE);
DEFINE_FUNCTIONS(bool, Bool, B_BOOL_TYPE);
DEFINE_FUNCTIONS(float, Float, B_FLOAT_TYPE);
DEFINE_FUNCTIONS(double, Double, B_DOUBLE_TYPE);

#undef DEFINE_FUNCTIONS

#define DEFINE_HAS_FUNCTION(typeName, typeCode)								\
bool																		\
BMessage::Has##typeName(const char *name, int32 index) const				\
{																			\
	return HasData(name, typeCode, index);									\
}

DEFINE_HAS_FUNCTION(String, B_STRING_TYPE);
DEFINE_HAS_FUNCTION(Pointer, B_POINTER_TYPE);
DEFINE_HAS_FUNCTION(Messenger, B_MESSENGER_TYPE);
DEFINE_HAS_FUNCTION(Ref, B_REF_TYPE);
DEFINE_HAS_FUNCTION(Message, B_MESSAGE_TYPE);

#undef DEFINE_HAS_FUNCTION

#define DEFINE_LAZY_FIND_FUNCTION(type, typeName, initialize)				\
type																		\
BMessage::Find##typeName(const char *name, int32 index) const				\
{																			\
	type val = initialize;													\
	Find##typeName(name, index, &val);										\
	return val;																\
}

DEFINE_LAZY_FIND_FUNCTION(BRect, Rect, BRect());
DEFINE_LAZY_FIND_FUNCTION(BPoint, Point, BPoint());
DEFINE_LAZY_FIND_FUNCTION(const char *, String, NULL);
DEFINE_LAZY_FIND_FUNCTION(int8, Int8, 0);
DEFINE_LAZY_FIND_FUNCTION(int16, Int16, 0);
DEFINE_LAZY_FIND_FUNCTION(int32, Int32, 0);
DEFINE_LAZY_FIND_FUNCTION(int64, Int64, 0);
DEFINE_LAZY_FIND_FUNCTION(bool, Bool, false);
DEFINE_LAZY_FIND_FUNCTION(float, Float, 0);
DEFINE_LAZY_FIND_FUNCTION(double, Double, 0);

#undef DEFINE_LAZY_FIND_FUNCTION

status_t
BMessage::AddString(const char *name, const char *string)
{
	return AddData(name, B_STRING_TYPE, string, string ? strlen(string) + 1 : 0, false);
}


status_t
BMessage::AddString(const char *name, const BString &string)
{
	return AddData(name, B_STRING_TYPE, string.String(), string.Length() + 1, false);
}


status_t
BMessage::AddPointer(const char *name, const void *pointer)
{
	return AddData(name, B_POINTER_TYPE, &pointer, sizeof(pointer), true);
}


status_t
BMessage::AddMessenger(const char *name, BMessenger messenger)
{
	return AddData(name, B_MESSENGER_TYPE, &messenger, sizeof(messenger), true);
}


status_t
BMessage::AddRef(const char *name, const entry_ref *ref)
{
	size_t size = sizeof(entry_ref) + B_PATH_NAME_LENGTH;
	char buffer[size];

	status_t error = BPrivate::entry_ref_flatten(buffer, &size, ref);

	if (error >= B_OK)
		error = AddData(name, B_REF_TYPE, buffer, size, false);

	return error;
}


status_t
BMessage::AddMessage(const char *name, const BMessage *message)
{
	/* ToDo: This and the following functions waste time by allocating and
	copying an extra buffer. Functions can be added that return a direct
	pointer into the message. */

	ssize_t size = message->FlattenedSize();
	char buffer[size];

	status_t error = message->Flatten(buffer, size);

	if (error >= B_OK)
		error = AddData(name, B_MESSAGE_TYPE, &buffer, size, false);

	return error;
}


status_t
BMessage::AddFlat(const char *name, BFlattenable *object, int32 count)
{
	ssize_t size = object->FlattenedSize();
	char buffer[size];

	status_t error = object->Flatten(buffer, size);

	if (error >= B_OK)
		error = AddData(name, object->TypeCode(), &buffer, size, false);

	return error;
}


status_t
BMessage::FindString(const char *name, const char **string) const
{
	return FindString(name, 0, string);
}


status_t
BMessage::FindString(const char *name, int32 index, const char **string) const
{
	ssize_t bytes;
	return FindData(name, B_STRING_TYPE, index, (const void **)string, &bytes);
}


status_t
BMessage::FindString(const char *name, BString *string) const
{
	return FindString(name, 0, string);
}


status_t
BMessage::FindString(const char *name, int32 index, BString *string) const
{
	const char *cstr;
	status_t error = FindString(name, index, &cstr);
	if (error < B_OK)
		return error;

	*string = cstr;
	return B_OK;
}


status_t
BMessage::FindPointer(const char *name, void **pointer) const
{
	return FindPointer(name, 0, pointer);
}


status_t
BMessage::FindPointer(const char *name, int32 index, void **pointer) const
{
	void **data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_POINTER_TYPE, index,
		(const void **)&data, &size);

	if (error == B_OK)
		*pointer = *data;
	else
		*pointer = NULL;

	return error;
}


status_t
BMessage::FindMessenger(const char *name, BMessenger *messenger) const
{
	return FindMessenger(name, 0, messenger);
}


status_t
BMessage::FindMessenger(const char *name, int32 index, BMessenger *messenger)
	const
{
	void *data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_MESSENGER_TYPE, index,
		(const void **)&data, &size);

	if (error == B_OK)
		memcpy(messenger, data, sizeof(BMessenger));
	else
		*messenger = BMessenger();

	return error;
}


status_t
BMessage::FindRef(const char *name, entry_ref *ref) const
{
	return FindRef(name, 0, ref);
}


status_t
BMessage::FindRef(const char *name, int32 index, entry_ref *ref) const
{
	void *data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_REF_TYPE, index,
		(const void **)&data, &size);

	if (error == B_OK)
		error = BPrivate::entry_ref_unflatten(ref, (char *)data, size);
	else
		*ref = entry_ref();

	return error;
}


status_t
BMessage::FindMessage(const char *name, BMessage *message) const
{
	return FindMessage(name, 0, message);
}


status_t
BMessage::FindMessage(const char *name, int32 index, BMessage *message) const
{
	void *data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_MESSAGE_TYPE, index,
		(const void **)&data, &size);

	if (error == B_OK)
		error = message->Unflatten((const char *)data);
	else
		*message = BMessage();

	return error;
}


status_t
BMessage::FindFlat(const char *name, BFlattenable *object) const
{
	return FindFlat(name, 0, object);
}


status_t
BMessage::FindFlat(const char *name, int32 index, BFlattenable *object) const
{
	void *data = NULL;
	ssize_t numBytes = 0;
	status_t error = FindData(name, object->TypeCode(), index,
		(const void **)&data, &numBytes);

	if (error == B_OK)
		error = object->Unflatten(object->TypeCode(), data, numBytes);

	return error;
}


status_t
BMessage::FindData(const char *name, type_code type, const void **data,
	ssize_t *numBytes) const
{
	return FindData(name, type, 0, data, numBytes);
}


status_t
BMessage::ReplaceString(const char *name, const char *string)
{
	return ReplaceData(name, B_STRING_TYPE, 0, string, strlen(string) + 1);
}


status_t
BMessage::ReplaceString(const char *name, int32 index, const char *string)
{
	return ReplaceData(name, B_STRING_TYPE, index, string, strlen(string) + 1);
}


status_t
BMessage::ReplaceString(const char *name, const BString &string)
{
	return ReplaceData(name, B_STRING_TYPE, 0, string.String(),
		string.Length() + 1);
}


status_t
BMessage::ReplaceString(const char *name, int32 index, const BString &string)
{
	return ReplaceData(name, B_STRING_TYPE, index, string.String(),
		string.Length() + 1);
}


status_t
BMessage::ReplacePointer(const char *name, const void *pointer)
{
	return ReplaceData(name, B_POINTER_TYPE, 0, &pointer, sizeof(pointer));
}


status_t
BMessage::ReplacePointer(const char *name, int32 index, const void *pointer)
{
	return ReplaceData(name, B_POINTER_TYPE, index, &pointer, sizeof(pointer));
}


status_t
BMessage::ReplaceMessenger(const char *name, BMessenger messenger)
{
	return ReplaceData(name, B_MESSENGER_TYPE, 0, &messenger,
		sizeof(BMessenger));
}


status_t
BMessage::ReplaceMessenger(const char *name, int32 index, BMessenger messenger)
{
	return ReplaceData(name, B_MESSENGER_TYPE, index, &messenger,
		sizeof(BMessenger));
}


status_t
BMessage::ReplaceRef(const char *name, const entry_ref *ref)
{
	return ReplaceRef(name, 0, ref);
}


status_t
BMessage::ReplaceRef(const char *name, int32 index, const entry_ref *ref)
{
	size_t size = sizeof(entry_ref) + B_PATH_NAME_LENGTH;
	char buffer[size];

	status_t error = BPrivate::entry_ref_flatten(buffer, &size, ref);

	if (error >= B_OK)
		error = ReplaceData(name, B_REF_TYPE, index, &buffer, size);

	return error;
}


status_t
BMessage::ReplaceMessage(const char *name, const BMessage *message)
{
	return ReplaceMessage(name, 0, message);
}


status_t
BMessage::ReplaceMessage(const char *name, int32 index, const BMessage *message)
{
	ssize_t size = message->FlattenedSize();
	char buffer[size];

	status_t error = message->Flatten(buffer, size);

	if (error >= B_OK)
		error = ReplaceData(name, B_MESSAGE_TYPE, index, &buffer, size);

	return error;
}


status_t
BMessage::ReplaceFlat(const char *name, BFlattenable *object)
{
	return ReplaceFlat(name, 0, object);
}


status_t
BMessage::ReplaceFlat(const char *name, int32 index, BFlattenable *object)
{
	ssize_t size = object->FlattenedSize();
	char buffer[size];

	status_t error = object->Flatten(buffer, size);

	if (error >= B_OK)
		error = ReplaceData(name, object->TypeCode(), index, &buffer, size);

	return error;
}


status_t
BMessage::ReplaceData(const char *name, type_code type, const void *data,
	ssize_t numBytes)
{
	return ReplaceData(name, type, 0, data, numBytes);
}


bool
BMessage::HasFlat(const char *name, const BFlattenable *object) const
{
	return HasFlat(name, 0, object);
}


bool
BMessage::HasFlat(const char *name, int32 index, const BFlattenable *object)
	const
{
	return HasData(name, object->TypeCode(), index);
}
