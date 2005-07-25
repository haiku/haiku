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
#include <TypeConstants.h>
#include "MessageField2.h"

namespace BPrivate {

BMessageField::BMessageField()
	:	fType(0),
		fFixedSize(false),
		fTotalSize(0),
		fTotalPadding(0),
		fNext(NULL)
{
	SetName("");
}


BMessageField::BMessageField(const char *name, type_code type)
	:	fType(type),
		fTotalSize(0),
		fTotalPadding(0),
		fNext(NULL)
{
	SetName(name);
	fFixedSize = IsFixedSize(type);
}


BMessageField::BMessageField(const BMessageField &other)
{
	*this = other;
}


BMessageField::~BMessageField()
{
	MakeEmpty();
}


BMessageField &
BMessageField::operator=(const BMessageField &other)
{
	if (this != &other) {
		MakeEmpty();

		fName = other.fName;
		fType = other.fType;
		fFixedSize = other.fFixedSize;
		fTotalSize = other.fTotalSize;
		fTotalPadding = other.fTotalPadding;
		fNext = NULL;

		for (int32 index = 0; index < other.fItems.CountItems(); index++) {
			BMallocIO *otherBuffer = (BMallocIO *)other.fItems.ItemAt(index);
			BMallocIO *newBuffer = new BMallocIO;
			newBuffer->Write(otherBuffer->Buffer(), otherBuffer->BufferLength());
			fItems.AddItem((void *)newBuffer);
		}
	}

	return *this;
}


void
BMessageField::MakeEmpty()
{
	for (int32 index = 0; index < fItems.CountItems(); index++) {
		BMallocIO *item = (BMallocIO *)fItems.ItemAt(index);
		delete item;
	}

	fItems.MakeEmpty();
}


uint8
BMessageField::Flags()
{
	uint8 flags = MSG_FLAG_VALID;
	
	if (fItems.CountItems() == 1)
		flags |= MSG_FLAG_SINGLE_ITEM;

	if (fTotalSize + fTotalPadding < 255)
		flags |= MSG_FLAG_MINI_DATA;

	if (fFixedSize)
		flags |= MSG_FLAG_FIXED_SIZE;

	return flags;
}


void
BMessageField::SetName(const char *name)
{
	fName = name;

	// the name length field is only 1 byte long
	// ToDo: change this? (BC problem)
	fName.Truncate(255 - 1);
}


void
BMessageField::AddItem(BMallocIO *item)
{
	fItems.AddItem((void *)item);
	fTotalSize += item->BufferLength();
	fTotalPadding += calc_padding(item->BufferLength() + 4, 8);
}


void
BMessageField::ReplaceItem(int32 index, BMallocIO *item, bool deleteOld)
{
	BMallocIO *oldItem = (BMallocIO *)fItems.ItemAt(index);
	fTotalSize -= oldItem->BufferLength();
	fTotalPadding -= calc_padding(oldItem->BufferLength() + 4, 8);

	fItems.ReplaceItem(index, item);
	fTotalSize += item->BufferLength();
	fTotalPadding += calc_padding(item->BufferLength() + 4, 8);

	if (deleteOld)
		delete oldItem;
}


void
BMessageField::RemoveItem(int32 index, bool deleteIt)
{
	BMallocIO *item = (BMallocIO *)fItems.RemoveItem(index);

	fTotalSize -= item->BufferLength();
	fTotalPadding -= calc_padding(item->BufferLength() + 4, 8);
	if (deleteIt)
		delete item;
}


size_t
BMessageField::SizeAt(int32 index) const
{
	BMallocIO *buffer = (BMallocIO *)fItems.ItemAt(index);

	if (buffer)
		return buffer->BufferLength();

	return 0;
}


const void *
BMessageField::BufferAt(int32 index) const
{
	BMallocIO *buffer = (BMallocIO *)fItems.ItemAt(index);

	if (buffer)
		return buffer->Buffer();

	return NULL;
}


void
BMessageField::PrintToStream() const
{
	// ToDo: implement
}


bool
BMessageField::IsFixedSize(type_code type)
{
	switch (type) {
		case B_ANY_TYPE:
		case B_MESSAGE_TYPE:
		case B_MIME_TYPE:
		case B_OBJECT_TYPE:
		case B_RAW_TYPE:
		case B_STRING_TYPE:
		case B_ASCII_TYPE: return false;

		case B_BOOL_TYPE:
		case B_CHAR_TYPE:
		case B_COLOR_8_BIT_TYPE:
		case B_DOUBLE_TYPE:
		case B_FLOAT_TYPE:
		case B_GRAYSCALE_8_BIT_TYPE:
		case B_INT64_TYPE:
		case B_INT32_TYPE:
		case B_INT16_TYPE:
		case B_INT8_TYPE:
		case B_MESSENGER_TYPE: // ToDo: test
		case B_MONOCHROME_1_BIT_TYPE:
		case B_OFF_T_TYPE:
		case B_PATTERN_TYPE:
		case B_POINTER_TYPE:
		case B_POINT_TYPE:
		case B_RECT_TYPE:
		case B_REF_TYPE: // ToDo: test
		case B_RGB_32_BIT_TYPE:
		case B_RGB_COLOR_TYPE:
		case B_SIZE_T_TYPE:
		case B_SSIZE_T_TYPE:
		case B_TIME_TYPE:
		case B_UINT64_TYPE:
		case B_UINT32_TYPE:
		case B_UINT16_TYPE:
		case B_UINT8_TYPE: return true;

		// ToDo: test
		case B_MEDIA_PARAMETER_TYPE:
		case B_MEDIA_PARAMETER_WEB_TYPE:
		case B_MEDIA_PARAMETER_GROUP_TYPE: return false;
	}

	return false;
}

} // namespace BPrivate
