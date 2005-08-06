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

#include <DataIO.h>
#include <List.h>
#include <SupportDefs.h>

#define MSG_FLAG_VALID			0x01
#define MSG_FLAG_MINI_DATA		0x02
#define MSG_FLAG_FIXED_SIZE		0x04
#define MSG_FLAG_SINGLE_ITEM	0x08
#define MSG_FLAG_ALL			0x0F

#define MSG_LAST_ENTRY			0x00

namespace BPrivate {

typedef struct field_header_s {
	uint8		flags;
	type_code	type;
	int32		count;
	ssize_t		dataSize;
	uint8		nameLength;
	char		name[255];
} __attribute__((__packed__)) FieldHeader;

class BMessageField {
public:
							BMessageField();
							BMessageField(const char *name, type_code type);
							BMessageField(uint8 flags, BDataIO *stream);
							BMessageField(const BMessageField &other);
							~BMessageField();

		BMessageField		&operator=(const BMessageField &other);

		void				Flatten(BDataIO *stream);
		void				Unflatten(uint8 flags, BDataIO *stream);

		uint8				Flags() { return fHeader.flags; };

		void				SetName(const char *name);
		const char			*Name() const { return fHeader.name; };
		uint8				NameLength() const { return fHeader.nameLength; };
		type_code			Type() const { return fHeader.type; };

		void				*AddItem(size_t length);
		void				*ReplaceItem(int32 index, size_t newLength);
		void				RemoveItem(int32 index);
		int32				CountItems() const { return fHeader.count; };

		const void			*BufferAt(int32 index, ssize_t *size) const;

		void				MakeEmpty();

		status_t			SetFixedSize(int32 itemSize, int32 count);
		bool				IsFixedSize() const { return fHeader.flags & MSG_FLAG_FIXED_SIZE; };
		size_t				TotalSize() const { return fHeader.dataSize; };

		void				PrintToStream() const;

		// hash table support
		void				SetNext(BMessageField *next) { fNext = next; };
		BMessageField		*Next() const { return fNext; };

private:
		void				ResetHeader(const char *name, type_code type);
		void				FlatResize(int32 offset, size_t oldLength,
								size_t newLength);

		FieldHeader			fHeader;
mutable	BMallocIO			fFlatBuffer;

		// fixed size items
		int32				fItemSize;

		// variable sized items
		BList				*fOffsets;

		// hash table support
		BMessageField		*fNext;
};

} // namespace BPrivate

#endif // _MESSAGE_FIELD_H_
