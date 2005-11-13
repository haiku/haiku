/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <Message.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>
#include "dano_message.h"
#include "r5_message.h"

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>

#include <Application.h>
#include <AppMisc.h>
#include <BlockCache.h>
#include <Entry.h>
#include <KMessage.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>
#include <TokenSpace.h>


#define DEBUG_FUNCTION_ENTER	//debug_printf("%ld: %s\n", __LINE__, __PRETTY_FUNCTION__);


static const uint32 kMessageMagicR5 = 'FOB1';
static const uint32 kMessageMagicR5Swapped = '1BOF';
static const uint32 kMessageMagicDano = 'FOB2';
static const uint32 kMessageMagicDanoSwapped = '2BOF';
static const uint32 kMessageMagic4 = '4GSM';
static const uint32 kMessageMagic4Swapped = 'MSG4';


const char *B_SPECIFIER_ENTRY = "specifiers";
const char *B_PROPERTY_ENTRY = "property";
const char *B_PROPERTY_NAME_ENTRY = "name";


static status_t		handle_reply(port_id replyPort, int32 *pCode,
						bigtime_t timeout, BMessage *reply);
static status_t		convert_message(const KMessage *fromMessage,
						BMessage *toMessage);


BBlockCache *BMessage::sMsgCache = NULL;
port_id BMessage::sReplyPorts[sNumReplyPorts];
long BMessage::sReplyPortInUse[sNumReplyPorts];


BMessage::BMessage()
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
	_InitHeader();
}


BMessage::BMessage(uint32 _what)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
	_InitHeader();
	fHeader->what = what = _what;
}


BMessage::BMessage(const BMessage &other)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
	*this = other;
}


BMessage::~BMessage()
{
	DEBUG_FUNCTION_ENTER;
	if (IsSourceWaiting())
		SendReply(B_NO_REPLY);

	_Clear();
}


BMessage &
BMessage::operator=(const BMessage &other)
{
	DEBUG_FUNCTION_ENTER;
	_Clear();

	fHeader = (MessageHeader *)malloc(sizeof(MessageHeader));
	memcpy(fHeader, other.fHeader, sizeof(MessageHeader));

	if (fHeader->fieldsSize > 0) {
		fFields = (FieldHeader *)malloc(fHeader->fieldsSize);
		memcpy(fFields, other.fFields, fHeader->fieldsSize);
	}

	if (fHeader->dataSize > 0) {
		fData = (uint8 *)malloc(other.fHeader->dataSize);
		memcpy(fData, other.fData, other.fHeader->dataSize);
	}

	fHeader->fieldsAvailable = 0;
	fHeader->dataAvailable = 0;
	what = fHeader->what;
	fQueueLink = other.fQueueLink;
	return *this;
}


void *
BMessage::operator new(size_t size)
{
	DEBUG_FUNCTION_ENTER;
	if (!sMsgCache)
		sMsgCache = new BBlockCache(10, size, B_OBJECT_CACHE);

	return sMsgCache->Get(size);
}


void *
BMessage::operator new(size_t, void *pointer)
{
	DEBUG_FUNCTION_ENTER;
	return pointer;
}


void
BMessage::operator delete(void *pointer, size_t size)
{
	DEBUG_FUNCTION_ENTER;
	sMsgCache->Save(pointer, size);
}


status_t
BMessage::_InitCommon()
{
	DEBUG_FUNCTION_ENTER;
	what = 0;

	fHeader = NULL;
	fFields = NULL;
	fData = NULL;

	fOriginal = NULL;
	fQueueLink = NULL;
	return B_OK;
}


status_t
BMessage::_InitHeader()
{
	DEBUG_FUNCTION_ENTER;
	fHeader = (MessageHeader *)malloc(sizeof(MessageHeader));
	memset(fHeader, 0, sizeof(MessageHeader));

	fHeader->format = kMessageMagic4;
	fHeader->flags = MESSAGE_FLAG_VALID;
	fHeader->currentSpecifier = -1;

	fHeader->target = B_NULL_TOKEN;
	fHeader->replyTarget = B_NULL_TOKEN;
	fHeader->replyPort = -1;
	fHeader->replyTeam = -1;

	// initializing the hash table to -1 because 0 is a valid index
	fHeader->hashTableSize = MESSAGE_BODY_HASH_TABLE_SIZE;
	memset(&fHeader->hashTable, 255, sizeof(fHeader->hashTable));
	return B_OK;
}


status_t
BMessage::_Clear()
{
	delete fOriginal;
	fOriginal = NULL;
	fQueueLink = NULL;

	free(fHeader);
	fHeader = NULL;
	free(fFields);
	fFields = NULL;
	free(fData);
	fData = NULL;

	return B_OK;
}


status_t
BMessage::GetInfo(type_code typeRequested, int32 index, char **nameFound,
	type_code *typeFound, int32 *countFound = NULL) const
{
	DEBUG_FUNCTION_ENTER;
	if (typeRequested == B_ANY_TYPE) {
		if (index >= fHeader->fieldCount)
			return B_BAD_INDEX;

		*nameFound = (char *)fData + fFields[index].offset;
		*typeFound = fFields[index].type;
		if (countFound)
			*countFound = fFields[index].count;
		return B_OK;
	}

	int32 counter = -1;
	FieldHeader *field = fFields;
	for (int32 i = 0; i < fHeader->fieldCount; i++, field++) {
		if (field->type == typeRequested)
			counter++;

		if (counter == index) {
			*nameFound = (char *)fData + field->offset;
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

	FieldHeader *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || !field)
		return result;

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
	FieldHeader *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);
	if (result < B_OK || !field)
		return result;

	*typeFound = field->type;
	*fixedSize = field->flags & FIELD_FLAG_FIXED_SIZE;

	return B_OK;
}


