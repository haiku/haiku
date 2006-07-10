/*
 * Copyright 2005-2006, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */


#include "dano_message.h"
#include "r5_message.h"

#include <Message.h>
#include <MessagePrivate.h>
#include <MessageUtils.h>

#include <Application.h>
#include <AppMisc.h>
#include <BlockCache.h>
#include <Entry.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <Path.h>
#include <Point.h>
#include <Rect.h>
#include <String.h>
#include <TokenSpace.h>
#include <util/KMessage.h>

#include <ctype.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>


#define DEBUG_FUNCTION_ENTER	//debug_printf("thread: 0x%x; this: 0x%08x; header: 0x%08x; fields: 0x%08x; data: 0x%08x; line: %04ld; func: %s\n", find_thread(NULL), this, fHeader, fFields, fData, __LINE__, __PRETTY_FUNCTION__);
#define DEBUG_FUNCTION_ENTER2	//debug_printf("thread: 0x%x;                                                                             line: %04ld: func: %s\n", find_thread(NULL), __LINE__, __PRETTY_FUNCTION__);


static const uint32 kMessageMagicR5 = 'FOB1';
static const uint32 kMessageMagicR5Swapped = '1BOF';
static const uint32 kMessageMagicDano = 'FOB2';
static const uint32 kMessageMagicDanoSwapped = '2BOF';
static const uint32 kMessageMagicHaiku = '1FMH';
static const uint32 kMessageMagicHaikuSwapped = 'HMF1';


const char *B_SPECIFIER_ENTRY = "specifiers";
const char *B_PROPERTY_ENTRY = "property";
const char *B_PROPERTY_NAME_ENTRY = "name";


static status_t		handle_reply(port_id replyPort, int32 *pCode,
						bigtime_t timeout, BMessage *reply);
static status_t		convert_message(const KMessage *fromMessage,
						BMessage *toMessage);

extern "C" {
		// private os function to set the owning team of an area
		status_t	_kern_transfer_area(area_id area, void **_address,
						uint32 addressSpec, team_id target);
}


BBlockCache *BMessage::sMsgCache = NULL;
port_id BMessage::sReplyPorts[sNumReplyPorts];
long BMessage::sReplyPortInUse[sNumReplyPorts];


BMessage::BMessage()
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
}

BMessage::BMessage(BMessage *other)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
	*this = *other;
}

BMessage::BMessage(uint32 _what)
{
	DEBUG_FUNCTION_ENTER;
	_InitCommon();
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

	fHeader = (message_header *)malloc(sizeof(message_header));
	memcpy(fHeader, other.fHeader, sizeof(message_header));

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
	DEBUG_FUNCTION_ENTER2;
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


status_t
BMessage::_InitCommon()
{
	DEBUG_FUNCTION_ENTER;
	what = 0;

	fHeader = NULL;
	fFields = NULL;
	fData = NULL;

	fClonedArea = -1;

	fOriginal = NULL;
	fQueueLink = NULL;

	return _InitHeader();
}


status_t
BMessage::_InitHeader()
{
	DEBUG_FUNCTION_ENTER;
	fHeader = (message_header *)malloc(sizeof(message_header));
	memset(fHeader, 0, sizeof(message_header) - sizeof(fHeader->hash_table));

	fHeader->format = kMessageMagicHaiku;
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
	DEBUG_FUNCTION_ENTER;
	if (fClonedArea >= B_OK)
		_Dereference();

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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;
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
	DEBUG_FUNCTION_ENTER;

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
		if (field->flags & FIELD_FLAG_FIXED_SIZE)
			size = field->data_size / field->count;

		uint8 *pointer = fData + field->offset + field->name_length;

		for (int32 j=0; j<field->count; j++) {
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
					printf("string(\"%s\", %ld bytes)\n", (char *)pointer, size);
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
							ref.device, ref.directory, ref.name);
	
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
					if (message.Unflatten((const char *)pointer)!=B_OK) {
						fprintf(stderr, "couldn't unflatten item %ld\n", i);
						break;
					}
					message._PrintToStream(buffer);
					printf("%s        }\n", indent);
					pointer += size;
					break;
				}
				
				default: {
					printf("(type = '%.4s')(size = %ld)\n", (char *)&value, size);
				}
			}
		}
	}
}


