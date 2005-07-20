/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/* BMessageBody handles data storage and retrieval for BMessage. */

#include <stdio.h>
#include <DataIO.h>
#include <TypeConstants.h>
#include "MessageBody2.h"

namespace BPrivate {

BMessageBody::BMessageBody()
{
}


BMessageBody::BMessageBody(const BMessageBody &other)
{
	*this = other;
}


BMessageBody::~BMessageBody()
{
	MakeEmpty();
}


BMessageBody &
BMessageBody::operator=(const BMessageBody &other)
{
	if (this != &other) {
		MakeEmpty();
		for (int32 index = 0; index < other.fFields.CountItems(); index++) {
			BMessageField *otherField = (BMessageField *)other.fFields.ItemAt(index);
			BMessageField *newField = new BMessageField(*otherField);
			fFields.AddItem((void *)newField);
		}
	}

	return *this;
}


status_t
BMessageBody::GetInfo(type_code typeRequested, int32 which, char **name,
	type_code *typeReturned, int32 *count) const
{
	int32 index = 0;
	int32 fieldCount = fFields.CountItems();
	BMessageField *field = NULL;
	bool found = false;

	for (int32 fieldIndex = 0; fieldIndex < fieldCount; fieldIndex++) {
		field = (BMessageField *)fFields.ItemAt(fieldIndex);

		if (typeRequested == B_ANY_TYPE || field->Type() == typeRequested) {
			if (index == which) {
				found = true;
				break;
			}

			index++;
		}
	}

	// we couldn't find any appropriate data
	if (!found) {
		if (count)
			*count = 0;

		if (index > 0)
			return B_BAD_INDEX;
		else
			return B_BAD_TYPE;
	}

	// ToDo: BC Break
	// Change 'name' parameter to const char *

	if (name)
		*name = const_cast<char *>(field->Name());

	if (typeReturned)
		*typeReturned = field->Type();

	if (count)
		*count = field->CountItems();

	return B_OK;
}


status_t
BMessageBody::GetInfo(const char *name, type_code *typeFound,
	int32 *countFound) const
{
	status_t error = B_OK;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (typeFound)
			*typeFound = field->Type();

		if (countFound)
			*countFound = field->CountItems();
	} else {
		if (countFound)
			*countFound = 0;
	}

	return error;
}


status_t
BMessageBody::GetInfo(const char *name, type_code *typeFound, bool *fixedSize) const
{
	status_t error = B_OK;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (typeFound)
			*typeFound = field->Type();

		if (fixedSize)
			*fixedSize = field->IsFixedSize();
	}

	return error;
}


int32
BMessageBody::CountNames(type_code type) const
{
	if (type == B_ANY_TYPE) {
		return fFields.CountItems();
	}

	int32 count = 0;
	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);
		if (field->Type() == type)
			count++;
	}

	return count;
}


bool
BMessageBody::IsEmpty() const
{
	return fFields.CountItems() == 0;
}


void
BMessageBody::PrintToStream() const
{
	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);
		field->PrintToStream();
		printf("\n");
	}
}


status_t
BMessageBody::Rename(const char *oldName, const char *newName)
{
	status_t error = B_OK;
	BMessageField *field = FindData(oldName, B_ANY_TYPE, error);

	if (!field)
		return B_NAME_NOT_FOUND;

	field->SetName(newName);
	return B_OK;
}


ssize_t
BMessageBody::FlattenedSize() const
{
	ssize_t size = 1; // for MSG_LAST_ENTRY

	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);
		size += field->TotalSize();
		size += field->NameLength();

		// ToDo: too expensive?
		uint8 flags = field->Flags();
		size += sizeof(flags);

		// count information
		if (!(flags & MSG_FLAG_SINGLE_ITEM)) {
			if (flags & MSG_FLAG_MINI_DATA)
				size += sizeof(uint8);
			else
				size += sizeof(int32);
		}

		// total size
		if (flags & MSG_FLAG_MINI_DATA)
			size += sizeof(uint8);
		else
			size += sizeof(size_t);

		// individual sizes
		if (!(flags & MSG_FLAG_FIXED_SIZE))
			size += field->CountItems() * sizeof(size_t);
	}

	return size;
}


