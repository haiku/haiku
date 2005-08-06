/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include <stdio.h>
#include <string.h>
#include <TypeConstants.h>
#include "MessageField2.h"
#include "MessageUtils2.h"

namespace BPrivate {

inline int32
round_to_8(int32 length)
{
	// already includes the + 4 for size_t length field
	return (length + 11) & ~7;
}


BMessageField::BMessageField()
	:	fItemSize(0),
		fOffsets(NULL),
		fNext(NULL)
{
	ResetHeader("", 0);
}


BMessageField::BMessageField(const char *name, type_code type)
	:	fItemSize(0),
		fOffsets(NULL),
		fNext(NULL)
{
	ResetHeader(name, type);
}


BMessageField::BMessageField(uint8 flags, BDataIO *stream)
	:	fItemSize(0),
		fOffsets(NULL),
		fNext(NULL)
{
	Unflatten(flags, stream);
}


BMessageField::BMessageField(const BMessageField &other)
	:	fItemSize(0),
		fOffsets(NULL),
		fNext(NULL)
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
		fNext = NULL;

		memcpy(&fHeader, &other.fHeader, sizeof(FieldHeader));

		fFlatBuffer.Write(other.fFlatBuffer.Buffer(), other.fFlatBuffer.BufferLength());

		if (other.fOffsets) {
			int32 count = other.fOffsets->CountItems();
			fOffsets = new BList(count);

			for (int32 index = 0; index < count; index++)
				fOffsets->AddItem(other.fOffsets->ItemAt(index));
		} else {
			fItemSize = other.fItemSize;
		}
	}

	return *this;
}


void
BMessageField::Flatten(BDataIO *stream)
{
	// write flat header including name
	stream->Write(&fHeader, sizeof(fHeader) - sizeof(fHeader.name) + fHeader.nameLength);

	// write flat data
	stream->Write(fFlatBuffer.Buffer(), fHeader.dataSize);
}


void
BMessageField::Unflatten(uint8 flags, BDataIO *stream)
{
	TReadHelper reader(stream);

	// read the header
	if (flags == MSG_FLAG_VALID
		|| flags == MSG_FLAG_VALID | MSG_FLAG_FIXED_SIZE) {
		// we can read the header as flat data
		fHeader.flags = flags;
		stream->Read(&fHeader.type, sizeof(fHeader) - sizeof(fHeader.flags) - sizeof(fHeader.name));
		stream->Read(&fHeader.name, fHeader.nameLength);
		fItemSize = fHeader.dataSize / fHeader.count;
	} else {
		// we have to disassemble the header
		type_code type;
		reader(type);
		ResetHeader("", type);

		if (flags & MSG_FLAG_SINGLE_ITEM) {
			fHeader.count = 1;

			if (flags & MSG_FLAG_MINI_DATA) {
				uint8 littleData;
				reader(littleData);
				fHeader.dataSize = littleData;
			} else
				reader(fHeader.dataSize);
		} else {
			if (flags & MSG_FLAG_MINI_DATA) {
				uint8 littleData;
				reader(littleData);
				fHeader.count = littleData;

				reader(littleData);
				fHeader.dataSize = littleData;
			} else {
				reader(fHeader.count);
				reader(fHeader.dataSize);
			}
		}

		reader(fHeader.nameLength);
		reader(fHeader.name, fHeader.nameLength);
	}

	// now read the data
	fFlatBuffer.SetSize(fHeader.dataSize);
	stream->Read((void *)fFlatBuffer.Buffer(), fHeader.dataSize);

	// ToDo: enable swapping
	// swap the data if necessary
	if (false) {
		// is the data fixed size?
		if (flags & MSG_FLAG_FIXED_SIZE) {
			// make sure to swap the data
			status_t status = swap_data(fHeader.type,
				(void *)fFlatBuffer.Buffer(), fHeader.dataSize, B_SWAP_ALWAYS);
			if (status < B_OK)
				throw status;
		} else if (fHeader.type == B_REF_TYPE) {
			// apparently, entry_refs are the only variable-length data
			// explicitely swapped -- the dev_t and ino_t specifically
			// ToDo: this looks broken
			byte_swap(*(entry_ref *)fFlatBuffer.Buffer());
		}
	}
}


void
BMessageField::ResetHeader(const char *name, type_code type)
{
	memset(&fHeader, 0, sizeof(fHeader));
	fHeader.flags |= MSG_FLAG_VALID;
	fHeader.type = type;
	SetName(name);
}


void
BMessageField::MakeEmpty()
{
	fFlatBuffer.SetSize(0);

	if (fOffsets)
		fOffsets->MakeEmpty();

	fHeader.dataSize = 0;
	fHeader.count = 0;
}


