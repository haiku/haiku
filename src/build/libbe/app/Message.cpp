/*
 * Copyright 2005-2011, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */


#include <Message.h>
#include <MessageAdapter.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>

#include <MessengerPrivate.h>
#include <TokenSpace.h>

#include <Application.h>
#include <AppMisc.h>
#include <BlockCache.h>
#include <Entry.h>
#include <MessageQueue.h>
#include <Messenger.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "tracing_config.h"
	// kernel tracing configuration

//#define VERBOSE_DEBUG_OUTPUT
#ifdef VERBOSE_DEBUG_OUTPUT
#define DEBUG_FUNCTION_ENTER	\
	debug_printf("msg thread: %ld; this: %p; header: %p; fields: %p;" \
		" data: %p; what: 0x%08lx '%.4s'; line: %d; func: %s\n", \
		find_thread(NULL), this, fHeader, fFields, fData, what, (char *)&what, \
		__LINE__, __PRETTY_FUNCTION__);

#define DEBUG_FUNCTION_ENTER2	\
	debug_printf("msg thread: %ld; line: %d: func: %s\n", find_thread(NULL), \
		__LINE__, __PRETTY_FUNCTION__);
#else
#define DEBUG_FUNCTION_ENTER	/* nothing */
#define DEBUG_FUNCTION_ENTER2	/* nothing */
#endif

#if BMESSAGE_TRACING
#	define KTRACE(format...)	ktrace_printf(format)
#else
#	define KTRACE(format...)
#endif


const char *B_SPECIFIER_ENTRY = "specifiers";
const char *B_PROPERTY_ENTRY = "property";
const char *B_PROPERTY_NAME_ENTRY = "name";

extern "C" {
	// private os function to set the owning team of an area
	status_t _kern_transfer_area(area_id area, void **_address,
		uint32 addressSpec, team_id target);
}


BBlockCache *BMessage::sMsgCache = NULL;


template<typename Type>
static void
print_to_stream_type(uint8 *pointer)
{
	Type *item = (Type *)pointer;
	item->PrintToStream();
}


template<typename Type>
static void
print_type(const char *format, uint8 *pointer)
{
	Type *item = (Type *)pointer;
	printf(format, *item, *item);
}


//	#pragma mark -


BMessage::BMessage()
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon(true);
}


BMessage::BMessage(BMessage *other)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon(false);
	*this = *other;
}


BMessage::BMessage(uint32 _what)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon(true);
	fHeader->what = what = _what;
}


BMessage::BMessage(const BMessage &other)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon(false);
	*this = other;
}


BMessage::~BMessage()
{
	DEBUG_FUNCTION_ENTER;
	_Clear();
}


BMessage &
BMessage::operator=(const BMessage &other)
{
	DEBUG_FUNCTION_ENTER;

    if (this == &other)
        return *this;

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (fHeader == NULL)
		return *this;

	memcpy(fHeader, other.fHeader, sizeof(message_header));

	// Clear some header flags inherited from the original message that don't
	// apply to the clone.
	fHeader->flags &= ~(MESSAGE_FLAG_REPLY_REQUIRED | MESSAGE_FLAG_REPLY_DONE
		| MESSAGE_FLAG_IS_REPLY | MESSAGE_FLAG_WAS_DELIVERED
		| MESSAGE_FLAG_PASS_BY_AREA);
	// Note, that BeOS R5 seems to keep the reply info.

	if (fHeader->field_count > 0) {
		size_t fieldsSize = fHeader->field_count * sizeof(field_header);
		fFields = (field_header *)malloc(fieldsSize);
		if (fFields == NULL) {
			fHeader->field_count = 0;
			fHeader->data_size = 0;
		} else
			memcpy(fFields, other.fFields, fieldsSize);
	}

	if (fHeader->data_size > 0) {
		fData = (uint8 *)malloc(fHeader->data_size);
		if (fData == NULL) {
			fHeader->field_count = 0;
			free(fFields);
			fFields = NULL;
		} else
			memcpy(fData, other.fData, fHeader->data_size);
	}

	fHeader->what = what = other.what;
	fHeader->message_area = -1;
	fFieldsAvailable = 0;
	fDataAvailable = 0;

	return *this;
}


