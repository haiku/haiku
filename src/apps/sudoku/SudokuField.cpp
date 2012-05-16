/*
 * Copyright 2007-2010, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "SudokuField.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Message.h>
#include <OS.h>


const char*
mask(uint32 set)
{
	static char text[64];
	uint32 i = 0;
	for (int32 number = 9; number > 0; number--) {
		text[i++] = set & (1UL << (number - 1)) ? number + '0' : '-';
	}

	text[i] = '\0';
	return text;
}


//	#pragma mark -


SudokuField::field::field()
	:
	hint_mask(0),
	valid_mask(~0),
	flags(0),
	value(0)
{
}


//	#pragma mark -


SudokuField::SudokuField(uint32 size)
	:
	fSize(size * size),
	fBlockSize(size)
{
	fFields = new (std::nothrow) field[fSize * fSize];
	fMaxMask = (1UL << fSize) - 1;
}


SudokuField::SudokuField(const BMessage* archive)
{
	if (archive->FindInt32("block size", (int32*)&fBlockSize) != B_OK)
		return;

	fSize = fBlockSize * fBlockSize;
	fMaxMask = (1UL << fSize) - 1;

	uint32 count = fSize * fSize;
	fFields = new (std::nothrow) field[count];
	if (fFields == NULL)
		return;

	for (uint32 i = 0; i < count; i++) {
		struct field& field = fFields[i];

		if (archive->FindInt32("value", i, (int32*)&field.value) != B_OK
			|| archive->FindInt32("valid mask", i,
					(int32*)&field.valid_mask) != B_OK
			|| archive->FindInt32("hint mask", i,
					(int32*)&field.hint_mask) != B_OK
			|| archive->FindInt32("flags", i, (int32*)&field.flags) != B_OK)
			break;
	}
}


SudokuField::SudokuField(const SudokuField& other)
	: BArchivable(other)
{
	fSize = other.fSize;
	fBlockSize = other.fBlockSize;
	fMaxMask = other.fMaxMask;

	fFields = new (std::nothrow) field[fSize * fSize];
	if (fFields != NULL)
		memcpy(fFields, other.fFields, sizeof(field) * fSize * fSize);
}


SudokuField::~SudokuField()
{
	delete[] fFields;
}


status_t
SudokuField::InitCheck()
{
	if (fBlockSize == 0)
		return B_BAD_VALUE;
	return fFields == NULL ? B_NO_MEMORY : B_OK;
}


status_t
SudokuField::Archive(BMessage* archive, bool deep) const
{
	status_t status = BArchivable::Archive(archive, deep);
	if (status == B_OK)
		archive->AddInt32("block size", fBlockSize);
	if (status < B_OK)
		return status;

	uint32 count = fSize * fSize;
	for (uint32 i = 0; i < count && status == B_OK; i++) {
		struct field& field = fFields[i];
		status = archive->AddInt32("value", field.value);
		if (status == B_OK)
			status = archive->AddInt32("valid mask", field.valid_mask);
		if (status == B_OK)
			status = archive->AddInt32("hint mask", field.hint_mask);
		if (status == B_OK)
			status = archive->AddInt32("flags", field.flags);
	}

	return status;
}


/*static*/ SudokuField*
SudokuField::Instantiate(BMessage* archive)
{
	if (!validate_instantiation(archive, "SudokuField"))
		return NULL;

	return new SudokuField(archive);
}


void
SudokuField::Reset()
{
	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			struct field& field = _FieldAt(x, y);
			field.value = 0;
			field.flags = 0;
			field.hint_mask = 0;
			field.valid_mask = fMaxMask;
		}
	}
}


status_t
SudokuField::SetTo(char base, const char* data)
{
	if (data != NULL && strlen(data) < fSize * fSize)
		return B_BAD_VALUE;

	Reset();

	if (data == NULL)
		return B_OK;

	uint32 i = 0;

	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			uint32 value = data[i++] - base;
			if (value) {
				struct field& field = _FieldAt(x, y);
				field.value = value;
				field.flags = kInitialValue;
			}
		}
	}

	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			_ComputeValidMask(x, y, false);
		}
	}

	return B_OK;
}


void
SudokuField::SetTo(const SudokuField* field)
{
	if (field == NULL) {
		Reset();
		return;
	}

	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			_FieldAt(x, y) = field->_FieldAt(x, y);
		}
	}
}


void
SudokuField::Dump()
{
	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			if (x != 0 && x % fBlockSize == 0)
				putchar(' ');
			printf("%lu", ValueAt(x, y));
		}
		putchar('\n');
	}
}


bool
SudokuField::IsSolved() const
{
	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			if (!_ValidValueAt(x, y))
				return false;
		}
	}

	return true;
}


bool
SudokuField::IsEmpty() const
{
	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			if (ValueAt(x, y) != 0)
				return false;
		}
	}

	return true;
}