status_t
BMessageField::SetFixedSize(int32 itemSize, int32 /*count*/)
{
	// ToDo: use count here to preallocate the buffer
	if (fHeader.count > 0 && itemSize != fItemSize) {
		debugger("not allowed\n");
		return B_BAD_VALUE;
	}

	fItemSize = itemSize;
	fHeader.flags |= MSG_FLAG_FIXED_SIZE;
	return B_OK;
}


void
BMessageField::SetName(const char *name)
{
	fHeader.nameLength = min_c(strlen(name), 254);
	memcpy(fHeader.name, name, fHeader.nameLength);
}


void *
BMessageField::AddItem(size_t length)
{
	fHeader.count++;

	int32 offset = fFlatBuffer.BufferLength();
	if (fHeader.flags & MSG_FLAG_FIXED_SIZE) {
		fHeader.dataSize += length;
		fFlatBuffer.SetSize(offset + length);
		return (char *)fFlatBuffer.Buffer() + offset;
	}

	if (!fOffsets)
		fOffsets = new BList();

	fOffsets->AddItem((void *)offset);

	size_t newLength = round_to_8(length);
	fHeader.dataSize += newLength;
	FlatResize(offset, 0, newLength);
	fFlatBuffer.WriteAt(offset, &length, sizeof(length));
	return (char *)fFlatBuffer.Buffer() + offset + sizeof(newLength);
}


void *
BMessageField::ReplaceItem(int32 index, size_t newLength)
{
	if (fHeader.flags & MSG_FLAG_FIXED_SIZE)
		return (char *)fFlatBuffer.Buffer() + (index * fItemSize);

	int32 offset = (int32)fOffsets->ItemAt(index);

	size_t oldLength;
	fFlatBuffer.ReadAt(offset, &oldLength, sizeof(oldLength));
	fFlatBuffer.WriteAt(offset, &newLength, sizeof(newLength));

	oldLength = round_to_8(oldLength);
	newLength = round_to_8(newLength);

	if (oldLength == newLength)
		return (char *)fFlatBuffer.Buffer() + offset + sizeof(newLength);

	FlatResize(offset, oldLength, newLength);
	fHeader.dataSize += newLength - oldLength;

	return (char *)fFlatBuffer.Buffer() + offset + sizeof(newLength);
}


void
BMessageField::RemoveItem(int32 index)
{
	fHeader.count--;

	if (fHeader.flags & MSG_FLAG_FIXED_SIZE) {
		FlatResize(index * fItemSize, fItemSize, 0);
		fHeader.dataSize -= fItemSize;
		return;
	}

	size_t oldLength;
	int32 offset = (int32)fOffsets->ItemAt(index);
	fFlatBuffer.ReadAt(offset, &oldLength, sizeof(oldLength));
	oldLength = round_to_8(oldLength);

	FlatResize(offset, oldLength, 0);
	fHeader.dataSize -= oldLength;
}


const void *
BMessageField::BufferAt(int32 index, ssize_t *size) const
{
	if (fHeader.flags & MSG_FLAG_FIXED_SIZE) {
		if (size)
			*size = fItemSize;

		return (char *)fFlatBuffer.Buffer() + (index * fItemSize);
	}

	int32 offset = (int32)fOffsets->ItemAt(index);
	if (size)
		fFlatBuffer.ReadAt(offset, size, sizeof(ssize_t));

	return (char *)fFlatBuffer.Buffer() + offset + sizeof(ssize_t);
}


void
BMessageField::PrintToStream() const
{
	// ToDo: implement
}


void
BMessageField::FlatResize(int32 offset, size_t oldLength, size_t newLength)
{
	ssize_t change = newLength - oldLength;
	ssize_t bufferLength = fFlatBuffer.BufferLength();

	if (change > 0)
		fFlatBuffer.SetSize(bufferLength + change);

	if (offset == bufferLength) {
		// the easy case for adds
		return;
	}

	ssize_t length = bufferLength - offset - oldLength;
	if (length > 0) {
		uint8 *location = (uint8 *)fFlatBuffer.Buffer() + offset;
		memmove(location + newLength, location + oldLength, length);
	}

	if (change < 0)
		fFlatBuffer.SetSize(bufferLength + change);

	for (int32 index = fOffsets->CountItems() - 1; index >= 0; index--) {
		int32 oldOffset = (int32)fOffsets->ItemAt(index);
		if (oldOffset <= offset)
			break;

		fOffsets->ReplaceItem(index, (void *)(offset + change));
	}
}

} // namespace BPrivate