void *
BMessage::operator new(size_t size)
{
	DEBUG_FUNCTION_ENTER2;
	if (!sMsgCache)
		sMsgCache = new BBlockCache(10, sizeof(BMessage), B_OBJECT_CACHE);
	void *pointer = sMsgCache->Get(size);
	return pointer;
}


void *
BMessage::operator new(size_t, void *pointer)
{
	DEBUG_FUNCTION_ENTER2;
	return pointer;
}


void
BMessage::operator delete(void *pointer, size_t size)
{
	DEBUG_FUNCTION_ENTER2;
	sMsgCache->Save(pointer, size);
}


bool
BMessage::HasSameData(const BMessage &other, bool ignoreFieldOrder,
	bool deep) const
{
	if (this == &other)
		return true;

	if (fHeader->field_count != other.fHeader->field_count)
		return false;

	for (uint32 i = 0; i < fHeader->field_count; i++) {
		field_header *field = &fFields[i];
		field_header *otherField = NULL;

		const char *name = (const char *)fData + field->offset;
		if (ignoreFieldOrder) {
			if (other._FindField(name, B_ANY_TYPE, &otherField) != B_OK)
				return false;
		} else {
			otherField = &other.fFields[i];
			if (otherField->name_length != field->name_length)
				return false;

			const char *otherName = (const char *)other.fData
				+ otherField->offset;
			if (strncmp(name, otherName, field->name_length) != 0)
				return false;
		}

		if (otherField->type != field->type || otherField->count != field->count)
			return false;

		uint8 *data = fData + field->offset + field->name_length;
		uint8 *otherData = other.fData + otherField->offset
			+ otherField->name_length;

		bool needsMemCompare = true;
		if (deep && field->type == B_MESSAGE_TYPE) {
			BMessage message, otherMessage;
			if (message.Unflatten((const char *)data) == B_OK
				&& otherMessage.Unflatten((const char *)otherData) == B_OK) {
				if (!message.HasSameData(ignoreFieldOrder, deep))
					return false;
				needsMemCompare = false;
			}
		}

		if (needsMemCompare) {
			if (otherField->data_size != field->data_size)
				return false;
			if (memcmp(data, otherData, field->data_size) != 0)
				return false;
		}
	}

	return true;
}


status_t
BMessage::_InitCommon(bool initHeader)
{
	DEBUG_FUNCTION_ENTER;
	what = 0;

	fHeader = NULL;
	fFields = NULL;
	fData = NULL;

	fFieldsAvailable = 0;
	fDataAvailable = 0;

	fOriginal = NULL;
	fQueueLink = NULL;

	if (initHeader)
		return _InitHeader();

	fHeader = NULL;
	return B_OK;
}


status_t
BMessage::_InitHeader()
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader == NULL) {
		fHeader = (message_header *)malloc(sizeof(message_header));
		if (fHeader == NULL)
			return B_NO_MEMORY;
	}

	memset(fHeader, 0, sizeof(message_header) - sizeof(fHeader->hash_table));

	fHeader->format = MESSAGE_FORMAT_HAIKU;
	fHeader->flags = MESSAGE_FLAG_VALID;
	fHeader->what = what;
	fHeader->current_specifier = -1;
	fHeader->message_area = -1;

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
	DEBUG_FUNCTION_ENTER;
	if (fHeader != NULL) {
		free(fHeader);
		fHeader = NULL;
	}

	free(fFields);
	fFields = NULL;
	free(fData);
	fData = NULL;

	fFieldsAvailable = 0;
	fDataAvailable = 0;

	delete fOriginal;
	fOriginal = NULL;

	return B_OK;
}


status_t
BMessage::GetInfo(type_code typeRequested, int32 index, char **nameFound,
	type_code *typeFound, int32 *countFound) const
{
	DEBUG_FUNCTION_ENTER;
	if (index < 0 || (uint32)index >= fHeader->field_count)
		return B_BAD_INDEX;

	if (typeRequested == B_ANY_TYPE) {
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
	for (uint32 i = 0; i < fHeader->field_count; i++, field++) {
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
	DEBUG_FUNCTION_ENTER;
	if (countFound)
		*countFound = 0;

	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || field == NULL)
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
	DEBUG_FUNCTION_ENTER;
	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || field == NULL)
		return result;

	if (typeFound)
		*typeFound = field->type;
	if (fixedSize)
		*fixedSize = (field->flags & FIELD_FLAG_FIXED_SIZE) != 0;

	return B_OK;
}