int32
BMessage::CountNames(type_code type) const
{
	DEBUG_FUNCTION_ENTER;
	if (type == B_ANY_TYPE)
		return fHeader->fieldCount;

	int32 count = 0;
	FieldHeader *field = fFields;
	for (int32 i = 0; i < fHeader->fieldCount; i++, field++) {
		if (field->type == type)
			count++;
	}

	return count;
}


bool
BMessage::IsEmpty() const
{
	DEBUG_FUNCTION_ENTER;
	return fHeader->fieldCount == 0;
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
	return fHeader->flags & MESSAGE_FLAG_IS_REPLY;
}


#define PRINT_SOME_TYPE(type, typeCode)										\
	case typeCode: {														\
		sprintf(buffer + strlen(buffer), "size=%2ld, ", size);				\
		printf("%s", buffer);												\
		memset(buffer, ' ', strlen(buffer));								\
																			\
		type *item = (type *)(fData + field->offset + field->nameLength);	\
		for (int32 i = 0; i < field->count; i++, item++) {					\
			if (i > 0) /* indent */											\
				printf(buffer);												\
																			\
			printf("data[%ld]: ", i);										\
			item->PrintToStream();											\
		}																	\
	} break

#define PRINT_INT_TYPE(type, typeCode, format, swap)						\
	case typeCode: {														\
		sprintf(buffer + strlen(buffer), "size=%2ld, ", size);				\
		printf("%s", buffer);												\
		memset(buffer, ' ', strlen(buffer));								\
																			\
		type *item = (type *)(fData + field->offset + field->nameLength);	\
		for (int32 i = 0; i < field->count; i++, item++) {					\
			if (i > 0) /* indent */											\
				printf(buffer);												\
																			\
			printf("data[%ld]: ", i);										\
			type value = swap(*item);										\
			printf(format, *item, *item, &value);							\
		}																	\
	} break


#define PRINT_FLOAT_TYPE(type, typeCode, format)							\
	case typeCode: {														\
		sprintf(buffer + strlen(buffer), "size=%2ld, ", size);				\
		printf("%s", buffer);												\
		memset(buffer, ' ', strlen(buffer));								\
																			\
		type *item = (type *)(fData + field->offset + field->nameLength);	\
		for (int32 i = 0; i < field->count; i++, item++) {					\
			if (i > 0) /* indent */											\
				printf(buffer);												\
																			\
			printf("data[%ld]: ", i);										\
			printf(format, *item);											\
		}																	\
	} break


#define REMOVE_BELOW_20(x)	(x >= 0x20 ? x : 0x20)


void
BMessage::PrintToStream() const
{
	DEBUG_FUNCTION_ENTER;
	printf("BMessage: what = ");

	int32 value = B_BENDIAN_TO_HOST_INT32(what);
	printf("%.4s", (char *)&value);
	printf(" (0x%lx, or %ld)\n", what, what);

	char buffer[1024];
	FieldHeader *field = fFields;
	for (int32 i = 0; i < fHeader->fieldCount; i++, field++) {
		value = B_BENDIAN_TO_HOST_INT32(field->type);
		sprintf(buffer, "    entry %14s, type='%.4s', c=%ld, ",
			(char *)(fData + field->offset), (char *)&value, field->count);

		ssize_t size = 0;
		if (field->flags & FIELD_FLAG_FIXED_SIZE)
			size = field->dataSize / field->count;

		switch (field->type) {
			PRINT_SOME_TYPE(BRect, B_RECT_TYPE);
			PRINT_SOME_TYPE(BPoint, B_POINT_TYPE);

			case B_STRING_TYPE: {
				printf("%s", buffer);
				memset(buffer, ' ', strlen(buffer));

				uint8 *pointer = fData + field->offset + field->nameLength;
				for (int32 i = 0; i < field->count; i++) {
					if (i > 0) /* indent */
						printf("%s", buffer);

					ssize_t size = *(ssize_t *)pointer;
					pointer += sizeof(ssize_t);
					printf("size=%ld, data[%ld]: ", size, i);
					printf("\"%s\"\n", (char *)pointer);
					pointer += size;
				}
			} break;

			PRINT_INT_TYPE(int8, B_INT8_TYPE, "0x%hx (%d \'%.1s\')\n", REMOVE_BELOW_20);
			PRINT_INT_TYPE(int16, B_INT16_TYPE, "0x%lx (%d, \'%.2s\')\n", B_BENDIAN_TO_HOST_INT16);
			PRINT_INT_TYPE(int32, B_INT32_TYPE, "0x%lx (%ld, \'%.4s\')\n", B_BENDIAN_TO_HOST_INT32);
			PRINT_INT_TYPE(int64, B_INT64_TYPE, "0x%Lx (%lld, \'%.4s\')\n", B_BENDIAN_TO_HOST_INT64);

			PRINT_FLOAT_TYPE(bool, B_BOOL_TYPE, "%d\n");
			PRINT_FLOAT_TYPE(float, B_FLOAT_TYPE, "%.4f\n");
			PRINT_FLOAT_TYPE(double, B_DOUBLE_TYPE, "%.8f\n");

			case B_REF_TYPE: {
				printf("%s", buffer);
				memset(buffer, ' ', strlen(buffer));

				uint8 *pointer = fData + field->offset + field->nameLength;
				for (int32 i = 0; i < field->count; i++) {
					if (i > 0) /* indent */
						printf("%s", buffer);

					ssize_t size = *(ssize_t *)pointer;
					pointer += sizeof(ssize_t);
					entry_ref ref;
					BPrivate::entry_ref_unflatten(&ref, (char *)pointer, size);

					printf("size=%ld, data[%ld]: ", size, i);
					printf("device=%ld, directory=%lld, name=\"%s\", ",
						ref.device, ref.directory, ref.name);

					BPath path(&ref);
					printf("path=\"%s\"\n", path.Path());
					pointer += size;
				}
			} break;

			default: {
				sprintf(buffer + strlen(buffer), "size=%2ld, \n", size);
				printf("%s", buffer);
			}
		}
	}	
}


