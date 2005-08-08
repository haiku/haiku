/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/* BMessageField contains the data for indiviual named fields in BMessageBody */

#ifndef _MESSAGE_FIELD_H_
#define _MESSAGE_FIELD_H_

#include <List.h>
#include <String.h>
#include <SupportDefs.h>

// we only support those
#define MSG_FLAG_VALID			0x01
#define MSG_FLAG_FIXED_SIZE		0x04
#define MSG_FLAG_ALL			0x05

// these are for R5 compatibility
#define MSG_FLAG_MINI_DATA		0x02
#define MSG_FLAG_SINGLE_ITEM	0x08

#define MSG_LAST_ENTRY			0x00

namespace BPrivate {

struct field_header_s {
	uint8		flags;
	type_code	type;
	int32		count;
	ssize_t		dataSize;
	uint8		nameLength;
} __attribute__((__packed__));

struct item_info_s {
				item_info_s(int32 _offset)
					:	offset(_offset)
				{
				}

				item_info_s(int32 _offset, int32 _length)
					:	offset(_offset),
						length(_length)
				{
				}

	int32		offset;
	int32		length;
	int32		paddedLength;
};

typedef field_header_s FieldHeader;
typedef item_info_s ItemInfo;

class BMessageBody;

class BMessageField {
public:
								BMessageField(BMessageBody *parent,
									int32 offset, const char *name,
									type_code type);

								BMessageField(BMessageBody *parent);
								~BMessageField();

		int32					Unflatten(int32 offset);

		uint8					Flags() const { return fHeader->flags; };
		status_t				SetFixedSize(int32 itemSize);
		bool					FixedSize() const { return fHeader->flags & MSG_FLAG_FIXED_SIZE; };

		void					SetName(const char *newName);
		const char				*Name() const;
		uint8					NameLength() const { return fHeader->nameLength; };
		type_code				Type() const { return fHeader->type; };

		void					AddItem(const void *item, ssize_t length);
		uint8					*AddItem(ssize_t length);

		void					ReplaceItem(int32 index, const void *newItem,
									ssize_t length);
		uint8					*ReplaceItem(int32 index, ssize_t length);

		void					*ItemAt(int32 index, ssize_t *size);

		void					RemoveItem(int32 index);
		int32					CountItems() const { return fHeader->count; };

		void					PrintToStream() const;

		void					SetOffset(int32 offset);
		int32					Offset() const { return fOffset; };

		// always do MakeEmpty -> RemoveSelf -> delete
		void					MakeEmpty();
		void					RemoveSelf();

		// hash table support
		void					SetNext(BMessageField *next) { fNext = next; };
		BMessageField			*Next() const { return fNext; };

private:
		bool					IsFixedSize(type_code type);

		BMessageBody			*fParent;
		int32					fOffset;
		FieldHeader				*fHeader;

		int32					fItemSize;
		BList					fItemInfos;

		BMessageField			*fNext;
};

} // namespace BPrivate

#endif // _MESSAGE_FIELD_H_