int32
BMessage::CountNames(type_code type) const
{
	DEBUG_FUNCTION_ENTER;
	if (type == B_ANY_TYPE)
		return fHeader->field_count;

	int32 count = 0;
	field_header *field = fFields;
	for (uint32 i = 0; i < fHeader->field_count; i++, field++) {
		if (field->type == type)
			count++;
	}

	return count;
}


bool
BMessage::IsEmpty() const
{
	DEBUG_FUNCTION_ENTER;
	return fHeader->field_count == 0;
}


bool
BMessage::IsSystem() const
{
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_IS_REPLY) != 0;
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
	DEBUG_FUNCTION_ENTER;

	int32 value = B_BENDIAN_TO_HOST_INT32(what);
	printf("BMessage(");
	if (isprint(*(char *)&value))
		printf("'%.4s'", (char *)&value);
	else
		printf("0x%" B_PRIx32, what);
	printf(") {\n");

	if (fHeader == NULL || fFields == NULL || fData == NULL)
		return;

	field_header *field = fFields;
	for (uint32 i = 0; i < fHeader->field_count; i++, field++) {
		value = B_BENDIAN_TO_HOST_INT32(field->type);
		ssize_t size = 0;
		if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0 && field->count > 0)
			size = field->data_size / field->count;

		uint8 *pointer = fData + field->offset + field->name_length;
		for (uint32 j = 0; j < field->count; j++) {
			if (field->count == 1) {
				printf("%s        %s = ", indent,
					(char *)(fData + field->offset));
			} else {
				printf("%s        %s[%" B_PRIu32 "] = ", indent,
					(char *)(fData + field->offset), j);
			}

			switch (field->type) {
				case B_RECT_TYPE:
					print_to_stream_type<BRect>(pointer);
					break;

				case B_POINT_TYPE:
					print_to_stream_type<BPoint>(pointer);
					break;

				case B_STRING_TYPE:
				{
					size = *(uint32 *)pointer;
					pointer += sizeof(uint32);
					printf("string(\"%s\", %ld bytes)\n", (char *)pointer,
						(long)size);
					break;
				}

				case B_INT8_TYPE:
					print_type<int8>("int8(0x%hx or %d or '%.1s')\n", pointer);
					break;

				case B_UINT8_TYPE:
					print_type<uint8>("uint8(0x%hx or %u or '%.1s')\n",
						pointer);
					break;

				case B_INT16_TYPE:
					print_type<int16>("int16(0x%x or %d)\n", pointer);
					break;

				case B_UINT16_TYPE:
					print_type<uint16>("uint16(0x%x or %u\n", pointer);
					break;

				case B_INT32_TYPE:
					print_type<int32>("int32(0x%lx or %ld)\n", pointer);
					break;

				case B_UINT32_TYPE:
					print_type<uint32>("uint32(0x%lx or %lu\n", pointer);
					break;

				case B_INT64_TYPE:
					print_type<int64>("int64(0x%Lx or %Ld)\n", pointer);
					break;

				case B_UINT64_TYPE:
					print_type<uint64>("uint64(0x%Lx or %Ld\n", pointer);
					break;

				case B_BOOL_TYPE:
					printf("bool(%s)\n", *((bool *)pointer) != 0
						? "true" : "false");
					break;

				case B_FLOAT_TYPE:
					print_type<float>("float(%.4f)\n", pointer);
					break;

				case B_DOUBLE_TYPE:
					print_type<double>("double(%.8f)\n", pointer);
					break;

				case B_REF_TYPE:
				{
					size = *(uint32 *)pointer;
					pointer += sizeof(uint32);
					entry_ref ref;
					BPrivate::entry_ref_unflatten(&ref, (char *)pointer, size);

					printf("entry_ref(device=%d, directory=%lld"
						", name=\"%s\", ", (int)ref.device,
						(long long)ref.directory, ref.name);

					BPath path(&ref);
					printf("path=\"%s\")\n", path.Path());
					break;
				}

				case B_MESSAGE_TYPE:
				{
					char buffer[1024];
					sprintf(buffer, "%s        ", indent);

					BMessage message;
					size = *(uint32 *)pointer;
					pointer += sizeof(uint32);
					status_t result = message.Unflatten((const char *)pointer);
					if (result != B_OK) {
						printf("failed unflatten: %s\n", strerror(result));
						break;
					}

					message._PrintToStream(buffer);
					printf("%s        }\n", indent);
					break;
				}

				default:
				{
					printf("(type = '%.4s')(size = %ld)\n", (char *)&value,
						(long)size);
					break;
				}
			}

			pointer += size;
		}
	}
}


