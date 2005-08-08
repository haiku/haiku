/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <stdio.h>
#include <DataIO.h>
#include <MessageUtils.h>
#include "MessageBody3.h"
#include "MessageField3.h"

namespace BPrivate {

#define DATA_OFFSET (fOffset + sizeof(FieldHeader) + fHeader->nameLength)

inline int32
round_to_8(int32 length)
{
	// already includes the + 4 for size_t length field
	return (length + 11) & ~7;
}


BMessageField::BMessageField(BMessageBody *parent, int32 offset,
	const char *name, type_code type)
	:	fParent(parent),
		fOffset(offset),
		fItemSize(0)
{
	// insert space needed for the header
	fParent->FlatInsert(fOffset, 0, sizeof(FieldHeader));

	// create and fill header
	fHeader = (FieldHeader *)(fParent->FlatBuffer() + fOffset);
	fHeader->type = type;
	fHeader->flags = MSG_FLAG_VALID;
	fHeader->count = 0;
	fHeader->dataSize = 0;
	fHeader->nameLength = 0;

	SetName(name);
}


BMessageField::BMessageField(BMessageBody *parent)
	:	fParent(parent),
		fOffset(0),
		fHeader(NULL),
		fItemSize(0)
{
}


BMessageField::~BMessageField()
{
}


int32
BMessageField::Unflatten(int32 _offset)
{
	fOffset = _offset;

	// field is already present, just tap into it
	fHeader = (FieldHeader *)(fParent->FlatBuffer() + fOffset);

	if (fHeader->flags & MSG_FLAG_FIXED_SIZE) {
		if (fHeader->count > 0)
			fItemSize = fHeader->dataSize / fHeader->count;
		return sizeof(FieldHeader) + fHeader->nameLength + fHeader->count * fItemSize;
	}

	// create the item info list
	int32 offset = 0;
	uint8 *location = fParent->FlatBuffer() + DATA_OFFSET;
	for (int32 index = 0; index < fHeader->count; index++) {
		ItemInfo *info = new ItemInfo(offset);
		info->length = *(size_t *)location;
		info->paddedLength = round_to_8(info->length);
		fItemInfos.AddItem(info);

		offset += info->paddedLength;
		location += info->paddedLength;
	}

	return sizeof(FieldHeader) + fHeader->nameLength + offset;
}


void
BMessageField::SetOffset(int32 offset)
{
	fOffset = offset;
	fHeader = (FieldHeader *)(fParent->FlatBuffer() + fOffset);
}


void
BMessageField::SetName(const char *newName)
{
	int32 newLength = min_c(strlen(newName), 254) + 1;
	char *buffer = (char *)fParent->FlatInsert(fOffset + sizeof(FieldHeader), fHeader->nameLength, newLength);
	strcpy(buffer, newName);
	fHeader->nameLength = newLength;
}


const char *
BMessageField::Name() const
{
	return (const char *)(fParent->FlatBuffer() + fOffset + sizeof(FieldHeader));
}


status_t
BMessageField::SetFixedSize(int32 itemSize)
{
	if ((fItemSize > 0 && fItemSize != itemSize)
		|| fItemInfos.CountItems() > 0)
		return B_BAD_VALUE;

	fItemSize = itemSize;
	fHeader->flags |= MSG_FLAG_FIXED_SIZE;
	return B_OK;
}


void
BMessageField::AddItem(const void *item, ssize_t length)
{
	memcpy(AddItem(length), item, length);
}


uint8 *
BMessageField::AddItem(ssize_t length)
{
	if (fHeader->flags & MSG_FLAG_FIXED_SIZE) {
		int32 offset = DATA_OFFSET + (fHeader->count * fItemSize);
		fHeader->dataSize += fItemSize;
		fHeader->count++;
		return fParent->FlatInsert(offset, 0, fItemSize);
	}

	int32 newOffset = 0;
	if (fHeader->count > 0) {
		ItemInfo *info = (ItemInfo *)fItemInfos.ItemAt(fHeader->count - 1);
		newOffset = info->offset + info->paddedLength;
	}

	ItemInfo *newInfo = new ItemInfo(newOffset, length);
	newInfo->paddedLength = round_to_8(length);

	fItemInfos.AddItem(newInfo);
	fHeader->dataSize += newInfo->paddedLength;
	fHeader->count++;

	uint8 *result = fParent->FlatInsert(DATA_OFFSET + newInfo->offset, 0, newInfo->paddedLength);
	*(int32 *)result = length;
	return result + sizeof(int32);
}


void
BMessageField::ReplaceItem(int32 index, const void *newItem, ssize_t length)
{
	memcpy(ReplaceItem(index, length), newItem, length);
}


uint8 *
BMessageField::ReplaceItem(int32 index, ssize_t length)
{
	if (fHeader->flags & MSG_FLAG_FIXED_SIZE)
		return fParent->FlatBuffer() + DATA_OFFSET + (index * fItemSize);

	ItemInfo *info = (ItemInfo *)fItemInfos.ItemAt(index);
	int32 newLength = round_to_8(length);

	if (info->paddedLength == newLength) {
		uint8 *result = fParent->FlatBuffer() + DATA_OFFSET + info->offset;
		info->length = length;
		*(int32 *)result = length;
		return result + sizeof(int32);
	}

	int32 change = newLength - info->paddedLength;
	for (int32 i = index + 1; i < fHeader->count; i++)
		((ItemInfo *)fItemInfos.ItemAt(i))->offset += change;
	fHeader->dataSize += change;

	int32 oldLength = info->paddedLength;
	info->length = length;
	info->paddedLength = newLength;

	uint8 *result = fParent->FlatInsert(DATA_OFFSET + info->offset, oldLength, newLength);
	*(int32 *)result = length;
	return result + sizeof(int32);
}


void *
BMessageField::ItemAt(int32 index, ssize_t *size)
{
	if (fHeader->flags & MSG_FLAG_FIXED_SIZE) {
		if (size)
			*size = fItemSize;

		return fParent->FlatBuffer() + DATA_OFFSET + (index * fItemSize);
	}

	ItemInfo *info = (ItemInfo *)fItemInfos.ItemAt(index);

	if (size)
		*size = info->length;

	return fParent->FlatBuffer() + DATA_OFFSET + info->offset + sizeof(int32);
}


void
BMessageField::RemoveItem(int32 index)
{
	if (fHeader->flags & MSG_FLAG_FIXED_SIZE) {
		fParent->FlatInsert(DATA_OFFSET + (index * fItemSize), fItemSize, 0);
		fHeader->dataSize -= fItemSize;
		fHeader->count--;
		return;
	}

	ItemInfo *info = (ItemInfo *)fItemInfos.RemoveItem(index);

	for (int32 i = index; i < fHeader->count; i++)
		((ItemInfo *)fItemInfos.ItemAt(i))->offset -= info->paddedLength;

	fParent->FlatInsert(DATA_OFFSET + info->offset, info->paddedLength, 0);
	fHeader->dataSize -= info->paddedLength;
	fHeader->count--;
	delete info;
}


void
BMessageField::PrintToStream() const
{
	printf("\tname: \"%s\"; items: %ld; dataSize: %ld; flags: %02x;", Name(), fHeader->count, fHeader->dataSize, fHeader->flags);
	// ToDo: implement for real
}


void
BMessageField::MakeEmpty()
{
	for (int32 index = 0; index < fItemInfos.CountItems(); index++)
		delete (ItemInfo *)fItemInfos.ItemAt(index);

	fItemInfos.MakeEmpty();
}


void
BMessageField::RemoveSelf()
{
	// remove ourselfs from the flat buffer
	fParent->FlatInsert(fOffset, sizeof(FieldHeader) + fHeader->nameLength + fHeader->dataSize, 0);
	fOffset = 0;
	fHeader = NULL;
}

} // namespace BPrivate
