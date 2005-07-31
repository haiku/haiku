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

#define MSG_FLAG_VALID			0x01
#define MSG_FLAG_MINI_DATA		0x02
#define MSG_FLAG_FIXED_SIZE		0x04
#define MSG_FLAG_SINGLE_ITEM	0x08
#define MSG_FLAG_ALL			0x0F

#define MSG_LAST_ENTRY			0x00

namespace BPrivate {

class BSimpleMallocIO;

class BMessageField {
public:
							BMessageField();
							BMessageField(const char *name, type_code type);
							BMessageField(const BMessageField &other);
							~BMessageField();

		BMessageField		&operator=(const BMessageField &other);

		uint8				Flags();

		void				SetName(const char *name);
		const char			*Name() const { return fName.String(); };
		uint8				NameLength() const { return fName.Length(); };
		type_code			Type() const { return fType; };

		void				AddItem(BSimpleMallocIO *item);
		void				ReplaceItem(int32 index, BSimpleMallocIO *item);
		void				RemoveItem(int32 index);
		int32				CountItems() const { return fItems.CountItems(); };
		size_t				SizeAt(int32 index) const;
		const void			*BufferAt(int32 index) const;

		void				MakeEmpty();

		bool				IsFixedSize() const { return fFixedSize; };
		size_t				TotalSize() const { return fTotalSize; };
		size_t				TotalPadding() const { return fTotalPadding; };

		void				PrintToStream() const;

		// hash table support
		void				SetNext(BMessageField *next) { fNext = next; };
		BMessageField		*Next() const { return fNext; };

private:
		bool				IsFixedSize(type_code type);

		BString				fName;
		type_code			fType;
		BList				fItems;
		bool				fFixedSize;
		size_t				fTotalSize;
		size_t				fTotalPadding;

		BMessageField		*fNext;
};

} // namespace BPrivate

#endif // _MESSAGE_FIELD_H_