status_t
BMessage::Rename(const char *oldEntry, const char *newEntry)
{
	DEBUG_FUNCTION_ENTER;
	if (oldEntry == NULL || newEntry == NULL)
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
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) != 0;
}


bool
BMessage::IsSourceWaiting() const
{
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_REPLY_REQUIRED) != 0
		&& (fHeader->flags & MESSAGE_FLAG_REPLY_DONE) == 0;
}


BMessenger
BMessage::ReturnAddress() const
{
	DEBUG_FUNCTION_ENTER;
	if ((fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) != 0) {
		BMessenger messenger;
		BMessenger::Private(messenger).SetTo(fHeader->reply_team,
			fHeader->reply_port, fHeader->reply_target);
		return messenger;
	}

	return BMessenger();
}


const BMessage *
BMessage::Previous() const
{
	DEBUG_FUNCTION_ENTER;
	/* ToDo: test if the "_previous_" field is used in R5 */
	if (fOriginal == NULL) {
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
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_WAS_DROPPED) != 0;
}


BPoint
BMessage::DropPoint(BPoint *offset) const
{
	DEBUG_FUNCTION_ENTER;
	if (offset)
		*offset = FindPoint("_drop_offset_");

	return FindPoint("_drop_point_");
}


ssize_t
BMessage::FlattenedSize() const
{
	DEBUG_FUNCTION_ENTER;
	return sizeof(message_header) + fHeader->field_count * sizeof(field_header)
		+ fHeader->data_size;
}


status_t
BMessage::Flatten(char *buffer, ssize_t size) const
{
	DEBUG_FUNCTION_ENTER;
	if (buffer == NULL || size < 0)
		return B_BAD_VALUE;

	if (fHeader == NULL)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	memcpy(buffer, fHeader, min_c(sizeof(message_header), (size_t)size));
	buffer += sizeof(message_header);
	size -= sizeof(message_header);

	size_t fieldsSize = fHeader->field_count * sizeof(field_header);
	memcpy(buffer, fFields, min_c(fieldsSize, (size_t)size));
	buffer += fieldsSize;
	size -= fieldsSize;

	memcpy(buffer, fData, min_c(fHeader->data_size, (size_t)size));
	if ((size_t)size < fHeader->data_size)
		return B_BUFFER_OVERFLOW;

	return B_OK;
}


status_t
BMessage::Flatten(BDataIO *stream, ssize_t *size) const
{
	DEBUG_FUNCTION_ENTER;
	if (stream == NULL)
		return B_BAD_VALUE;

	if (fHeader == NULL)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	ssize_t result1 = stream->Write(fHeader, sizeof(message_header));
	if (result1 != sizeof(message_header))
		return result1 < 0 ? result1 : B_ERROR;

	ssize_t result2 = 0;
	if (fHeader->field_count > 0) {
		ssize_t fieldsSize = fHeader->field_count * sizeof(field_header);
		result2 = stream->Write(fFields, fieldsSize);
		if (result2 != fieldsSize)
			return result2 < 0 ? result2 : B_ERROR;
	}

	ssize_t result3 = 0;
	if (fHeader->data_size > 0) {
		result3 = stream->Write(fData, fHeader->data_size);
		if (result3 != (ssize_t)fHeader->data_size)
			return result3 < 0 ? result3 : B_ERROR;
	}

	if (size)
		*size = result1 + result2 + result3;

	return B_OK;
}


status_t
BMessage::_ValidateMessage()
{
	if (fHeader->field_count == 0)
		return B_OK;

	if (fFields == NULL)
		return B_NO_INIT;

	for (uint32 i = 0; i < fHeader->field_count; i++) {
		field_header *field = &fFields[i];
		if ((field->next_field >= 0
				&& (uint32)field->next_field > fHeader->field_count)
			|| (field->offset + field->name_length + field->data_size
				> fHeader->data_size)) {
			// the message is corrupt
			MakeEmpty();
			return B_BAD_VALUE;
		}
	}

	return B_OK;
}