#undef PRINT_FLOAT_TYPE
#undef PRINT_INT_TYPE
#undef PRINT_SOME_TYPE


status_t
BMessage::Rename(const char *oldEntry, const char *newEntry)
{
	DEBUG_FUNCTION_ENTER;
	if (!oldEntry || !newEntry)
		return B_BAD_VALUE;

	uint32 hash = _HashName(oldEntry) % fHeader->hashTableSize;
	int32 *nextField = &fHeader->hashTable[hash];

	while (*nextField >= 0) {
		FieldHeader *field = &fFields[*nextField];

		if (strncmp((const char *)(fData + field->offset), oldEntry,
			field->nameLength) == 0) {
			// nextField points to the field for oldEntry, save it and unlink
			int32 index = *nextField;
			*nextField = field->nextField;
			field->nextField = -1;

			hash = _HashName(newEntry) % fHeader->hashTableSize;
			nextField = &fHeader->hashTable[hash];
			while (*nextField >= 0)
				nextField = &fFields[*nextField].nextField;
			*nextField = index;

			int32 oldLength = field->nameLength;
			field->nameLength = strlen(newEntry) + 1;
			_ResizeData(field->offset, field->nameLength - oldLength);
			memcpy(fData + field->offset, newEntry, field->nameLength);
			return B_OK;
		}

		nextField = &field->nextField;
	}

	return B_NAME_NOT_FOUND;
}


bool
BMessage::WasDelivered() const
{
	DEBUG_FUNCTION_ENTER;
	return fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED;
}


bool
BMessage::IsSourceWaiting() const
{
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_REPLY_REQUIRED)
		&& !(fHeader->flags & MESSAGE_FLAG_REPLY_DONE);
}


bool
BMessage::IsSourceRemote() const
{
	DEBUG_FUNCTION_ENTER;
	return (fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED)
		&& (fHeader->replyTeam != BPrivate::current_team());
}


BMessenger
BMessage::ReturnAddress() const
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) {
		BMessenger messenger;
		BMessenger::Private(messenger).SetTo(fHeader->replyTeam,
			fHeader->replyPort, fHeader->replyTarget,
			fHeader->replyTarget == B_PREFERRED_TOKEN);
		return messenger;
	}

	return BMessenger();
}