bool
SudokuField::IsValueCompleted(uint32 value) const
{
	uint32 count = 0;
	for (uint32 y = 0; y < fSize; y++) {
		for (uint32 x = 0; x < fSize; x++) {
			if (ValueAt(x, y) == value)
				count++;
		}
	}

	return count == Size();
}


void
SudokuField::SetHintMaskAt(uint32 x, uint32 y, uint32 hintMask)
{
	_FieldAt(x, y).hint_mask = hintMask;
}


uint32
SudokuField::HintMaskAt(uint32 x, uint32 y) const
{
	return _FieldAt(x, y).hint_mask;
}


void
SudokuField::SetValidMaskAt(uint32 x, uint32 y, uint32 validMask)
{
	_FieldAt(x, y).valid_mask = validMask & fMaxMask;
}


uint32
SudokuField::ValidMaskAt(uint32 x, uint32 y) const
{
	return _FieldAt(x, y).valid_mask;
}


void
SudokuField::SetFlagsAt(uint32 x, uint32 y, uint32 flags)
{
	_FieldAt(x, y).flags = flags;
}


uint32
SudokuField::FlagsAt(uint32 x, uint32 y) const
{
	return _FieldAt(x, y).flags;
}


void
SudokuField::SetValueAt(uint32 x, uint32 y, uint32 value, bool setSolved)
{
	_FieldAt(x, y).value = value;
	_FieldAt(x, y).hint_mask = 0;
	_UpdateValidMaskChanged(x, y, setSolved);
}


uint32
SudokuField::ValueAt(uint32 x, uint32 y) const
{
	return _FieldAt(x, y).value;
}


bool
SudokuField::_ValidValueAt(uint32 x, uint32 y) const
{
	uint32 value = _FieldAt(x, y).value;
	if (!value)
		return false;

	value = 1UL << (value - 1);
	return (_FieldAt(x, y).valid_mask & value) != 0;
}


void
SudokuField::_ComputeValidMask(uint32 x, uint32 y, bool setSolved)
{
	if (ValueAt(x, y))
		return;

	// check row

	uint32 foundMask = 0;
	for (uint32 i = 0; i < fSize; i++) {
		uint32 value = ValueAt(i, y);
		if (value && _ValidValueAt(i, y))
			foundMask |= 1UL << (value - 1);
	}

	// check column

	for (uint32 i = 0; i < fSize; i++) {
		uint32 value = ValueAt(x, i);
		if (value && _ValidValueAt(x, i))
			foundMask |= 1UL << (value - 1);
	}

	// check block

	uint32 offsetX = x / fBlockSize * fBlockSize;
	uint32 offsetY = y / fBlockSize * fBlockSize;

	for (uint32 partY = 0; partY < fBlockSize; partY++) {
		for (uint32 partX = 0; partX < fBlockSize; partX++) {
			uint32 value = ValueAt(partX + offsetX, partY + offsetY);
			if (value && _ValidValueAt(partX + offsetX, partY + offsetY))
				foundMask |= 1UL << (value - 1);
		}
	}

	SetValidMaskAt(x, y, ~foundMask);

	if (setSolved) {
		// find the one set bit, if not more
		uint32 value = 0;
		for (uint32 i = 0; i < fSize; i++) {
			if ((foundMask & (1UL << i)) == 0) {
				if (value != 0) {
					value = 0;
					break;
				}

				value = i + 1;
			}
		}
		if (value != 0)
			SetValueAt(x, y, value, true);
	}
}


void
SudokuField::_UpdateValidMaskChanged(uint32 x, uint32 y, bool setSolved)
{
	// update row

	for (uint32 i = 0; i < fSize; i++) {
		_ComputeValidMask(i, y, setSolved);
	}

	// update column

	for (uint32 i = 0; i < fSize; i++) {
		if (i == y)
			continue;

		_ComputeValidMask(x, i, setSolved);
	}

	// update block

	uint32 offsetX = x / fBlockSize * fBlockSize;
	uint32 offsetY = y / fBlockSize * fBlockSize;

	for (uint32 partY = 0; partY < fBlockSize; partY++) {
		for (uint32 partX = 0; partX < fBlockSize; partX++) {
			if (partX + offsetX == x || partY + offsetY == y)
				continue;

			_ComputeValidMask(partX + offsetX, partY + offsetY, setSolved);
		}
	}
}


const SudokuField::field&
SudokuField::_FieldAt(uint32 x, uint32 y) const
{
	if (x >= fSize || y >= fSize)
		debugger("field outside bounds");

	return fFields[x + y * fSize];
}


SudokuField::field&
SudokuField::_FieldAt(uint32 x, uint32 y)
{
	if (x >= fSize || y >= fSize)
		debugger("field outside bounds");

	return fFields[x + y * fSize];
}


