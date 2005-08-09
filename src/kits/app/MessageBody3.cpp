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
#include "MessageBody3.h"
#include "MessageField3.h"
#include "MessageUtils3.h"

namespace BPrivate {

BMessageBody::BMessageBody()
{
	InitCommon();
}


BMessageBody::BMessageBody(const BMessageBody &other)
{
	InitCommon();
	*this = other;
}


BMessageBody::~BMessageBody()
{
	MakeEmpty();
	delete fFieldTable;
}


BMessageBody &
BMessageBody::operator=(const BMessageBody &other)
{
	if (this != &other) {
		MakeEmpty();
		// the flat buffer has no last entry flag included
		BMemoryIO memoryIO(other.FlatBuffer(), other.FlattenedSize() - 1);
		Unflatten(&memoryIO, other.FlattenedSize() - 1);
	}

	return *this;
}


status_t
BMessageBody::InitCommon()
{
	fFieldTableSize = 100;
	fFieldTable = new BMessageField *[fFieldTableSize];
	HashClear();
	return B_OK;
}


status_t
BMessageBody::GetInfo(type_code typeRequested, int32 which, char **name,
	type_code *typeReturned, int32 *count) const
{
	int32 index = 0;
	int32 fieldCount = fFieldList.CountItems();
	BMessageField *field = NULL;
	bool found = false;

	for (int32 fieldIndex = 0; fieldIndex < fieldCount; fieldIndex++) {
		field = (BMessageField *)fFieldList.ItemAt(fieldIndex);

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
	status_t error = B_ERROR;
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
	status_t error = B_ERROR;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (typeFound)
			*typeFound = field->Type();

		if (fixedSize)
			*fixedSize = field->FixedSize();
	}

	return error;
}


int32
BMessageBody::CountNames(type_code type) const
{
	if (type == B_ANY_TYPE)
		return fFieldList.CountItems();

	int32 count = 0;
	for (int32 index = 0; index < fFieldList.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);
		if (field->Type() == type)
			count++;
	}

	return count;
}


bool
BMessageBody::IsEmpty() const
{
	return fFieldList.CountItems() == 0;
}


void
BMessageBody::PrintToStream() const
{
	for (int32 index = 0; index < fFieldList.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);
		field->PrintToStream();
		printf("\n");
	}
}


status_t
BMessageBody::Rename(const char *oldName, const char *newName)
{
	status_t error = B_ERROR;
	BMessageField *field = FindData(oldName, B_ANY_TYPE, error);

	if (!field)
		return error;

	field->SetName(newName);
	HashInsert(HashRemove(oldName));
	return B_OK;
}


ssize_t
BMessageBody::FlattenedSize() const
{
	// one more for the last entry flag
	return fFlatBuffer.BufferLength() + 1;
}


status_t
BMessageBody::Flatten(BDataIO *stream) const
{
	stream->Write(fFlatBuffer.Buffer(), fFlatBuffer.BufferLength());

	uint8 lastField = MSG_LAST_ENTRY;
	status_t error = stream->Write(&lastField, sizeof(lastField));

	if (error < B_OK)
		return error;
	if (error == 0)
		return B_ERROR;

	return B_OK;
}


status_t
BMessageBody::Unflatten(BDataIO *stream, int32 length)
{
	if (length <= 0)
		return B_OK;

	fFlatBuffer.SetSize(length);
	status_t error = stream->Read((void *)fFlatBuffer.Buffer(), length);

	if (error < B_OK)
		return B_ERROR;

	int32 offset = 0;
	uint8 *location = (uint8 *)fFlatBuffer.Buffer();
	while (offset < error && *location & MSG_FLAG_VALID) {
		BMessageField *field = new BMessageField(this);
		int32 fieldLength = field->Unflatten(offset);

		if (fieldLength <= 0) {
			field->MakeEmpty();
			delete field;
			MakeEmpty();
			return B_ERROR;
		}

		location += fieldLength;
		offset += fieldLength;

		fFieldList.AddItem(field);
		HashInsert(field);
	}

	// set the buffer to the actual size
	fFlatBuffer.SetSize(offset);
	return B_OK;
}


status_t
BMessageBody::AddData(const char *name, type_code type, const void *item,
	ssize_t length, bool fixedSize)
{
	status_t error = B_ERROR;
	BMessageField *field = AddData(name, type, error);

	if (!field)
		return error;

	if (fixedSize) {
		error = field->SetFixedSize(length);
		if (error != B_OK)
			return error;
	}

	field->AddItem(item, length);
	return B_OK;
}


status_t
BMessageBody::AddData(const char *name, type_code type, void **buffer,
	ssize_t length)
{
	status_t error = B_ERROR;
	BMessageField *field = AddData(name, type, error);

	if (!field)
		return error;

	*buffer = field->AddItem(length);
	return B_OK;
}


BMessageField *
BMessageBody::AddData(const char *name, type_code type, status_t &error)
{
	if (type == B_ANY_TYPE) {
		error = B_BAD_VALUE;
		return NULL;
	}

	BMessageField *foundField = FindData(name, type, error);

	if (error < B_OK) {
		if (error == B_NAME_NOT_FOUND) {
			// reset the error - we will create the field
			error = B_OK;
		} else {
			// looking for B_BAD_TYPE here in particular, which would indicate
			// that we tried to add data of type X when we already had data of
			// type Y with the same name
			return NULL;
		}
	}

	if (foundField)
		return foundField;

	// add a new field if it's not yet present
	BMessageField *newField = new BMessageField(this,
		fFlatBuffer.BufferLength(), name, type);

	fFieldList.AddItem(newField);
	HashInsert(newField);

	return newField;
}