status_t
BMessage::Rename(const char *oldEntry, const char *newEntry)
{
	DEBUG_FUNCTION_ENTER;
	if (!oldEntry || !newEntry)
		return B_BAD_VALUE;

	if (fClonedArea >= B_OK)
		_CopyForWrite();

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

			int32 oldLength = field->name_length;
			field->name_length = strlen(newEntry) + 1;
			_ResizeData(field->offset, field->name_length - oldLength);
			memcpy(fData + field->offset, newEntry, field->name_length);
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
		&& (fHeader->reply_team != BPrivate::current_team());
}


BMessenger
BMessage::ReturnAddress() const
{
	DEBUG_FUNCTION_ENTER;
	if (fHeader->flags & MESSAGE_FLAG_WAS_DELIVERED) {
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
	DEBUG_FUNCTION_ENTER;
	return fHeader->flags & MESSAGE_FLAG_WAS_DROPPED;
}


BPoint
BMessage::DropPoint(BPoint *offset) const
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
	messengerPrivate.SetTo(fHeader->reply_team, fHeader->reply_port,
		fHeader->reply_target);

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
	messengerPrivate.SetTo(fHeader->reply_team, fHeader->reply_port,
		fHeader->reply_target);

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
	return BPrivate::r5_message_flattened_size(this);
}


status_t
BMessage::Flatten(char *buffer, ssize_t size) const
{
	DEBUG_FUNCTION_ENTER;
	return BPrivate::flatten_r5_message(this, buffer, size);
}


status_t
BMessage::Flatten(BDataIO *stream, ssize_t *size) const
{
	DEBUG_FUNCTION_ENTER;
	return BPrivate::flatten_r5_message(this, stream, size);
}


