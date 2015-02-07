/*
 * Copyright 2012, Stephan Aßmus <superstippi@gmx.de>
 * Distributed under the terms of the MIT License.
 */

#include "EditStack.h"

#include <stdio.h>


EditStack::EditStack()
	: fEdits()
{
}


EditStack::~EditStack()
{
}


bool
EditStack::Push(const UndoableEditRef& edit)
{
	return fEdits.Add(edit);
}


UndoableEditRef
EditStack::Pop()
{
	UndoableEditRef edit(Top());
	fEdits.Remove();
	return edit;
}


const UndoableEditRef&
EditStack::Top() const
{
	return fEdits.LastItem();
}


bool
EditStack::IsEmpty() const
{
	return fEdits.CountItems() == 0;
}