const BMessage *
BMessage::Previous() const
{
	DEBUG_FUNCTION_ENTER;
	/* ToDo: test if the "_previous_" field is used in R5 */
	if (!fOriginal) {
		delete fOriginal;
		fOriginal = new BMessage;

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
	return fHeader->flags & MESSAGE_FLAG_READ_ONLY;
}


BPoint
BMessage::DropPoint(BPoint *offset = NULL) const
{
	DEBUG_FUNCTION_ENTER;
	if (offset)
		*offset = FindPoint("_drop_offset_");

	return FindPoint("_drop_point_");
}


status_t
BMessage::SendReply(uint32 command, BHandler *replyTo)
{
	DEBUG_FUNCTION_ENTER;
	BMessage message(command);
	return SendReply(&message, replyTo);
}


status_t
BMessage::SendReply(BMessage *reply, BHandler *replyTo, bigtime_t timeout)
{
	DEBUG_FUNCTION_ENTER;
	BMessenger messenger(replyTo);
	return SendReply(reply, messenger, timeout);
}


status_t
BMessage::SendReply(BMessage *reply, BMessenger replyTo, bigtime_t timeout)
{
	DEBUG_FUNCTION_ENTER;
	BMessenger messenger;
	BMessenger::Private messengerPrivate(messenger);
	messengerPrivate.SetTo(fHeader->replyTeam, fHeader->replyPort,
		fHeader->replyTarget, fHeader->replyTarget == B_PREFERRED_TOKEN);

	if (fHeader->flags & MESSAGE_FLAG_REPLY_REQUIRED) {
		if (fHeader->flags & MESSAGE_FLAG_REPLY_DONE)
			return B_DUPLICATE_REPLY;

		fHeader->flags |= MESSAGE_FLAG_REPLY_DONE;
		reply->fHeader->flags |= MESSAGE_FLAG_IS_REPLY;
		status_t result = messenger.SendMessage(reply, replyTo, timeout);
		reply->fHeader->flags &= ~MESSAGE_FLAG_IS_REPLY;

		if (result != B_OK) {
			if (set_port_owner(messengerPrivate.Port(),
				messengerPrivate.Team()) == B_BAD_TEAM_ID) {
				delete_port(messengerPrivate.Port());
			}
		}

		return result;
	}

	// no reply required
	if (!(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_REPLY;

	reply->AddMessage("_previous_", this);
	reply->fHeader->flags |= MESSAGE_FLAG_IS_REPLY;
	status_t result = messenger.SendMessage(reply, replyTo, timeout);
	reply->fHeader->flags &= ~MESSAGE_FLAG_IS_REPLY;
	reply->RemoveName("_previous_");
	return result;
}


status_t
BMessage::SendReply(uint32 command, BMessage *replyToReply)
{
	DEBUG_FUNCTION_ENTER;
	BMessage message(command);
	return SendReply(&message, replyToReply);
}


status_t
BMessage::SendReply(BMessage *reply, BMessage *replyToReply,
	bigtime_t sendTimeout, bigtime_t replyTimeout)
{
	DEBUG_FUNCTION_ENTER;
	BMessenger messenger;
	BMessenger::Private messengerPrivate(messenger);
	messengerPrivate.SetTo(fHeader->replyTeam, fHeader->replyPort,
		fHeader->replyTarget, fHeader->replyTarget == B_PREFERRED_TOKEN);

	if (fHeader->flags & MESSAGE_FLAG_REPLY_REQUIRED) {
		if (fHeader->flags & MESSAGE_FLAG_REPLY_DONE)
			return B_DUPLICATE_REPLY;

		fHeader->flags |= MESSAGE_FLAG_REPLY_DONE;
		reply->fHeader->flags |= MESSAGE_FLAG_IS_REPLY;
		status_t result = messenger.SendMessage(reply, replyToReply,
			sendTimeout, replyTimeout);
		reply->fHeader->flags &= ~MESSAGE_FLAG_IS_REPLY;

		if (result != B_OK) {
			if (set_port_owner(messengerPrivate.Port(),
				messengerPrivate.Team()) == B_BAD_TEAM_ID) {
				delete_port(messengerPrivate.Port());
			}
		}

		return result;
	}

	// no reply required
	if (!(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_REPLY;

	reply->AddMessage("_previous_", this);
	reply->fHeader->flags |= MESSAGE_FLAG_IS_REPLY;
	status_t result = messenger.SendMessage(reply, replyToReply, sendTimeout,
		replyTimeout);
	reply->fHeader->flags &= ~MESSAGE_FLAG_IS_REPLY;
	reply->RemoveName("_previous_");
	return result;
}


ssize_t
BMessage::FlattenedSize() const
{
	DEBUG_FUNCTION_ENTER;
	//return _NativeFlattenedSize();
	return BPrivate::R5MessageFlattenedSize(this);
}


status_t
BMessage::Flatten(char *buffer, ssize_t size) const
{
	DEBUG_FUNCTION_ENTER;
	//return _NativeFlatten(buffer, size);
	return BPrivate::R5MessageFlatten(this, buffer, size);
}


status_t
BMessage::Flatten(BDataIO *stream, ssize_t *size) const
{
	DEBUG_FUNCTION_ENTER;
	//return _NativeFlatten(stream, size);
	return BPrivate::R5MessageFlatten(this, stream, size);
}


ssize_t
BMessage::_NativeFlattenedSize() const
{
	DEBUG_FUNCTION_ENTER;
	return sizeof(MessageHeader) + fHeader->fieldsSize + fHeader->dataSize;
}


status_t
BMessage::_NativeFlatten(char *buffer, ssize_t size) const
{
	DEBUG_FUNCTION_ENTER;
	if (!buffer)
		return B_BAD_VALUE;

	if (!fHeader)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	memcpy(buffer, fHeader, min_c(sizeof(MessageHeader), (size_t)size));
	buffer += sizeof(MessageHeader);
	size -= sizeof(MessageHeader);

	memcpy(buffer, fFields, min_c(fHeader->fieldsSize, size));
	buffer += fHeader->fieldsSize;
	size -= fHeader->fieldsSize;

	memcpy(buffer, fData, min_c(fHeader->dataSize, size));
	if (size >= fHeader->dataSize)
		return B_OK;

	return B_NO_MEMORY;
}


status_t
BMessage::_NativeFlatten(BDataIO *stream, ssize_t *size) const
{
	DEBUG_FUNCTION_ENTER;
	if (!stream)
		return B_BAD_VALUE;

	if (!fHeader)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;

	ssize_t result1 = stream->Write(fHeader, sizeof(MessageHeader));
	if (result1 != sizeof(MessageHeader))
		return (result1 >= 0 ? B_ERROR : result1);

	ssize_t result2 = 0;
	if (fHeader->fieldsSize > 0) {
		stream->Write(fFields, fHeader->fieldsSize);
		if (result2 != fHeader->fieldsSize)
			return (result2 >= 0 ? B_ERROR : result2);
	}

	ssize_t result3 = 0;
	if (fHeader->dataSize > 0) {
		stream->Write(fData, fHeader->dataSize);
		if (result3 != fHeader->dataSize)
			return (result3 >= 0 ? B_ERROR : result3);
	}

	if (size)
		*size = result1 + result2 + result3;

	return B_OK;
}


status_t
BMessage::Unflatten(const char *flatBuffer)
{
	DEBUG_FUNCTION_ENTER;
	if (!flatBuffer)
		return B_BAD_VALUE;

	uint32 format = *(uint32 *)flatBuffer;
	if (format != kMessageMagic4) {
		if (format == KMessage::kMessageHeaderMagic) {
			KMessage message;
			status_t result = message.SetTo(flatBuffer,
				((KMessage::Header*)flatBuffer)->size);
			if (result != B_OK)
				return result;

			return convert_message(&message, this);
		}

		if (format == kMessageMagicR5)
			return BPrivate::R5MessageUnflatten(this, flatBuffer);

		if (format == kMessageMagicDano) {
			BMemoryIO stream(flatBuffer, BPrivate::dano_message_size(flatBuffer));
			return BPrivate::unflatten_dano_message(format, stream, *this);
		}
	}

	free(fHeader);
	fHeader = NULL;
	fHeader = (MessageHeader *)malloc(sizeof(MessageHeader));
	if (!fHeader)
		return B_NO_MEMORY;

	memcpy(fHeader, flatBuffer, sizeof(MessageHeader));
	flatBuffer += sizeof(MessageHeader);

	/* ToDo: better message validation (checksum) */
	if (fHeader->format != kMessageMagic4 ||
		!(fHeader->flags & MESSAGE_FLAG_VALID)) {
		_Clear();
		_InitHeader();
		return B_BAD_VALUE;
	}

	free(fFields);
	fFields = NULL;
	if (fHeader->fieldsSize > 0) {
		fFields = (FieldHeader *)malloc(fHeader->fieldsSize);
		if (!fFields)
			return B_NO_MEMORY;

		memcpy(fFields, flatBuffer, fHeader->fieldsSize);
		flatBuffer += fHeader->fieldsSize;
	}

	free(fData);
	fData = NULL;
	if (fHeader->dataSize > 0) {
		fData = (uint8 *)malloc(fHeader->dataSize);
		if (!fData)
			return B_NO_MEMORY;

		memcpy(fData, flatBuffer, fHeader->dataSize);
	}

	fHeader->fieldsAvailable = 0;
	fHeader->dataAvailable = 0;
	what = fHeader->what;
	return B_OK;
}


status_t
BMessage::Unflatten(BDataIO *stream)
{
	DEBUG_FUNCTION_ENTER;
	if (!stream)
		return B_BAD_VALUE;

	uint32 format = 0;
	stream->Read(&format, sizeof(uint32));
	if (format != kMessageMagic4) {
		if (format == kMessageMagicR5)
			return BPrivate::R5MessageUnflatten(this, stream);

		if (format == kMessageMagicDano)
			return BPrivate::unflatten_dano_message(format, *stream, *this);
	}

	free(fHeader);
	fHeader = (MessageHeader *)malloc(sizeof(MessageHeader));
	if (!fHeader)
		return B_NO_MEMORY;

	fHeader->format = format;
	uint8 *header = (uint8 *)fHeader;
	ssize_t result = stream->Read(header + sizeof(uint32),
		sizeof(MessageHeader) - sizeof(uint32));
	if (result != sizeof(MessageHeader) - sizeof(uint32)) {
		_Clear();
		_InitHeader();
		return (result >= 0 ? B_ERROR : result);
	}

	/* ToDo: better message validation (checksum) */
	if (fHeader->format != kMessageMagic4 ||
		!(fHeader->flags & MESSAGE_FLAG_VALID)) {
		_Clear();
		_InitHeader();
		return B_BAD_VALUE;
	}

	free(fFields);
	fFields = NULL;
	if (fHeader->fieldsSize > 0) {
		fFields = (FieldHeader *)malloc(fHeader->fieldsSize);
		if (!fFields)
			return B_NO_MEMORY;

		result = stream->Read(fFields, fHeader->fieldsSize);
		if (result != fHeader->fieldsSize) {
			_Clear();
			_InitHeader();
			return (result >= 0 ? B_ERROR : result);
		}
	}

	free(fData);
	fData = NULL;
	if (fHeader->dataSize > 0) {
		fData = (uint8 *)malloc(fHeader->dataSize);
		if (!fData)
			return B_NO_MEMORY;

		result = stream->Read(fData, fHeader->dataSize);
		if (result != fHeader->dataSize) {
			_Clear();
			_InitHeader();
			return (result >= 0 ? B_ERROR : result);
		}
	}

	fHeader->fieldsAvailable = 0;
	fHeader->dataAvailable = 0;
	what = fHeader->what;
	return B_OK;
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

	fHeader->currentSpecifier++;
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

	fHeader->currentSpecifier = index;
	return B_OK;
}


status_t
BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier, int32 *what,
	const char **property) const
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader->currentSpecifier < 0
		|| !(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_SCRIPT_SYNTAX;

	if (index)
		*index = fHeader->currentSpecifier;

	if (specifier) {
		if (FindMessage(B_SPECIFIER_ENTRY, fHeader->currentSpecifier,
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
	DEBUG_FUNCTION_ENTER;
	return fHeader->flags & MESSAGE_FLAG_HAS_SPECIFIERS;
}


status_t
BMessage::PopSpecifier()
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader->currentSpecifier < 0 ||
		!(fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED))
		return B_BAD_VALUE;

	if (fHeader->currentSpecifier >= 0)
		fHeader->currentSpecifier--;

	return B_OK;
}


status_t
BMessage::_ResizeData(int32 offset, int32 change)
{
	if (change == 0)
		return B_OK;

	/* optimize for the most usual case: appending data */
	if (offset < fHeader->dataSize) {
		FieldHeader *field = fFields;
		for (int32 i = 0; i < fHeader->fieldCount; i++, field++) {
			if (field->offset >= offset)
				field->offset += change;
		}
	}

	if (change > 0) {
		if (fHeader->dataAvailable >= change) {
			if (offset < fHeader->dataSize) {
				ssize_t length = fHeader->dataSize - offset;
				memmove(fData + offset + change, fData + offset, length);
			}

			fHeader->dataAvailable -= change;
			fHeader->dataSize += change;
			return B_OK;
		}

		ssize_t size = fHeader->dataSize * 2;
		size = min_c(size, fHeader->dataSize + MAX_DATA_PREALLOCATION);
		size = max_c(size, fHeader->dataSize + change);

		fData = (uint8 *)realloc(fData, size);
		if (!fData)
			return B_NO_MEMORY;

		if (offset < fHeader->dataSize) {
			memmove(fData + offset + change, fData + offset,
				fHeader->dataSize - offset - change);
		}

		fHeader->dataSize += change;
		fHeader->dataAvailable = size - fHeader->dataSize;
	} else {
		ssize_t size = fHeader->dataSize;
		memmove(fData + offset, fData + offset - change, size - offset + change);
		fHeader->dataSize += change;
		fHeader->dataAvailable -= change;

		if (fHeader->dataAvailable > MAX_DATA_PREALLOCATION) {
			ssize_t available = MAX_DATA_PREALLOCATION / 2;
			fData = (uint8 *)realloc(fData, fHeader->dataSize + available);
			if (!fData)
				return B_NO_MEMORY;

			fHeader->dataAvailable = available;
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
BMessage::_FindField(const char *name, type_code type, FieldHeader **result) const
{
	if (!name)
		return B_BAD_VALUE;

	if (!fHeader || !fFields || !fData)
		return B_NAME_NOT_FOUND;

	uint32 hash = _HashName(name) % fHeader->hashTableSize;
	int32 nextField = fHeader->hashTable[hash];

	while (nextField >= 0) {
		FieldHeader *field = &fFields[nextField];

		if (strncmp((const char *)(fData + field->offset), name,
			field->nameLength) == 0) {
			if (type != B_ANY_TYPE && field->type != type)
				return B_BAD_TYPE;

			*result = field;
			return B_OK;
		}

		nextField = field->nextField;
	}

	return B_NAME_NOT_FOUND;
}


status_t
BMessage::_AddField(const char *name, type_code type, bool isFixedSize,
	FieldHeader **result)
{
	if (!fHeader)
		return B_ERROR;

	if (fHeader->fieldsAvailable <= 0) {
		int32 count = fHeader->fieldCount * 2 + 1;
		count = min_c(count, fHeader->fieldCount + MAX_FIELD_PREALLOCATION);

		fFields = (FieldHeader *)realloc(fFields, count * sizeof(FieldHeader));
		if (!fFields)
			return B_NO_MEMORY;

		fHeader->fieldsAvailable = count - fHeader->fieldCount;
	}

	uint32 hash = _HashName(name) % fHeader->hashTableSize;

	int32 *nextField = &fHeader->hashTable[hash];
	while (*nextField >= 0)
		nextField = &fFields[*nextField].nextField;
	*nextField = fHeader->fieldCount;

	FieldHeader *field = &fFields[fHeader->fieldCount];
	field->type = type;
	field->flags = FIELD_FLAG_VALID;
	if (isFixedSize)
		field->flags |= FIELD_FLAG_FIXED_SIZE;

	field->count = 0;
	field->dataSize = 0;
	field->allocated = 0;
	field->nextField = -1;
	field->offset = fHeader->dataSize;
	field->nameLength = strlen(name) + 1;
	_ResizeData(field->offset, field->nameLength);
	memcpy(fData + field->offset, name, field->nameLength);

	fHeader->fieldsAvailable--;
	fHeader->fieldsSize += sizeof(FieldHeader);
	fHeader->fieldCount++;
	*result = field;
	return B_OK;
}


status_t
BMessage::_RemoveField(FieldHeader *field)
{
	int32 index = ((uint8 *)field - (uint8 *)fFields) / sizeof(FieldHeader);
	int32 nextField = field->nextField;
	if (nextField > index)
		nextField--;

	int32 *value = fHeader->hashTable;
	for (int32 i = 0; i < fHeader->hashTableSize; i++, value++) {
		if (*value > index)
			*value -= 1;
		else if (*value == index)
			*value = nextField;
	}

	FieldHeader *other = fFields;
	for (int32 i = 0; i < fHeader->fieldCount; i++, other++) {
		if (other->nextField > index)
			other->nextField--;
		else if (other->nextField == index)
			other->nextField = nextField;
	}

	_ResizeData(field->offset, -(field->dataSize + field->nameLength));

	ssize_t size = fHeader->fieldsSize - (index + 1) * sizeof(FieldHeader);
	memmove(fFields + index, fFields + index + 1, size);
	fHeader->fieldsSize -= sizeof(FieldHeader);
	fHeader->fieldCount--;
	fHeader->fieldsAvailable++;

	if (fHeader->fieldsAvailable > MAX_FIELD_PREALLOCATION) {
		ssize_t available = MAX_FIELD_PREALLOCATION / 2;
		fFields = (FieldHeader *)realloc(fFields, fHeader->fieldsSize
			+ available * sizeof(FieldHeader));
		if (!fFields)
			return B_NO_MEMORY;

		fHeader->fieldsAvailable = available;
	}

	return B_OK;
}


status_t
BMessage::AddData(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool isFixedSize, int32 count)
{
	DEBUG_FUNCTION_ENTER;
	if (numBytes <= 0 || !data)
		return B_BAD_VALUE;

	FieldHeader *field = NULL;
	status_t result = _FindField(name, type, &field);
	if (result == B_NAME_NOT_FOUND)
		result = _AddField(name, type, isFixedSize, &field);

	if (result < B_OK || !field)
		return result;

	uint32 offset = field->offset + field->nameLength + field->dataSize;
	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		if (field->count) {
			ssize_t size = field->dataSize / field->count;
			if (size != numBytes)
				return B_BAD_VALUE;
		}

		_ResizeData(offset, numBytes);
		memcpy(fData + offset, data, numBytes);
		field->dataSize += numBytes;
	} else {
		int32 change = numBytes + sizeof(numBytes);
		_ResizeData(offset, change);
		memcpy(fData + offset, &numBytes, sizeof(numBytes));
		memcpy(fData + offset + sizeof(numBytes), data, numBytes);
		field->dataSize += change;
	}

	field->count++;
	return B_OK;
}


status_t
BMessage::RemoveData(const char *name, int32 index)
{
	DEBUG_FUNCTION_ENTER;
	if (index < 0)
		return B_BAD_VALUE;

	FieldHeader *field = NULL;
	status_t result = _FindField(name, B_ANY_TYPE, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->count == 1)
		return _RemoveField(field);

	uint32 offset = field->offset + field->nameLength;
	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		ssize_t size = field->dataSize / field->count;
		_ResizeData(offset + index * size, -size);
		field->dataSize -= size;
	} else {
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(ssize_t *)pointer + sizeof(ssize_t);
			pointer = fData + offset;
		}

		ssize_t currentSize = *(ssize_t *)pointer + sizeof(ssize_t);
		_ResizeData(offset, -currentSize);
		field->dataSize -= currentSize;
	}

	field->count--;
	return B_OK;
}


status_t
BMessage::RemoveName(const char *name)
{
	DEBUG_FUNCTION_ENTER;
	FieldHeader *field = NULL;
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
	DEBUG_FUNCTION_ENTER;

	free(fFields);
	fFields = NULL;
	free(fData);
	fData = NULL;

	fHeader->fieldsSize = 0;
	fHeader->dataSize = 0;
	fHeader->fieldsAvailable = 0;
	fHeader->dataAvailable = 0;
	fHeader->currentSpecifier = -1;
	fHeader->fieldCount = 0;

	fHeader->flags &= ~MESSAGE_FLAG_HAS_SPECIFIERS;

	// initializing the hash table to -1 because 0 is a valid index
	memset(&fHeader->hashTable, 255, sizeof(fHeader->hashTable));
	return B_OK;
}


status_t
BMessage::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *numBytes) const
{
	DEBUG_FUNCTION_ENTER;
	if (!data || !numBytes)
		return B_BAD_VALUE;

	*data = NULL;
	FieldHeader *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		*numBytes = field->dataSize / field->count;
		*data = fData + field->offset + field->nameLength + index * *numBytes;
	} else {
		uint8 *pointer = fData + field->offset + field->nameLength;

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
	DEBUG_FUNCTION_ENTER;
	if (numBytes <= 0 || !data)
		return B_BAD_VALUE;

	FieldHeader *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return result;

	if (!field)
		return B_ERROR;

	if (index >= field->count)
		return B_BAD_INDEX;

	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		ssize_t size = field->dataSize / field->count;
		if (size != numBytes)
			return B_BAD_VALUE;

		memcpy(fData + field->offset + field->nameLength + index * size, data,
			size);
	} else {
		uint32 offset = field->offset + field->nameLength;
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(ssize_t *)pointer + sizeof(ssize_t);
			pointer = fData + offset;
		}

		ssize_t currentSize = *(ssize_t *)pointer;
		int32 change = numBytes - currentSize;
		_ResizeData(offset, change);
		memcpy(fData + offset, &numBytes, sizeof(numBytes));
		memcpy(fData + offset + sizeof(numBytes), data, numBytes);
		field->dataSize += change;
	}

	return B_OK;
}


bool
BMessage::HasData(const char *name, type_code type, int32 index) const
{
	DEBUG_FUNCTION_ENTER;
	FieldHeader *field = NULL;
	status_t result = _FindField(name, type, &field);

	if (result < B_OK)
		return false;

	if (!field)
		return false;

	if (index >= field->count)
		return false;

	return true;
}


/* Static functions for cache initialization and cleanup */
void
BMessage::_StaticInit()
{
	DEBUG_FUNCTION_ENTER;
	sReplyPorts[0] = create_port(1, "tmp_rport0");
	sReplyPorts[1] = create_port(1, "tmp_rport1");
	sReplyPorts[2] = create_port(1, "tmp_rport2");

	sReplyPortInUse[0] = 0;
	sReplyPortInUse[1] = 0;
	sReplyPortInUse[2] = 0;

	sMsgCache = NULL;
}


void
BMessage::_StaticCleanup()
{
	DEBUG_FUNCTION_ENTER;
	delete_port(sReplyPorts[0]);
	sReplyPorts[0] = -1;
	delete_port(sReplyPorts[1]);
	sReplyPorts[1] = -1;
	delete_port(sReplyPorts[2]);
	sReplyPorts[2] = -1;
}


void
BMessage::_StaticCacheCleanup()
{
	DEBUG_FUNCTION_ENTER;
	delete sMsgCache;
	sMsgCache = NULL;
}


int32
BMessage::_StaticGetCachedReplyPort()
{
	DEBUG_FUNCTION_ENTER;
	int index = -1;
	for (int32 i = 0; i < sNumReplyPorts; i++) {
		int32 old = atomic_add(&(sReplyPortInUse[i]), 1);
		if (old == 0) {
			// This entry is free
			index = i;
			break;
		} else {
			// This entry is being used.
			atomic_add(&(sReplyPortInUse[i]), -1);
		}
	}

	return index;
}


status_t
BMessage::_SendMessage(port_id port, int32 token, bool preferred,
	bigtime_t timeout, bool replyRequired, BMessenger &replyTo) const
{
	DEBUG_FUNCTION_ENTER;
	uint32 oldFlags = fHeader->flags;
	int32 oldTarget = fHeader->target;
	port_id oldReplyPort = fHeader->replyPort;
	int32 oldReplyTarget = fHeader->replyTarget;
	team_id oldReplyTeam = fHeader->replyTeam;

	if (!replyTo.IsValid()) {
		BMessenger::Private(replyTo).SetTo(fHeader->replyTeam,
			fHeader->replyPort, fHeader->replyTarget,
			fHeader->replyTarget == B_PREFERRED_TOKEN);

		if (!replyTo.IsValid())
			replyTo = be_app_messenger;
	}

	BMessenger::Private replyToPrivate(replyTo);

	if (replyRequired)
		fHeader->flags |= MESSAGE_FLAG_REPLY_REQUIRED;
	else
		fHeader->flags &= ~MESSAGE_FLAG_REPLY_REQUIRED;

	fHeader->target = (preferred ? B_PREFERRED_TOKEN : token);
	fHeader->replyTeam = replyToPrivate.Team();
	fHeader->replyPort = replyToPrivate.Port();
	fHeader->replyTarget = (replyToPrivate.IsPreferredTarget()
		? B_PREFERRED_TOKEN : replyToPrivate.Token());
	fHeader->flags |= MESSAGE_FLAG_WAS_DELIVERED;

	/* ToDo: we can use _kern_writev_port to send the three parts directly:
		iovec vectors[3];
		vectors[0].iov_base = fHeader;
		vectors[0].iov_len = sizeof(MessageHeader);
		vectors[1].iov_base = fFields;
		vectors[1].iov_len = fHeader->fieldsSize;
		vectors[2].iov_base = fData;
		vectors[2].iov_len = fHeader->dataSize;
		status_t result;

		do {
			result = _kern_writev_port_etc(port, 'pjpp', vectors, 3,
				_NativeFlattenedSize(), B_RELATIVE_TIMEOUT, timeout);
		} while (result == B_INTERRUPTED);
	*/

	ssize_t size = _NativeFlattenedSize();
	char *buffer = new char[size];
	status_t result = _NativeFlatten(buffer, size);
	if (result < B_OK)
		goto error;

	do {
		result = write_port_etc(port, 'pjpp', buffer, size,
			B_RELATIVE_TIMEOUT, timeout);
	} while (result == B_INTERRUPTED);

error:
	fHeader->flags = oldFlags;
	fHeader->target = oldTarget;
	fHeader->replyPort = oldReplyPort;
	fHeader->replyTarget = oldReplyTarget;
	fHeader->replyTeam = oldReplyTeam;
	delete[] buffer;
	return result;
}


status_t
BMessage::_SendMessage(port_id port, team_id portOwner, int32 token,
	bool preferred, BMessage *reply, bigtime_t sendTimeout,
	bigtime_t replyTimeout) const
{
	DEBUG_FUNCTION_ENTER;
	const int32 cachedReplyPort = _StaticGetCachedReplyPort();
	port_id replyPort = B_BAD_PORT_ID;
	status_t result = B_OK;

	if (cachedReplyPort < B_OK) {
		// All the cached reply ports are in use; create a new one
		replyPort = create_port(1 /* for one message */, "tmp_reply_port");
		if (replyPort < B_OK)
			return replyPort;
	} else {
		assert(cachedReplyPort < sNumReplyPorts);
		replyPort = sReplyPorts[cachedReplyPort];
	}

	team_id team = B_BAD_TEAM_ID;
	if (be_app != NULL)
		team = be_app->Team();
	else {
		port_info portInfo;
		result = get_port_info(replyPort, &portInfo);
		if (result < B_OK)
			goto error;

		team = portInfo.team;
	}

	result = set_port_owner(replyPort, portOwner);
	if (result < B_OK)
		goto error;

	{
		BMessenger messenger;
		BMessenger::Private(messenger).SetTo(team, replyPort,
			B_PREFERRED_TOKEN, true);
		result = _SendMessage(port, token, preferred, sendTimeout, true,
			messenger);
	}

	if (result < B_OK)
		goto error;

	int32 code;
	result = handle_reply(replyPort, &code, replyTimeout, reply);
	if (result < B_OK && cachedReplyPort >= 0) {
		delete_port(replyPort);
		sReplyPorts[cachedReplyPort] = create_port(1, "tmp_rport");
	}

error:
	if (cachedReplyPort >= 0) {
		// Reclaim ownership of cached port
		set_port_owner(replyPort, team);
		// Flag as available
		atomic_add(&sReplyPortInUse[cachedReplyPort], -1);
		return result;
	}

	delete_port(replyPort);
	return result;
}


status_t
BMessage::_SendFlattenedMessage(void *data, int32 size, port_id port,
	int32 token, bool preferred, bigtime_t timeout)
{
	DEBUG_FUNCTION_ENTER;
	if (!data)
		return B_BAD_VALUE;

	uint32 magic = *(uint32*)data;

	if (magic == kMessageMagic4 || magic == kMessageMagic4Swapped) {
		MessageHeader *header = (MessageHeader *)data;
		header->target = (preferred ? B_PREFERRED_TOKEN : token);
	} else if (magic == kMessageMagicR5) {
		uint8 *header = (uint8 *)data;
		header += sizeof(uint32) /* magic */ + sizeof(uint32) /* checksum */
			+ sizeof(ssize_t) /* flattenedSize */ + sizeof(int32) /* what */
			+ sizeof(uint8) /* flags */;
		*(int32 *)header = (preferred ? B_PREFERRED_TOKEN : token);
	} else if (((KMessage::Header *)data)->magic == KMessage::kMessageHeaderMagic) {
		KMessage::Header *header = (KMessage::Header *)data;
		header->targetToken = (preferred ? B_PREFERRED_TOKEN : token);
	} else {
		return B_NOT_A_MESSAGE;
	}

	// send the message
	status_t result;

	do {
		result = write_port_etc(port, 'pjpp', data, size, B_RELATIVE_TIMEOUT,
			timeout);
	} while (result == B_INTERRUPTED);

	return result;
}


static status_t
handle_reply(port_id replyPort, int32 *pCode, bigtime_t timeout,
	BMessage *reply)
{
	DEBUG_FUNCTION_ENTER;
	status_t result;
	do {
		result = port_buffer_size_etc(replyPort, B_RELATIVE_TIMEOUT, timeout);
	} while (result == B_INTERRUPTED);

	if (result < B_OK)
		return result;

	// The API lied. It really isn't an error code, but the message size...
	char *buffer = new char[result];

	do {
		result = read_port(replyPort, pCode, buffer, result);
	} while (result == B_INTERRUPTED);

	if (result < B_OK || *pCode != 'pjpp') {
		delete[] buffer;
		return (result < B_OK ? result : B_ERROR);
	}

	result = reply->Unflatten(buffer);
	delete[] buffer;
	return result;
}


static status_t
convert_message(const KMessage *fromMessage, BMessage *toMessage)
{
	DEBUG_FUNCTION_ENTER;
	if (!fromMessage || !toMessage)
		return B_BAD_VALUE;

	// make empty and init what of the target message
	toMessage->MakeEmpty();
	toMessage->what = fromMessage->What();

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
						result = convert_message(&message, &bMessage);
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
	return AddData(name, B_STRING_TYPE, string, strlen(string) + 1, false);
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