ssize_t
BMessage::_NativeFlattenedSize() const
{
	DEBUG_FUNCTION_ENTER;
	return sizeof(message_header) + fHeader->fields_size + fHeader->data_size;
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
	//fHeader->fields_checksum = BPrivate::CalculateChecksum((uint8 *)fFields, fHeader->fields_size);
	//fHeader->data_checksum = BPrivate::CalculateChecksum((uint8 *)fData, fHeader->data_size);

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
	DEBUG_FUNCTION_ENTER;
	if (!stream)
		return B_BAD_VALUE;

	if (!fHeader)
		return B_NO_INIT;

	/* we have to sync the what code as it is a public member */
	fHeader->what = what;
	//fHeader->fields_checksum = BPrivate::CalculateChecksum((uint8 *)fFields, fHeader->fields_size);
	//fHeader->data_checksum = BPrivate::CalculateChecksum((uint8 *)fData, fHeader->data_size);

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


/*	The concept of message sending by area:

	The traditional way of sending a message is to send it by flattening it to
	a buffer, pushing it through a port, reading it into the outputbuffer and
	unflattening it from there (copying the data again). While this works ok
	for small messages it does not make any sense for larger ones and may even
	hit some port capacity limit.
	Often in the life of a BMessage, it will be sent to someone. Almost as
	often the one receiving the message will not need to change the message
	in any way, but uses it "read only" to get information from it. This means
	that all that copying is pretty pointless in the first place since we
	could simply pass the original buffers on.
	It's obviously not exactly as simple as this, since we cannot just use the
	memory of one application in another - but we can share areas with
	eachother.
	Therefore instead of flattening into a buffer, we copy the message data
	into an area, put this information into the message header and only push
	this through the port. The receiving looper then builds a BMessage from
	the header, that only references the data in the area (not copying it),
	allowing read only access to it.
	Only if write access is necessary the message will be copyed from the area
	to its own buffers (like in the unflatten step before).
	The double copying is reduced to a single copy in most cases and we safe
	the slower route of moving the data through a port.
	Additionally we save us the reference counting with the use of areas that
	are reference counted internally. So we don't have to worry about leaving
	an area behind or deleting one that is still in use.
*/

status_t
BMessage::_FlattenToArea(message_header **_header) const
{
	DEBUG_FUNCTION_ENTER;
	message_header *header = (message_header *)malloc(sizeof(message_header));
	memcpy(header, fHeader, sizeof(message_header));

	header->what = what;
	header->fields_available = 0;
	header->data_available = 0;
	header->flags |= MESSAGE_FLAG_PASS_BY_AREA;
	*_header = header;

	if (header->shared_area >= B_OK)
		return B_OK;

	if (header->fields_size == 0 && header->data_size == 0)
		return B_OK;

	uint8 *address = NULL;
	ssize_t size = header->fields_size + header->data_size;
	size = (size + B_PAGE_SIZE) & ~(B_PAGE_SIZE - 1);
	area_id area = create_area("Shared BMessage data", (void **)&address,
		B_ANY_ADDRESS, size, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);

	if (area < B_OK) {
		free(header);
		*_header = NULL;
		return area;
	}

	if (header->fields_size > 0) {
		memcpy(address, fFields, header->fields_size);
		header->fields_checksum = BPrivate::CalculateChecksum((uint8 *)address, header->fields_size);
		address += header->fields_size;
	}

	if (header->data_size > 0) {
		memcpy(address, fData, header->data_size);
		header->data_checksum = BPrivate::CalculateChecksum((uint8 *)address, header->data_size);
	}

	header->shared_area = area;
	return B_OK;
}


status_t
BMessage::_Reference(message_header *header)
{
	DEBUG_FUNCTION_ENTER;
	fHeader = header;
	fHeader->flags &= ~MESSAGE_FLAG_PASS_BY_AREA;

	/* if there is no data at all we don't need the area */
	if (fHeader->fields_size == 0 && header->data_size == 0)
		return B_OK;

	uint8 *address = NULL;
	area_id clone = clone_area("Cloned BMessage data", (void **)&address,
		B_ANY_ADDRESS, B_READ_AREA, fHeader->shared_area);

	if (clone < B_OK) {
		free(fHeader);
		fHeader = NULL;
		_InitHeader();
		return clone;
	}

	fClonedArea = clone;
	fFields = (field_header *)address;
	address += fHeader->fields_size;
	fData = address;
	return B_OK;
}


status_t
BMessage::_Dereference()
{
	DEBUG_FUNCTION_ENTER;
	delete_area(fClonedArea);
	fClonedArea = -1;
	fFields = NULL;
	fData = NULL;
	return B_OK;
}


status_t
BMessage::_CopyForWrite()
{
	DEBUG_FUNCTION_ENTER;
	if (fClonedArea < B_OK)
		return B_OK;

	field_header *newFields = NULL;
	uint8 *newData = NULL;

	if (fHeader->fields_size > 0) {
		newFields = (field_header *)malloc(fHeader->fields_size);
		memcpy(newFields, fFields, fHeader->fields_size);
	}

	if (fHeader->data_size > 0) {
		newData = (uint8 *)malloc(fHeader->data_size);
		memcpy(newData, fData, fHeader->data_size);
	}

	_Dereference();

	fHeader->fields_available = 0;
	fHeader->data_available = 0;

	fFields = newFields;
	fData = newData;
	return B_OK;
}


status_t
BMessage::Unflatten(const char *flatBuffer)
{
	DEBUG_FUNCTION_ENTER;
	if (!flatBuffer)
		return B_BAD_VALUE;

	uint32 format = *(uint32 *)flatBuffer;
	if (format != kMessageMagicHaiku) {
		if (format == KMessage::kMessageHeaderMagic) {
			KMessage message;
			status_t result = message.SetTo(flatBuffer,
				((KMessage::Header*)flatBuffer)->size);
			if (result != B_OK)
				return result;

			return convert_message(&message, this);
		}

		try {
			if (format == kMessageMagicR5 || format == kMessageMagicR5Swapped)
				return BPrivate::unflatten_r5_message(format, this, flatBuffer);

			if (format == kMessageMagicDano || format == kMessageMagicDanoSwapped) {
				BMemoryIO stream(flatBuffer + sizeof(uint32),
					BPrivate::dano_message_flattened_size(flatBuffer));
				return BPrivate::unflatten_dano_message(format, stream, *this);
			}
		} catch (status_t error) {
			MakeEmpty();
			return error;
		}

		return B_NOT_A_MESSAGE;
	}

	// native message unflattening

	_Clear();

	fHeader = (message_header *)malloc(sizeof(message_header));
	if (!fHeader)
		return B_NO_MEMORY;

	memcpy(fHeader, flatBuffer, sizeof(message_header));
	flatBuffer += sizeof(message_header);

	if (fHeader->format != kMessageMagicHaiku
		|| !(fHeader->flags & MESSAGE_FLAG_VALID)) {
		free(fHeader);
		fHeader = NULL;
		_InitHeader();
		return B_BAD_VALUE;
	}

	fHeader->fields_available = 0;
	fHeader->data_available = 0;
	what = fHeader->what;

	if (fHeader->flags & MESSAGE_FLAG_PASS_BY_AREA) {
		status_t result = _Reference(fHeader);
		if (result < B_OK)
			return result;
	} else {
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
	}

	/*if (fHeader->fields_checksum != BPrivate::CalculateChecksum((uint8 *)fFields, fHeader->fields_size)
		|| fHeader->data_checksum != BPrivate::CalculateChecksum((uint8 *)fData, fHeader->data_size)) {
		debug_printf("checksum mismatch 1\n");
		_Clear();
		_InitHeader();
		return B_BAD_VALUE;
	}*/

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
	if (format != kMessageMagicHaiku) {
		try {
			if (format == kMessageMagicR5 || format == kMessageMagicR5Swapped)
				return BPrivate::unflatten_r5_message(format, this, stream);

			if (format == kMessageMagicDano || format == kMessageMagicDanoSwapped)
				return BPrivate::unflatten_dano_message(format, *stream, *this);
		} catch (status_t error) {
			MakeEmpty();
			return error;
		}

		return B_NOT_A_MESSAGE;
	}

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

	if (result != B_OK || fHeader->format != kMessageMagicHaiku
		|| !(fHeader->flags & MESSAGE_FLAG_VALID)) {
		free(fHeader);
		fHeader = NULL;
		_InitHeader();
		return B_BAD_VALUE;
	}

	fHeader->fields_available = 0;
	fHeader->data_available = 0;
	what = fHeader->what;

	if (fHeader->flags & MESSAGE_FLAG_PASS_BY_AREA) {
		result = _Reference(fHeader);
		if (result < B_OK)
			return result;
	} else {
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
	}

	/*if (fHeader->fields_checksum != BPrivate::CalculateChecksum((uint8 *)fFields, fHeader->fields_size)
		|| fHeader->data_checksum != BPrivate::CalculateChecksum((uint8 *)fData, fHeader->data_size)) {
		debug_printf("checksum mismatch 2\n");
		_Clear();
		_InitHeader();
		return B_BAD_VALUE;
	}*/

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
BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier, int32 *what,
	const char **property) const
{
	DEBUG_FUNCTION_ENTER;

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
	DEBUG_FUNCTION_ENTER;
	return fHeader->flags & MESSAGE_FLAG_HAS_SPECIFIERS;
}


status_t
BMessage::PopSpecifier()
{
	DEBUG_FUNCTION_ENTER;
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

		fData = (uint8 *)realloc(fData, size);
		if (!fData)
			return B_NO_MEMORY;

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
			fData = (uint8 *)realloc(fData, fHeader->data_size + available);
			if (!fData)
				return B_NO_MEMORY;

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

		fFields = (field_header *)realloc(fFields, count * sizeof(field_header));
		if (!fFields)
			return B_NO_MEMORY;

		fHeader->fields_available = count - fHeader->field_count;
	}

	uint32 hash = _HashName(name) % fHeader->hash_table_size;

	int32 *nextField = &fHeader->hash_table[hash];
	while (*nextField >= 0)
		nextField = &fFields[*nextField].next_field;
	*nextField = fHeader->field_count;

	field_header *field = &fFields[fHeader->field_count];
	field->type = type;
	field->flags = FIELD_FLAG_VALID;
	if (isFixedSize)
		field->flags |= FIELD_FLAG_FIXED_SIZE;

	field->count = 0;
	field->data_size = 0;
	field->allocated = 0;
	field->next_field = -1;
	field->offset = fHeader->data_size;
	field->name_length = strlen(name) + 1;
	_ResizeData(field->offset, field->name_length);
	memcpy(fData + field->offset, name, field->name_length);

	fHeader->fields_available--;
	fHeader->fields_size += sizeof(field_header);
	fHeader->field_count++;
	*result = field;
	return B_OK;
}