status_t
BMessageBody::ReplaceData(const char *name, type_code type, int32 index,
	const void *item, ssize_t length)
{
	if (type == B_ANY_TYPE)
		return B_BAD_VALUE;

	status_t error = B_ERROR;
	BMessageField *field = FindData(name, type, error);

	if (!field)
		return error;

	if (field->FixedSize()) {
		ssize_t itemSize = 0;
		void *item = field->ItemAt(0, &itemSize);

		if (item && itemSize != length)
			return B_BAD_VALUE;
	}

	field->ReplaceItem(index, item, length);
	return B_OK;
}


status_t
BMessageBody::ReplaceData(const char *name, type_code type, int32 index,
	void **buffer, ssize_t length)
{
	if (type == B_ANY_TYPE)
		return B_BAD_VALUE;

	status_t error = B_ERROR;
	BMessageField *field = FindData(name, type, error);

	if (!field)
		return error;

	*buffer = field->ReplaceItem(index, length);
	return B_OK;
}


status_t
BMessageBody::RemoveData(const char *name, int32 index)
{
	if (index < 0)
		return B_BAD_VALUE;

	status_t error = B_ERROR;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (index < field->CountItems()) {
			if (field->CountItems() == 1)
				RemoveName(name);
			else
				field->RemoveItem(index);
		} else
			error = B_BAD_INDEX;
	}

	return error;
}


status_t
BMessageBody::RemoveName(const char *name)
{
	status_t error = B_ERROR;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (!field)
		return error;

	fFieldList.RemoveItem(field);
	HashRemove(name);
	field->MakeEmpty();
	field->RemoveSelf();
	delete field;

	return B_OK;
}


status_t
BMessageBody::MakeEmpty()
{
	for (int32 index = 0; index < fFieldList.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);
		field->MakeEmpty();
		delete field;
	}

	fFieldList.MakeEmpty();
	fFlatBuffer.SetSize(0);
	HashClear();
	return B_OK;
}


bool
BMessageBody::HasData(const char *name, type_code type, int32 index) const
{
	if (!name || index < 0)
		return false;

	status_t error = B_ERROR;
	BMessageField *field = FindData(name, type, error);

	if (!field)
		return false;

	if (index >= field->CountItems())
		return false;

	return true;
}


status_t
BMessageBody::FindData(const char *name, type_code type, int32 index,
	const void **data, ssize_t *numBytes) const
{
	if (!data)
		return B_BAD_VALUE;

	status_t error = B_ERROR;
	*data = NULL;

	BMessageField *field = FindData(name, type, error);
	if (error >= B_OK) {
		if (index >= field->CountItems())
			return B_BAD_INDEX;

		*data = field->ItemAt(index, numBytes);
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

	BMessageField *field = HashLookup(name);

	if (field) {
		if (type != B_ANY_TYPE && field->Type() != type) {
			error = B_BAD_TYPE;
			return NULL;
		}

		error = B_OK;
		return field;
	}

	error = B_NAME_NOT_FOUND;
	return NULL;
}


uint8 *
BMessageBody::FlatInsert(int32 offset, ssize_t oldLength, ssize_t newLength)
{
	if (oldLength == newLength)
		return FlatBuffer() + offset;

	ssize_t change = newLength - oldLength;
	ssize_t bufferLength = fFlatBuffer.BufferLength();

	if (change > 0) {
		fFlatBuffer.SetSize(bufferLength + change);

		// usual case for adds
		if (offset == bufferLength)
			return FlatBuffer() + offset;
	}

	ssize_t length = bufferLength - offset - oldLength;
	if (length > 0) {
		uint8 *location = FlatBuffer() + offset;
		memmove(location + newLength, location + oldLength, length);
	}

	if (change < 0)
		fFlatBuffer.SetSize(bufferLength + change);

	for (int32 index = fFieldList.CountItems() - 1; index >= 0; index--) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);
		if (field->Offset() <= offset)
			break;

		field->SetOffset(field->Offset() + change);
	}

	return FlatBuffer() + offset;
}


void
BMessageBody::HashInsert(BMessageField *field)
{
	uint32 index = HashString(field->Name()) % fFieldTableSize;
	field->SetNext(fFieldTable[index]);
	fFieldTable[index] = field;
}


BMessageField *
BMessageBody::HashLookup(const char *name) const
{
	uint32 index = HashString(name) % fFieldTableSize;
	BMessageField *result = fFieldTable[index];

	while (result) {
		if (strcmp(result->Name(), name) == 0)
			return result;

		result = result->Next();
	}

	return NULL;
}


BMessageField *
BMessageBody::HashRemove(const char *name)
{
	uint32 index = HashString(name) % fFieldTableSize;
	BMessageField *result = fFieldTable[index];
	BMessageField *last = NULL;

	while (result) {
		if (strcmp(result->Name(), name) == 0) {
			if (last)
				last->SetNext(result->Next());
			else
				fFieldTable[index] = result->Next();

			return result;
		}

		last = result;
		result = result->Next();
	}

	return NULL;
}


void
BMessageBody::HashClear()
{
	memset(fFieldTable, 0, fFieldTableSize * sizeof(BMessageField *));
}


uint32
BMessageBody::HashString(const char *string) const
{
	char ch;
	uint32 result = 0;

	while ((ch = *string++) != 0) {
		result = (result << 7) ^ (result >> 24);
		result ^= ch;
	}

	result ^= result << 12;
	return result;
}

} // namespace BPrivate