status_t
BMessage::Unflatten(const char *flatBuffer)
{
	DEBUG_FUNCTION_ENTER;
	if (flatBuffer == NULL)
		return B_BAD_VALUE;

	uint32 format = *(uint32 *)flatBuffer;
	if (format != MESSAGE_FORMAT_HAIKU)
		return BPrivate::MessageAdapter::Unflatten(format, this, flatBuffer);

	// native message unflattening

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (fHeader == NULL)
		return B_NO_MEMORY;

	memcpy(fHeader, flatBuffer, sizeof(message_header));
	flatBuffer += sizeof(message_header);

	if (fHeader->format != MESSAGE_FORMAT_HAIKU
		|| (fHeader->flags & MESSAGE_FLAG_VALID) == 0) {
		_InitHeader();
		return B_BAD_VALUE;
	}

	what = fHeader->what;

	if ((fHeader->flags & MESSAGE_FLAG_PASS_BY_AREA) != 0
		&& fHeader->message_area >= 0) {
//		status_t result = _Reference();
//		if (result != B_OK)
//			return result;
	} else {
		fHeader->message_area = -1;

		if (fHeader->field_count > 0) {
			size_t fieldsSize = fHeader->field_count * sizeof(field_header);
			fFields = (field_header *)malloc(fieldsSize);
			if (fFields == NULL) {
				_InitHeader();
				return B_NO_MEMORY;
			}

			memcpy(fFields, flatBuffer, fieldsSize);
			flatBuffer += fieldsSize;
		}

		if (fHeader->data_size > 0) {
			fData = (uint8 *)malloc(fHeader->data_size);
			if (fData == NULL) {
				free(fFields);
				fFields = NULL;
				_InitHeader();
				return B_NO_MEMORY;
			}

			memcpy(fData, flatBuffer, fHeader->data_size);
		}
	}

	return _ValidateMessage();
}


status_t
BMessage::Unflatten(BDataIO *stream)
{
	DEBUG_FUNCTION_ENTER;
	if (stream == NULL)
		return B_BAD_VALUE;

	uint32 format = 0;
	stream->Read(&format, sizeof(uint32));
	if (format != MESSAGE_FORMAT_HAIKU)
		return BPrivate::MessageAdapter::Unflatten(format, this, stream);

	// native message unflattening

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (fHeader == NULL)
		return B_NO_MEMORY;

	fHeader->format = format;
	uint8 *header = (uint8 *)fHeader;
	ssize_t result = stream->Read(header + sizeof(uint32),
		sizeof(message_header) - sizeof(uint32));
	if (result != sizeof(message_header) - sizeof(uint32)
		|| (fHeader->flags & MESSAGE_FLAG_VALID) == 0) {
		_InitHeader();
		return result < 0 ? result : B_BAD_VALUE;
	}

	what = fHeader->what;

	fHeader->message_area = -1;

	if (fHeader->field_count > 0) {
		ssize_t fieldsSize = fHeader->field_count * sizeof(field_header);
		fFields = (field_header *)malloc(fieldsSize);
		if (fFields == NULL) {
			_InitHeader();
			return B_NO_MEMORY;
		}

		result = stream->Read(fFields, fieldsSize);
		if (result != fieldsSize)
			return result < 0 ? result : B_BAD_VALUE;
	}

	if (fHeader->data_size > 0) {
		fData = (uint8 *)malloc(fHeader->data_size);
		if (fData == NULL) {
			free(fFields);
			fFields = NULL;
			_InitHeader();
			return B_NO_MEMORY;
		}

		result = stream->Read(fData, fHeader->data_size);
		if (result != (ssize_t)fHeader->data_size)
			return result < 0 ? result : B_BAD_VALUE;
	}

	return _ValidateMessage();
}