status_t
BMessage::_RemoveField(field_header *field)
{
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

	_ResizeData(field->offset, -(field->data_size + field->name_length));

	ssize_t size = fHeader->fields_size - (index + 1) * sizeof(field_header);
	memmove(fFields + index, fFields + index + 1, size);
	fHeader->fields_size -= sizeof(field_header);
	fHeader->field_count--;
	fHeader->fields_available++;

	if (fHeader->fields_available > MAX_FIELD_PREALLOCATION) {
		ssize_t available = MAX_FIELD_PREALLOCATION / 2;
		fFields = (field_header *)realloc(fFields, fHeader->fields_size
			+ available * sizeof(field_header));
		if (!fFields)
			return B_NO_MEMORY;

		fHeader->fields_available = available;
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

	if (fClonedArea >= B_OK)
		_CopyForWrite();

	field_header *field = NULL;
	status_t result = _FindField(name, type, &field);
	if (result == B_NAME_NOT_FOUND)
		result = _AddField(name, type, isFixedSize, &field);

	if (result < B_OK || !field)
		return result;

	uint32 offset = field->offset + field->name_length + field->data_size;
	if (field->flags & FIELD_FLAG_FIXED_SIZE) {
		if (field->count) {
			ssize_t size = field->data_size / field->count;
			if (size != numBytes)
				return B_BAD_VALUE;
		}

		_ResizeData(offset, numBytes);
		memcpy(fData + offset, data, numBytes);
		field->data_size += numBytes;
	} else {
		int32 change = numBytes + sizeof(numBytes);
		_ResizeData(offset, change);
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
	DEBUG_FUNCTION_ENTER;
	if (index < 0)
		return B_BAD_VALUE;

	if (fClonedArea >= B_OK)
		_CopyForWrite();

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
		_ResizeData(offset + index * size, -size);
		field->data_size -= size;
	} else {
		uint8 *pointer = fData + offset;

		for (int32 i = 0; i < index; i++) {
			offset += *(ssize_t *)pointer + sizeof(ssize_t);
			pointer = fData + offset;
		}

		ssize_t currentSize = *(ssize_t *)pointer + sizeof(ssize_t);
		_ResizeData(offset, -currentSize);
		field->data_size -= currentSize;
	}

	field->count--;
	return B_OK;
}


status_t
BMessage::RemoveName(const char *name)
{
	DEBUG_FUNCTION_ENTER;
	if (fClonedArea >= B_OK)
		_CopyForWrite();

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
	DEBUG_FUNCTION_ENTER;
	if (numBytes <= 0 || !data)
		return B_BAD_VALUE;

	if (fClonedArea >= B_OK)
		_CopyForWrite();

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
		_ResizeData(offset, change);
		memcpy(fData + offset, &numBytes, sizeof(numBytes));
		memcpy(fData + offset + sizeof(numBytes), data, numBytes);
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
	DEBUG_FUNCTION_ENTER2;
	sReplyPorts[0] = create_port(1, "tmp_rport0");
	sReplyPorts[1] = create_port(1, "tmp_rport1");
	sReplyPorts[2] = create_port(1, "tmp_rport2");

	sReplyPortInUse[0] = 0;
	sReplyPortInUse[1] = 0;
	sReplyPortInUse[2] = 0;

	sMsgCache = new BBlockCache(20, sizeof(BMessage), B_OBJECT_CACHE);
}


void
BMessage::_StaticCleanup()
{
	DEBUG_FUNCTION_ENTER2;
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
	DEBUG_FUNCTION_ENTER2;
	delete sMsgCache;
	sMsgCache = NULL;
}


int32
BMessage::_StaticGetCachedReplyPort()
{
	DEBUG_FUNCTION_ENTER2;
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
BMessage::_SendMessage(port_id port, int32 token, bigtime_t timeout,
	bool replyRequired, BMessenger &replyTo) const
{
	DEBUG_FUNCTION_ENTER;
	ssize_t size = 0;
	char *buffer = NULL;
	message_header *header = NULL;
	status_t result;

	if (/*fHeader->fields_size + fHeader->data_size > B_PAGE_SIZE*/false) {
		result = _FlattenToArea(&header);
		buffer = (char *)header;
		size = sizeof(message_header);

#ifndef HAIKU_TARGET_PLATFORM_LIBBE_TEST
		port_info info;
		get_port_info(port, &info);
		void *address = NULL;
		_kern_transfer_area(header->shared_area, &address, B_ANY_ADDRESS, 
			info.team);
#endif
	} else {
		size = _NativeFlattenedSize();
		buffer = (char *)malloc(size);
		if (buffer != NULL) {
			result = _NativeFlatten(buffer, size);
			header = (message_header *)buffer;
		} else
			result = B_NO_MEMORY;
	}

	if (result < B_OK)
		return result;

	if (!replyTo.IsValid()) {
		BMessenger::Private(replyTo).SetTo(header->reply_team,
			header->reply_port, header->reply_target);

		if (!replyTo.IsValid())
			replyTo = be_app_messenger;
	}

	BMessenger::Private replyToPrivate(replyTo);

	if (replyRequired) {
		header->flags |= MESSAGE_FLAG_REPLY_REQUIRED;
		header->flags &= ~MESSAGE_FLAG_REPLY_DONE;
	}

	header->target = token;
	header->reply_team = replyToPrivate.Team();
	header->reply_port = replyToPrivate.Port();
	header->reply_target = replyToPrivate.Token();
	header->flags |= MESSAGE_FLAG_WAS_DELIVERED;

	do {
		result = write_port_etc(port, kPortMessageCode, (void *)buffer, size,
			B_RELATIVE_TIMEOUT, timeout);
	} while (result == B_INTERRUPTED);

	if (result == B_OK && IsSourceWaiting()) {
		// the forwarded message will handle the reply - we must not do
		// this anymore
		fHeader->flags |= MESSAGE_FLAG_REPLY_DONE;
	}

	free(buffer);
	return result;
}


status_t
BMessage::_SendMessage(port_id port, team_id portOwner, int32 token,
	BMessage *reply, bigtime_t sendTimeout, bigtime_t replyTimeout) const
{
	if (IsSourceWaiting()) {
		// we can't forward this message synchronously when it's already
		// waiting for a reply
		return B_ERROR;
	}

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

	// tests if the queue of the reply port is really empty
#if 0
	port_info portInfo;
	if (get_port_info(replyPort, &portInfo) == B_OK
		&& portInfo.queue_count > 0)
		debugger("reply port not empty!");
#endif

	{
		BMessenger replyTarget;
		BMessenger::Private(replyTarget).SetTo(team, replyPort,
			B_PREFERRED_TOKEN);
		result = _SendMessage(port, token, sendTimeout, true, replyTarget);
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
	int32 token, bigtime_t timeout)
{
	DEBUG_FUNCTION_ENTER2;
	if (!data)
		return B_BAD_VALUE;

	uint32 magic = *(uint32 *)data;

	if (magic == kMessageMagicHaiku || magic == kMessageMagicHaikuSwapped) {
		message_header *header = (message_header *)data;
		header->target = token;
		header->flags |= MESSAGE_FLAG_WAS_DELIVERED;
	} else if (magic == kMessageMagicR5) {
		uint8 *header = (uint8 *)data;
		header += sizeof(uint32) /* magic */ + sizeof(uint32) /* checksum */
			+ sizeof(ssize_t) /* flattenedSize */ + sizeof(int32) /* what */
			+ sizeof(uint8) /* flags */;
		*(int32 *)header = token;
	} else if (((KMessage::Header *)data)->magic == KMessage::kMessageHeaderMagic) {
		KMessage::Header *header = (KMessage::Header *)data;
		header->targetToken = token;
	} else {
		return B_NOT_A_MESSAGE;
	}

	// send the message
	status_t result;

	do {
		result = write_port_etc(port, kPortMessageCode, data, size,
			B_RELATIVE_TIMEOUT, timeout);
	} while (result == B_INTERRUPTED);

	return result;
}


static status_t
handle_reply(port_id replyPort, int32 *_code, bigtime_t timeout,
	BMessage *reply)
{
	DEBUG_FUNCTION_ENTER2;
	ssize_t size;
	do {
		size = port_buffer_size_etc(replyPort, B_RELATIVE_TIMEOUT, timeout);
	} while (size == B_INTERRUPTED);

	if (size < B_OK)
		return size;

	status_t result;
	char *buffer = (char *)malloc(size);
	do {
		result = read_port(replyPort, _code, buffer, size);
	} while (result == B_INTERRUPTED);

	if (result < B_OK || *_code != kPortMessageCode) {
		free(buffer);
		return result < B_OK ? result : B_ERROR;
	}

	result = reply->Unflatten(buffer);
	free(buffer);
	return result;
}


static status_t
convert_message(const KMessage *fromMessage, BMessage *toMessage)
{
	DEBUG_FUNCTION_ENTER2;
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
