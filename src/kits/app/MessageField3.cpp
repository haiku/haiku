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

#define DATA_OFFSET (fOffset + fDataOffset)

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
		fDataOffset(0),
		fFixedSize(false),
		fItemSize(0),
		fItemInfos(5),
		fNext(NULL)
{
	// insert space needed for the header
	fParent->FlatInsert(fOffset, 0, sizeof(FieldHeader));

	// fill the header
	FieldHeader *header = Header();
	header->type = type;
	header->flags = MSG_FLAG_VALID;
	header->count = 0;
	header->dataSize = 0;
	header->nameLength = 0;

	SetName(name);
}


BMessageField::BMessageField(BMessageBody *parent)
	:	fParent(parent),
		fOffset(0),
		fDataOffset(0),
		fFixedSize(false),
		fItemSize(0),
		fItemInfos(5),
		fNext(NULL)
{
}


int32
BMessageField::Unflatten(int32 _offset)
{
	fOffset = _offset;

	// field is already present, just tap into it
	FieldHeader *header = Header();
	fDataOffset = sizeof(FieldHeader) + header->nameLength;
	fFixedSize = header->flags & MSG_FLAG_FIXED_SIZE;

	if (header->nameLength == 0)
		return 0;

	if (fFixedSize) {
		if (header->count > 0)
			fItemSize = header->dataSize / header->count;
		return fDataOffset + header->count * fItemSize;
	}

	// create the item info list
	int32 offset = 0;
	uint8 *location = fParent->FlatBuffer() + DATA_OFFSET;
	for (int32 index = 0; index < header->count; index++) {
		ItemInfo *info = new ItemInfo(offset);
		info->length = *(size_t *)location;
		info->paddedLength = round_to_8(info->length);
		fItemInfos.AddItem(info);

		offset += info->paddedLength;
		location += info->paddedLength;
	}

	return fDataOffset + offset;
}


void
BMessageField::SetName(const char *newName)
{
	FieldHeader *header = Header();

	int32 newLength = min_c(strlen(newName), 254) + 1;
	char *buffer = (char *)fParent->FlatInsert(fOffset + sizeof(FieldHeader), header->nameLength, newLength);
	strncpy(buffer, newName, newLength);
	buffer[newLength] = 0;

	header->nameLength = newLength;
	fDataOffset = sizeof(FieldHeader) + newLength;
}


const char *
BMessageField::Name() const
{
	return (const char *)(fParent->FlatBuffer() + fOffset + sizeof(FieldHeader));
}


status_t
BMessageField::SetFixedSize(int32 itemSize)
{
	if (fItemInfos.CountItems() > 0
		&& (fItemSize > 0 && fItemSize != itemSize))
		return B_BAD_VALUE;

	fItemSize = itemSize;
	Header()->flags |= MSG_FLAG_FIXED_SIZE;
	fFixedSize = true;
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
	FieldHeader *header = Header();

	if (fFixedSize) {
		int32 offset = DATA_OFFSET + (header->count * fItemSize);
		header->dataSize += fItemSize;
		header->count++;
		return fParent->FlatInsert(offset, 0, fItemSize);
	}

	int32 newOffset = 0;
	if (header->count > 0) {
		ItemInfo *info = (ItemInfo *)fItemInfos.ItemAt(header->count - 1);
		newOffset = info->offset + info->paddedLength;
	}

	ItemInfo *newInfo = new ItemInfo(newOffset, length);
	newInfo->paddedLength = round_to_8(length);

	fItemInfos.AddItem(newInfo);
	header->dataSize += newInfo->paddedLength;
	header->count++;

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
	if (fFixedSize)
		return fParent->FlatBuffer() + DATA_OFFSET + (index * fItemSize);

	ItemInfo *info = (ItemInfo *)fItemInfos.ItemAt(index);
	int32 newLength = round_to_8(length);

	if (info->paddedLength == newLength) {
		uint8 *result = fParent->FlatBuffer() + DATA_OFFSET + info->offset;
		info->length = length;
		*(int32 *)result = length;
		return result + sizeof(int32);
	}

	FieldHeader *header = Header();
	int32 change = newLength - info->paddedLength;
	for (int32 i = index + 1; i < header->count; i++)
		((ItemInfo *)fItemInfos.ItemAt(i))->offset += change;
	header->dataSize += change;

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
	if (fFixedSize) {
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
	FieldHeader *header = Header();

	if (fFixedSize) {
		fParent->FlatInsert(DATA_OFFSET + (index * fItemSize), fItemSize, 0);
		header->dataSize -= fItemSize;
		header->count--;
		return;
	}

	ItemInfo *info = (ItemInfo *)fItemInfos.RemoveItem(index);

	for (int32 i = index; i < header->count; i++)
		((ItemInfo *)fItemInfos.ItemAt(i))->offset -= info->paddedLength;

	fParent->FlatInsert(DATA_OFFSET + info->offset, info->paddedLength, 0);
	header->dataSize -= info->paddedLength;
	header->count--;
	delete info;
}


void
BMessageField::PrintToStream() const
{
	FieldHeader *header = Header();
	printf("\tname: \"%s\"; items: %ld; dataSize: %ld; flags: %02x; offset: %ld", Name(), header->count, header->dataSize, header->flags, fOffset);
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
	// remove ourself from the flat buffer
	FieldHeader *header = Header();
	fParent->FlatInsert(fOffset, fDataOffset + header->dataSize, 0);
	fOffset = 0;
}

} // namespace BPrivate