status_t
BMessage::AddSpecifier(const char *property)
{
	DEBUG_FUNCTION_ENTER;
	BMessage message(B_DIRECT_SPECIFIER);
	status_t result = message.AddString(B_PROPERTY_ENTRY, property);
	if (result < B_OK)
		return result;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, int32 index)
{
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier, int32 *_what,
	const char **property) const
{
	DEBUG_FUNCTION_ENTER;

	if (index != NULL)
		*index = fHeader->current_specifier;

	if (fHeader->current_specifier < 0
		|| (fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) == 0)
		return B_BAD_SCRIPT_SYNTAX;

	if (specifier) {
		if (FindMessage(B_SPECIFIER_ENTRY, fHeader->current_specifier,
			specifier) < B_OK)
			return B_BAD_SCRIPT_SYNTAX;

		if (_what != NULL)
			*_what = specifier->what;

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
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_HAS_SPECIFIERS) != 0;
}


status_t
BMessage::PopSpecifier()
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader->current_specifier < 0 ||
		(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) == 0)
		return B_BAD_VALUE;

	if (fHeader->current_specifier >= 0)
		fHeader->current_specifier--;

	return B_OK;
}


status_t
BMessage::_ResizeData(uint32 offset, int32 change)
{
	if (change == 0)
		return B_OK;

	/* optimize for the most usual case: appending data */
	if (offset < fHeader->data_size) {
		field_header *field = fFields;
		for (uint32 i = 0; i < fHeader->field_count; i++, field++) {
			if (field->offset >= offset)
				field->offset += change;
		}
	}

	if (change > 0) {
		if (fDataAvailable >= (uint32)change) {
			if (offset < fHeader->data_size) {
				memmove(fData + offset + change, fData + offset,
					fHeader->data_size - offset);
			}

			fDataAvailable -= change;
			fHeader->data_size += change;
			return B_OK;
		}

		size_t size = fHeader->data_size * 2;
		size = min_c(size, fHeader->data_size + MAX_DATA_PREALLOCATION);
		size = max_c(size, fHeader->data_size + change);

		uint8 *newData = (uint8 *)realloc(fData, size);
		if (size > 0 && newData == NULL)
			return B_NO_MEMORY;

		fData = newData;
		if (offset < fHeader->data_size) {
			memmove(fData + offset + change, fData + offset,
				fHeader->data_size - offset);
		}

		fHeader->data_size += change;
		fDataAvailable = size - fHeader->data_size;
	} else {
		ssize_t length = fHeader->data_size - offset + change;
		if (length > 0)
			memmove(fData + offset, fData + offset - change, length);

		// change is negative
		fHeader->data_size += change;
		fDataAvailable -= change;

		if (fDataAvailable > MAX_DATA_PREALLOCATION) {
			ssize_t available = MAX_DATA_PREALLOCATION / 2;
			ssize_t size = fHeader->data_size + available;
			uint8 *newData = (uint8 *)realloc(fData, size);
			if (size > 0 && newData == NULL) {
				// this is strange, but not really fatal
				return B_OK;
			}

			fData = newData;
			fDataAvailable = available;
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
	if (name == NULL)
		return B_BAD_VALUE;

	if (fHeader == NULL || fFields == NULL || fData == NULL)
		return B_NAME_NOT_FOUND;

	uint32 hash = _HashName(name) % fHeader->hash_table_size;
	int32 nextField = fHeader->hash_table[hash];

	while (nextField >= 0) {
		field_header *field = &fFields[nextField];
		if ((field->flags & FIELD_FLAG_VALID) == 0)
			break;

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
	if (fHeader == NULL)
		return B_ERROR;

	if (fFieldsAvailable <= 0) {
		uint32 count = fHeader->field_count * 2 + 1;
		count = min_c(count, fHeader->field_count + MAX_FIELD_PREALLOCATION);

		field_header *newFields = (field_header *)realloc(fFields,
			count * sizeof(field_header));
		if (count > 0 && newFields == NULL)
			return B_NO_MEMORY;

		fFields = newFields;
		fFieldsAvailable = count - fHeader->field_count;
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

	fFieldsAvailable--;
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
	for (uint32 i = 0; i < fHeader->hash_table_size; i++, value++) {
		if (*value > index)
			*value -= 1;
		else if (*value == index)
			*value = nextField;
	}

	field_header *other = fFields;
	for (uint32 i = 0; i < fHeader->field_count; i++, other++) {
		if (other->next_field > index)
			other->next_field--;
		else if (other->next_field == index)
			other->next_field = nextField;
	}

	size_t size = (fHeader->field_count - index - 1) * sizeof(field_header);
	memmove(fFields + index, fFields + index + 1, size);
	fHeader->field_count--;
	fFieldsAvailable++;

	if (fFieldsAvailable > MAX_FIELD_PREALLOCATION) {
		ssize_t available = MAX_FIELD_PREALLOCATION / 2;
		size = (fHeader->field_count + available) * sizeof(field_header);
		field_header *newFields = (field_header *)realloc(fFields, size);
		if (size > 0 && newFields == NULL) {
			// this is strange, but not really fatal
			return B_OK;
		}

		fFields = newFields;
		fFieldsAvailable = available;
	}

	return B_OK;
}


status_t
BMessage::AddData(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool isFixedSize, int32 count)
{
	// Note that the "count" argument is only a hint at how many items
	// the caller expects to add to this field. Since we do no item pre-
	// allocation, we ignore this argument.
	DEBUG_FUNCTION_ENTER;
	if (numBytes <= 0 || data == NULL)
		return B_BAD_VALUE;

	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);
	if (result == B_NAME_NOT_FOUND)
		result = _AddField(name, type, isFixedSize, &field);

	if (result < B_OK)
		return result;

	if (field == NULL)
		return B_ERROR;

	uint32 offset = field->offset + field->name_length + field->data_size;
	if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0) {
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
		int32 change = numBytes + sizeof(uint32);
		result = _ResizeData(offset, change);
		if (result < B_OK) {
			if (field->count == 0)
				_RemoveField(field);
			return result;
		}

		uint32 size = (uint32)numBytes;
		memcpy(fData + offset, &size, sizeof(uint32));
		memcpy(fData + offset + sizeof(uint32), data, size);
		field->data_size += change;
	}

	field->count++;
	return B_OK;
}


status_t
BMessage::RemoveData(const char *name, int32 index)
{
	DEBUG_FUNCTION_ENTER;
	if (index < 0)
		return B_BAD_INDEX;

	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);

	if (result < B_OK)
		return result;

	if (field == NULL)
		return B_ERROR;

	if ((uint32)index >= field->count)
		return B_BAD_INDEX;

	if (field->count == 1)
		return _RemoveField(field);

	uint32 offset = field->offset + field->name_length;
	if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0) {
		ssize_t size = field->data_size / field->count;
		result = _ResizeData(offset + index * size, -size);
		if (result < B_OK)
			return result;

		field->data_size -= size;
	} else {
		uint8 *pointer = fData + offset;
		for (int32 i = 0; i < index; i++) {
			offset += *(uint32 *)pointer + sizeof(uint32);
			pointer = fData + offset;
		}

		size_t currentSize = *(uint32 *)pointer + sizeof(uint32);
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
	DEBUG_FUNCTION_ENTER;
	field_header *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);

	if (result < B_OK)
		return result;

	if (field == NULL)
		return B_ERROR;

	return _RemoveField(field);
}


