/*
 * Copyright 2014, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "TextSelection.h"


TextSelection::TextSelection()
	:
	fAnchor(0),
	fCaret(0)
{
}


TextSelection::TextSelection(int32 anchor, int32 caret)
	:
	fAnchor(anchor),
	fCaret(caret)
{
}


TextSelection::TextSelection(const TextSelection& other)
	:
	fAnchor(other.fAnchor),
	fCaret(other.fCaret)
{
}


TextSelection&
TextSelection::operator=(const TextSelection& other)
{
	if (this == &other)
		return *this;

	fAnchor = other.fAnchor;
	fCaret = other.fCaret;
	return *this;
}


bool
TextSelection::operator==(const TextSelection& other) const
{
	return (this == &other)
		|| (fAnchor == other.fAnchor && fCaret == other.fCaret);
}


bool
TextSelection::operator!=(const TextSelection& other) const
{
	return !(*this == other);
}


void
TextSelection::SetAnchor(int32 anchor)
{
	fAnchor = anchor;
}


void
TextSelection::SetCaret(int32 caret)
{
	fCaret = caret;
}

