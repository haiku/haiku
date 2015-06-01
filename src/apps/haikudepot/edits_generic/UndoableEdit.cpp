/*
 * Copyright 2006-2012, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#include "UndoableEdit.h"

#include <stdio.h>

#include <OS.h>
#include <String.h>


//static int32 sInstanceCount = 0;


UndoableEdit::UndoableEdit()
	:
	fTimeStamp(system_time())
{
//	sInstanceCount++;
//	printf("UndoableEdits: %ld\n", sInstanceCount);
}


UndoableEdit::~UndoableEdit()
{
//	sInstanceCount--;
//	printf("UndoableEdits: %ld\n", sInstanceCount);
}


status_t
UndoableEdit::InitCheck()
{
	return B_NO_INIT;
}


status_t
UndoableEdit::Perform(EditContext& context)
{
	return B_ERROR;
}


status_t
UndoableEdit::Undo(EditContext& context)
{
	return B_ERROR;
}


status_t
UndoableEdit::Redo(EditContext& context)
{
	return Perform(context);
}


void
UndoableEdit::GetName(BString& name)
{
	name << "Name of edit goes here.";
}


bool
UndoableEdit::UndoesPrevious(const UndoableEdit* previous)
{
	return false;
}


bool
UndoableEdit::CombineWithNext(const UndoableEdit* next)
{
	return false;
}


bool
UndoableEdit::CombineWithPrevious(const UndoableEdit* previous)
{
	return false;
}