status_t
BMessage::MakeEmpty()
{
	DEBUG_FUNCTION_ENTER;
	_Clear();
	_InitHeader();
	return B_OK;
}


status_t
BMessage::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *numBytes) const
{
	DEBUG_FUNCTION_ENTER;
	if (data == NULL)
		return B_BAD_VALUE;

	*data = NULL;
	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (field == NULL)
		return B_ERROR;

	if (index < 0 || (uint32)index >= field->count)
		return B_BAD_INDEX;

	if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0) {
		size_t bytes = field->data_size / field->count;
		*data = fData + field->offset + field->name_length + index * bytes;
		if (numBytes != NULL)
			*numBytes = bytes;
	} else {
		uint8 *pointer = fData + field->offset + field->name_length;
		for (int32 i = 0; i < index; i++)
			pointer += *(uint32 *)pointer + sizeof(uint32);

		*data = pointer + sizeof(uint32);
		if (numBytes != NULL)
			*numBytes = *(uint32 *)pointer;
	}

	return B_OK;
}


status_t
BMessage::ReplaceData(const char *name, type_code type, int32 index,
	const void *data, ssize_t numBytes)
{
	DEBUG_FUNCTION_ENTER;
	if (numBytes <= 0 || data == NULL)
		return B_BAD_VALUE;

	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (field == NULL)
		return B_ERROR;

	if (index < 0 || (uint32)index >= field->count)
		return B_BAD_INDEX;

	if ((field->flags & FIELD_FLAG_FIXED_SIZE) != 0) {
		ssize_t size = field->data_size / field->count;
		if (size != numBytes)
			return B_BAD_VALUE;

		memcpy(fData + field->offset + field->name_length + index * size, data,
			size);
	} else {
		uint32 offset = field->offset + field->name_length;
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(uint32 *)pointer + sizeof(uint32);
			pointer = fData + offset;
		}

		size_t currentSize = *(uint32 *)pointer;
		int32 change = numBytes - currentSize;
		result = _ResizeData(offset, change);
		if (result < B_OK)
			return result;

		uint32 newSize = (uint32)numBytes;
		memcpy(fData + offset, &newSize, sizeof(uint32));
		memcpy(fData + offset + sizeof(uint32), data, newSize);
		field->data_size += change;
	}

	return B_OK;
}