status_t
BMessageBody::Flatten(BDataIO *stream) const
{
	status_t error = B_OK;

	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);
	
		uint8 flags = field->Flags();
		stream->Write(&flags, sizeof(flags));

		type_code type = field->Type();
		stream->Write(&type, sizeof(type));

		// add item count if not a single item
		int32 count = field->CountItems();
		if (!(flags & MSG_FLAG_SINGLE_ITEM)) {
			if (flags & MSG_FLAG_MINI_DATA) {
				uint8 miniCount = (uint8)count;
				stream->Write(&miniCount, sizeof(miniCount));
			} else
				stream->Write(&count, sizeof(count));
		}

		bool isFixed = flags & MSG_FLAG_FIXED_SIZE;

		// overall data size
		size_t size = field->TotalSize();
		if (!isFixed) {
			// add bytes for holding each items size
			size += count * sizeof(size_t);
			// ToDo: add padding here
		}

		if (flags & MSG_FLAG_MINI_DATA) {
			uint8 miniSize = (uint8)size;
			stream->Write(&miniSize, sizeof(miniSize));
		} else
			stream->Write(&size, sizeof(size));

		// name length
		uint8 nameLength = field->NameLength();
		stream->Write(&nameLength, sizeof(nameLength));

		// name
		stream->Write(field->Name(), nameLength);

		// if we have a fixed size we initialize size once here
		if (isFixed)
			size = field->SizeAt(0);

		// data items
		for (int32 dataIndex = 0; dataIndex < count; dataIndex++) {
			if (!isFixed) {
				// set the size for each item
				size = field->SizeAt(dataIndex);
				stream->Write(&size, sizeof(size));
			}

			error = stream->Write(field->BufferAt(dataIndex), size);
			// ToDo: add padding here too
		}
	}

	if (error >= B_OK) {
		error = stream->Write('\0', 1);	// MSG_LAST_ENTRY
	}

	if (error >= B_OK)
		return B_OK;

	return error;
}


status_t
BMessageBody::AddData(const char *name, BMallocIO *buffer, type_code type)
{
	status_t error = B_OK;
	BMessageField *foundField = FindData(name, type, error);

	if (error < B_OK) {
		if (error == B_NAME_NOT_FOUND) {
			// reset the error - we will create the field
			error = B_OK;
		} else {
			// looking for B_BAD_TYPE here in particular, which would indicate
			// that we tried to add data of type X when we already had data of
			// type Y with the same name
			return error;
		}
	}

	if (!foundField) {
		// add a new field if it's not yet present
		BMessageField *newField = new BMessageField(name, type);
		newField->AddItem(buffer);
		fFields.AddItem(newField);
	} else {
		// add to the existing field otherwise
		foundField->AddItem(buffer);
	}

	return error;
}


status_t
BMessageBody::ReplaceData(const char *name, int32 index, BMallocIO *buffer,
	type_code type)
{
	if (type == B_ANY_TYPE)
		return B_BAD_VALUE;

	status_t error = B_OK;
	BMessageField *field = FindData(name, type, error);

	if (error < B_OK)
		return error;

	if (!field)
		return B_ERROR;

	field->ReplaceItem(index, buffer);
	return error;
}


status_t
BMessageBody::RemoveData(const char *name, int32 index)
{
	if (index < 0) {
		return B_BAD_VALUE;
	}

	status_t error = B_OK;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (index < field->CountItems()) {
			field->RemoveItem(index);

			if (field->CountItems() == 0) {
				RemoveName(name);
			}
		} else
			error = B_BAD_INDEX;
	}

	return error;
}


status_t
BMessageBody::RemoveName(const char *name)
{
	status_t error = B_OK;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		fFields.RemoveItem(field);
		delete field;
	}

	return error;
}


status_t
BMessageBody::MakeEmpty()
{
	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);
		delete field;
	}

	fFields.MakeEmpty();
	return B_OK;
}


bool
BMessageBody::HasData(const char *name, type_code type, int32 n) const
{
	if (!name || n < 0)
		return false;

	status_t error;
	BMessageField *field = FindData(name, type, error);

	if (!field)
		return false;

	if (n >= field->CountItems())
		return false;

	return true;
}


status_t
BMessageBody::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *numBytes) const
{
	status_t error = B_OK;
	*data = NULL;

	BMessageField *field = FindData(name, type, error);
	if (error >= B_OK) {
		if (index >= field->CountItems())
			return B_BAD_INDEX;

		if (data)
			*data = field->BufferAt(index);

		if (numBytes)
			*numBytes = field->SizeAt(index);
	}

	return error;
}


BMessageField *
BMessageBody::FindData(const char *name, type_code type, status_t &error) const
{
	if (!name) {
		error = B_BAD_VALUE;
		return NULL;
	}

	for (int32 index = 0; index < fFields.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFields.ItemAt(index);

		if (strcmp(name, field->Name()) == 0) {
			if (type != B_ANY_TYPE && field->Type() != type) {
				error = B_BAD_TYPE;
				return NULL;
			}

			error = B_OK;
			return field;
		}
	}

	error = B_NAME_NOT_FOUND;
	return NULL;
}

} // namespace BPrivate
