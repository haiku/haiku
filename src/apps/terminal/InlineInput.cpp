/*
 * Copyright 2003-2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include "InlineInput.h"

#include <cstdlib>

struct clause
{
	int32 start;
	int32 end;
};


InlineInput::InlineInput(BMessenger messenger)
	:
	fMessenger(messenger),
	fActive(false),
	fSelectionOffset(0),
	fSelectionLength(0),
	fNumClauses(0),
	fClauses(NULL)
{
}


InlineInput::~InlineInput()
{
	ResetClauses();
}


const BMessenger *
InlineInput::Method() const
{
	return &fMessenger;
}


const char *
InlineInput::String() const
{
	return fString.String();
}


void
InlineInput::SetString(const char *string)
{
	fString = string;
}

		
bool
InlineInput::IsActive() const
{
	return fActive;
}


void
InlineInput::SetActive(bool active)
{
	fActive = active;
}


int32
InlineInput::SelectionLength() const
{
	return fSelectionLength;
}


void
InlineInput::SetSelectionLength(int32 length)
{
	fSelectionLength = length;
}


int32
InlineInput::SelectionOffset() const
{
	return fSelectionOffset;
}


void
InlineInput::SetSelectionOffset(int32 offset)
{
	fSelectionOffset = offset;
}


bool
InlineInput::AddClause(int32 start, int32 end)
{
	void *newData = realloc(fClauses, (fNumClauses + 1) * sizeof(clause));
	if (newData == NULL)
		return false;

	fClauses = (clause *)newData;
	fClauses[fNumClauses].start = start;
	fClauses[fNumClauses].end = end;
	fNumClauses++;
	return true;
}


bool
InlineInput::GetClause(int32 index, int32 *start, int32 *end) const
{
	bool result = false;
	if (index >= 0 && index < fNumClauses) {
		result = true;
		clause *clause = &fClauses[index];
		if (start)
			*start = clause->start;
		if (end)
			*end = clause->end;
	}
	
	return result;
}


int32
InlineInput::CountClauses() const
{
	return fNumClauses;
}


void
InlineInput::ResetClauses()
{
	fNumClauses = 0;
	free(fClauses);
	fClauses = NULL;
}