bool
BMessage::HasData(const char *name, type_code type, int32 index) const
{
	DEBUG_FUNCTION_ENTER;
	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return false;

	if (field == NULL)
		return false;

	if (index < 0 || (uint32)index >= field->count)
		return false;

	return true;
}


void BMessage::_ReservedMessage1(void) {};
void BMessage::_ReservedMessage2(void) {};
void BMessage::_ReservedMessage3(void) {};


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
DEFINE_FUNCTIONS(uint8, UInt8, B_UINT8_TYPE);
DEFINE_FUNCTIONS(int16, Int16, B_INT16_TYPE);
DEFINE_FUNCTIONS(uint16, UInt16, B_UINT16_TYPE);
DEFINE_FUNCTIONS(int32, Int32, B_INT32_TYPE);
DEFINE_FUNCTIONS(uint32, UInt32, B_UINT32_TYPE);
DEFINE_FUNCTIONS(int64, Int64, B_INT64_TYPE);
DEFINE_FUNCTIONS(uint64, UInt64, B_UINT64_TYPE);
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
	if (message == NULL)
		return B_BAD_VALUE;

	// TODO: This and the following functions waste time by allocating and
	// copying an extra buffer. Functions can be added that return a direct
	// pointer into the message.

	char stackBuffer[16384];
	ssize_t size = message->FlattenedSize();

	char* buffer;
	if (size > (ssize_t)sizeof(stackBuffer)) {
		buffer = (char *)malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;
	} else
		buffer = stackBuffer;

	status_t error = message->Flatten(buffer, size);

	if (error >= B_OK)
		error = AddData(name, B_MESSAGE_TYPE, buffer, size, false);

	if (buffer != stackBuffer)
		free(buffer);

	return error;
}


status_t
BMessage::AddFlat(const char *name, BFlattenable *object, int32 count)
{
	if (object == NULL)
		return B_BAD_VALUE;

	char stackBuffer[16384];
	ssize_t size = object->FlattenedSize();

	char* buffer;
	if (size > (ssize_t)sizeof(stackBuffer)) {
		buffer = (char *)malloc(size);
		if (buffer == NULL)
			return B_NO_MEMORY;
	} else
		buffer = stackBuffer;

	status_t error = object->Flatten(buffer, size);

	if (error >= B_OK)
		error = AddData(name, object->TypeCode(), buffer, size, false);

	if (buffer != stackBuffer)
		free(buffer);

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
	if (string == NULL)
		return B_BAD_VALUE;

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
	if (pointer == NULL)
		return B_BAD_VALUE;

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
	if (messenger == NULL)
		return B_BAD_VALUE;

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
	if (ref == NULL)
		return B_BAD_VALUE;

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
	if (message == NULL)
		return B_BAD_VALUE;

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
	if (object == NULL)
		return B_BAD_VALUE;

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
	if (string == NULL)
		return B_BAD_VALUE;

	return ReplaceData(name, B_STRING_TYPE, 0, string, strlen(string) + 1);
}


status_t
BMessage::ReplaceString(const char *name, int32 index, const char *string)
{
	if (string == NULL)
		return B_BAD_VALUE;

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
	if (message == NULL)
		return B_BAD_VALUE;

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
	if (object == NULL)
		return B_BAD_VALUE;

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
