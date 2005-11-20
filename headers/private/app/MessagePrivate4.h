/*
 * Copyright 2005, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef _MESSAGE_PRIVATE_H_
#define _MESSAGE_PRIVATE_H_

#include <Message.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <TokenSpace.h>


#define MESSAGE_BODY_HASH_TABLE_SIZE	10
#define MAX_DATA_PREALLOCATION			B_PAGE_SIZE * 10
#define MAX_FIELD_PREALLOCATION			50
#define MAX_ITEM_PREALLOCATION			B_PAGE_SIZE


enum {
	MESSAGE_FLAG_VALID = 0x0001,
	MESSAGE_FLAG_REPLY_REQUIRED = 0x0002,
	MESSAGE_FLAG_REPLY_DONE = 0x0004,
	MESSAGE_FLAG_IS_REPLY = 0x0008,
	MESSAGE_FLAG_WAS_DELIVERED = 0x0010,
	MESSAGE_FLAG_HAS_SPECIFIERS = 0x0020,
	MESSAGE_FLAG_READ_ONLY = 0x0040
};


enum {
	FIELD_FLAG_VALID = 0x0001,
	FIELD_FLAG_FIXED_SIZE = 0x0002,
};


typedef struct field_header_s {
	uint32		flags;
	type_code	type;
	int32		nameLength;
	int32		count;
	ssize_t		dataSize;
	ssize_t		allocated;
	int32		offset;
	int32		nextField;
} FieldHeader;


typedef struct message_header_s {
	uint32		format;
	uint32		what;
	uint32		flags;

	ssize_t		fieldsSize;
	ssize_t		dataSize;
	ssize_t		fieldsAvailable;
	ssize_t		dataAvailable;

	int32		target;
	int32		currentSpecifier;

	// reply info
	port_id		replyPort;
	int32		replyTarget;
	team_id		replyTeam;

	// body info
	int32		fieldCount;
	int32		hashTableSize;
	int32		hashTable[MESSAGE_BODY_HASH_TABLE_SIZE];

	/*	The hash table does contain indexes into the field list and
		not direct offsets to the fields. This has the advantage
		of not needing to update offsets in two locations.
		The hash table must be reevaluated when we remove a field
		though.
	*/
} MessageHeader;


class BMessage::Private {
	public:
		Private(BMessage *msg)
			: fMessage(msg)
		{
		}

		Private(BMessage &msg)
			: fMessage(&msg)
		{
		}

		void
		SetTarget(int32 token)
		{
			fMessage->fHeader->target = token;
		}

		void
		SetReply(BMessenger messenger)
		{
			BMessenger::Private messengerPrivate(messenger);
			fMessage->fHeader->replyPort = messengerPrivate.Port();
			fMessage->fHeader->replyTarget = messengerPrivate.Token();
			fMessage->fHeader->replyTeam = messengerPrivate.Team();
		}

		int32
		GetTarget()
		{
			return fMessage->fHeader->target;
		}

		bool
		UsePreferredTarget()
		{
			return fMessage->fHeader->target == B_PREFERRED_TOKEN;
		}

		status_t
		Clear()
		{
			return fMessage->_Clear();
		}

		status_t
		InitHeader()
		{
			return fMessage->_InitHeader();
		}

		MessageHeader*
		GetMessageHeader()
		{
			return fMessage->fHeader;
		}

		FieldHeader*
		GetMessageFields()
		{
			return fMessage->fFields;
		}

		uint8*
		GetMessageData()
		{
			return fMessage->fData;
		}

		ssize_t
		NativeFlattenedSize() const
		{
			return fMessage->_NativeFlattenedSize();
		}

		status_t
		NativeFlatten(char *buffer, ssize_t size) const
		{
			return fMessage->_NativeFlatten(buffer, size);
		}

		status_t
		NativeFlatten(BDataIO *stream, ssize_t *size) const
		{
			return fMessage->_NativeFlatten(stream, size);
		}

		status_t
		SendMessage(port_id port, int32 token, bigtime_t timeout,
			bool replyRequired, BMessenger &replyTo) const
		{
			return fMessage->_SendMessage(port, token,
				timeout, replyRequired, replyTo);
		}

		status_t
		SendMessage(port_id port, team_id portOwner, int32 token,
			BMessage *reply, bigtime_t sendTimeout,
			bigtime_t replyTimeout) const
		{
			return fMessage->_SendMessage(port, portOwner, token,
				reply, sendTimeout, replyTimeout);
		}

		// static methods

		static status_t
		SendFlattenedMessage(void *data, int32 size, port_id port,
			int32 token, bigtime_t timeout)
		{
			return BMessage::_SendFlattenedMessage(data, size,
				port, token, timeout);
		}

		static void
		StaticInit()
		{
			BMessage::_StaticInit();
		}

		static void
		StaticCleanup()
		{
			BMessage::_StaticCleanup();
		}

		static void
		StaticCacheCleanup()
		{
			BMessage::_StaticCacheCleanup();
		}

	private:
		BMessage* fMessage;
};

#endif	// _MESSAGE_PRIVATE_H_
