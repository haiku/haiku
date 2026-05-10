/*
 * Copyright 2015, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "UndoableEditListener.h"


UndoableEditListener::UndoableEditListener()
	:
	BReferenceable()
{
}


UndoableEditListener::~UndoableEditListener()
{
}


void
UndoableEditListener::UndoableEditHappened(const TextDocument* document,
	const UndoableEditRef& edit)
{
}
