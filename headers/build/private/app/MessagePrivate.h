/*
 * Copyright 2005-2007, Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */
#ifndef _MESSAGE_PRIVATE_H_
#define _MESSAGE_PRIVATE_H_

#include <Message.h>
#include <TokenSpace.h>


#define MESSAGE_BODY_HASH_TABLE_SIZE	10
#define MAX_DATA_PREALLOCATION			B_PAGE_SIZE * 10
#define MAX_FIELD_PREALLOCATION			50
#define MAX_ITEM_PREALLOCATION			B_PAGE_SIZE


static const int32 kPortMessageCode = 'pjpp';


enum {
	MESSAGE_FLAG_VALID = 0x0001,
	MESSAGE_FLAG_REPLY_REQUIRED = 0x0002,
	MESSAGE_FLAG_REPLY_DONE = 0x0004,
	MESSAGE_FLAG_IS_REPLY = 0x0008,
	MESSAGE_FLAG_WAS_DELIVERED = 0x0010,
	MESSAGE_FLAG_HAS_SPECIFIERS = 0x0020,
	MESSAGE_FLAG_WAS_DROPPED = 0x0080,
	MESSAGE_FLAG_PASS_BY_AREA = 0x0100
};


enum {
	FIELD_FLAG_VALID = 0x0001,
	FIELD_FLAG_FIXED_SIZE = 0x0002,
};


struct BMessage::field_header {
	uint32		flags;
	type_code	type;
	int32		name_length;
	int32		count;
	ssize_t		data_size;
	ssize_t		allocated;
	int32		offset;
	int32		next_field;
};


struct BMessage::message_header {
	uint32		format;
	uint32		what;
	uint32		flags;

	ssize_t		fields_size;
	ssize_t		data_size;
	ssize_t		fields_available;
	ssize_t		data_available;

	uint32		fields_checksum;
	uint32		data_checksum;

	int32		target;
	int32		current_specifier;
	area_id		shared_area;

	// reply info
	port_id		reply_port;
	int32		reply_target;
	team_id		reply_team;

	// body info
	int32		field_count;
	int32		hash_table_size;
	int32		hash_table[MESSAGE_BODY_HASH_TABLE_SIZE];

	/*	The hash table does contain indexes into the field list and
		not direct offsets to the fields. This has the advantage
		of not needing to update offsets in two locations.
		The hash table must be reevaluated when we remove a field
		though.
	*/
};


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

		void
		SetWasDropped(bool wasDropped)
		{
			if (wasDropped)
				fMessage->fHeader->flags |= MESSAGE_FLAG_WAS_DROPPED;
			else
				fMessage->fHeader->flags &= ~MESSAGE_FLAG_WAS_DROPPED;
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

		BMessage::message_header*
		GetMessageHeader()
		{
			return fMessage->fHeader;
		}

		BMessage::field_header*
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

		// static methods

		static void
		StaticCacheCleanup()
		{
			BMessage::_StaticCacheCleanup();
		}

	private:
		BMessage* fMessage;
};

#endif	// _MESSAGE_PRIVATE_H_
