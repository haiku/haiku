/*
 * Copyright 2006-2112, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2021, Andrew Lindesay <apl@lindesay.co.nz>
 * Distributed under the terms of the MIT License.
 */

#include "CompoundEdit.h"

#include <stdio.h>


CompoundEdit::CompoundEdit(const char* name)
	: UndoableEdit()
	, fEdits()
	, fName(name)
{
}


CompoundEdit::~CompoundEdit()
{
}


status_t
CompoundEdit::InitCheck()
{
	return B_OK;
}


status_t
CompoundEdit::Perform(EditContext& context)
{
	status_t status = B_OK;

	int32 count = static_cast<int32>(fEdits.size());
	int32 i = 0;
	for (; i < count; i++) {
		status = fEdits[i]->Perform(context);
		if (status != B_OK)
			break;
	}

	if (status != B_OK) {
		// roll back
		i--;
		for (; i >= 0; i--) {
			fEdits[i]->Undo(context);
		}
	}

	return status;
}


status_t
CompoundEdit::Undo(EditContext& context)
{
	status_t status = B_OK;

	int32 count = static_cast<int32>(fEdits.size());
	int32 i = count - 1;
	for (; i >= 0; i--) {
		status = fEdits[i]->Undo(context);
		if (status != B_OK)
			break;
	}

	if (status != B_OK) {
		// roll back
		i++;
		for (; i < count; i++) {
			fEdits[i]->Redo(context);
		}
	}

	return status;
}


status_t
CompoundEdit::Redo(EditContext& context)
{
	status_t status = B_OK;

	int32 count = static_cast<int32>(fEdits.size());
	int32 i = 0;
	for (; i < count; i++) {
		status = fEdits[i]->Redo(context);
		if (status != B_OK)
			break;
	}

	if (status != B_OK) {
		// roll back
		i--;
		for (; i >= 0; i--) {
			fEdits[i]->Undo(context);
		}
	}

	return status;
}


void
CompoundEdit::GetName(BString& name)
{
	name << fName;
}


void
CompoundEdit::AppendEdit(const UndoableEditRef& edit)
{
	fEdits.push_back(edit);
}
