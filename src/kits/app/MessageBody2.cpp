/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/* BMessageBody handles data storage and retrieval for BMessage. */

#include <stdio.h>
#include <string.h>
#include <TypeConstants.h>
#include "MessageBody2.h"
#include "MessageUtils2.h"

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

		for (int32 index = 0; index < other.fFieldList.CountItems(); index++) {
			BMessageField *otherField = (BMessageField *)other.fFieldList.ItemAt(index);
			BMessageField *newField = new BMessageField(*otherField);
			fFieldList.AddItem((void *)newField);
			HashInsert(newField);
		}
	}

	return *this;
}


status_t
BMessageBody::InitCommon()
{
	fFlattenedSize = -1;
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
	status_t error = B_OK;
	BMessageField *field = FindData(oldName, B_ANY_TYPE, error);

	if (!field)
		return B_NAME_NOT_FOUND;

	field->SetName(newName);
	HashInsert(HashRemove(oldName));
	fFlattenedSize = -1;
	return B_OK;
}


ssize_t
BMessageBody::FlattenedSize() const
{
	if (fFlattenedSize > 0)
		return fFlattenedSize;

	ssize_t size = 1; // for MSG_LAST_ENTRY

	for (int32 index = 0; index < fFieldList.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);

		uint8 flags = field->Flags();
		size += sizeof(flags) + sizeof(type_code);

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

		// name length byte and name length
		size += 1 + field->NameLength();

		size += field->TotalSize();
	}

	// cache the value for next time.
	// changing the body will reset this.
	fFlattenedSize = size;
	return size;
}


status_t
BMessageBody::Flatten(BDataIO *stream) const
{
	for (int32 index = 0; index < fFieldList.CountItems(); index++)
		((BMessageField *)fFieldList.ItemAt(index))->Flatten(stream);

	uint8 lastEntry = 0;
	size_t error = stream->Write(&lastEntry, sizeof(lastEntry));

	if (error > B_OK)
		return B_OK;
	else if (error == 0)
		return B_ERROR;

	return error;
}


status_t
BMessageBody::Unflatten(BDataIO *stream)
{
	if (!IsEmpty())
		MakeEmpty();

	BMessageField *newField = NULL;
	try {
		TReadHelper reader(stream);

		uint8 flags;
		reader(flags);

		while (flags != MSG_LAST_ENTRY) {
			newField = new BMessageField(flags, stream);
			fFieldList.AddItem(newField);
			HashInsert(newField);
			reader(flags);
		}
	} catch (status_t &error) {
		delete newField;
		return error;
	}

	return B_OK;
}


status_t
BMessageBody::AddData(const char *name, type_code type, size_t length,
	void **buffer, bool fixedSize, int32 count)
{
	status_t error = B_OK;
	BMessageField *newField = AddField(name, type, error);

	if (error < B_OK)
		return error;

	if (fixedSize) {
		error = newField->SetFixedSize(length, count);
		if (error < B_OK)
			return error;
	}

	*buffer = newField->AddItem(length);
	fFlattenedSize = -1;
	return error;
}


BMessageField *
BMessageBody::AddField(const char *name, type_code type, status_t &error)
{
	BMessageField *foundField = FindData(name, type, error);

	if (foundField) {
		error = B_OK;
		return foundField;
	}

	if (!foundField && error == B_NAME_NOT_FOUND) {
		// add a new field if it's not yet present
		BMessageField *newField = new BMessageField(name, type);
		fFieldList.AddItem(newField);
		HashInsert(newField);
		error = B_OK;
		return newField;
	}

	return NULL;
}


status_t
BMessageBody::ReplaceData(const char *name, type_code type, int32 index,
	size_t length, void **buffer)
{
	if (type == B_ANY_TYPE)
		return B_BAD_VALUE;

	status_t error = B_OK;
	BMessageField *field = FindData(name, type, error);

	if (error < B_OK)
		return error;

	if (!field)
		return B_ERROR;

	*buffer = field->ReplaceItem(index, length);
	fFlattenedSize = -1;
	return error;
}


status_t
BMessageBody::RemoveData(const char *name, int32 index)
{
	if (index < 0)
		return B_BAD_VALUE;

	status_t error = B_OK;
	BMessageField *field = FindData(name, B_ANY_TYPE, error);

	if (field) {
		if (index < field->CountItems()) {
			field->RemoveItem(index);
			fFlattenedSize = -1;

			if (field->CountItems() == 0)
				RemoveName(name);
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
		fFieldList.RemoveItem(field);
		fFlattenedSize = -1;
		HashRemove(name);
		delete field;
	}

	return error;
}


status_t
BMessageBody::MakeEmpty()
{
	for (int32 index = 0; index < fFieldList.CountItems(); index++) {
		BMessageField *field = (BMessageField *)fFieldList.ItemAt(index);
		delete field;
	}

	fFieldList.MakeEmpty();
	fFlattenedSize = -1;
	HashClear();
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

		*data = field->BufferAt(index, numBytes);
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
