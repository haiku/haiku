//------------------------------------------------------------------------------
//	Copyright (c) 2003-2004, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		InlineInput.cpp
//	Author:			Stefano Ceccherini (burton666@libero.it)
//	Description:	Helper class to handle input method requests
//------------------------------------------------------------------------------

// TODO: he bebook says we should highlight in blue/red different "clauses".
// Though it looks like what really matters is the "selection" field in
// the BMessage sent by the input method addon. Have I missed something ?

#include "InlineInput.h"

#include <cstdlib>

struct clause
{
	int32 start;
	int32 end;
};


/*! \brief Constructs a _BInlineInput_ object.
	\param messenger The BMessenger of the input server method addon.
*/
_BInlineInput_::_BInlineInput_(BMessenger messenger)
	:
	fMessenger(messenger),
	fActive(false),
	fOffset(0),
	fLength(0),
	fSelectionOffset(0),
	fSelectionLength(0),
	fNumClauses(0),
	fClauses(NULL)
{
}


/*! \brief Destructs the object, free the allocated memory.
*/
_BInlineInput_::~_BInlineInput_()
{
	ResetClauses();
}


/*! \brief Returns a pointer to the Input Server Method BMessenger
	which requested the transaction.
*/
const BMessenger *
_BInlineInput_::Method() const
{
	return &fMessenger;
}


bool
_BInlineInput_::IsActive() const
{
	return fActive;
}


void
_BInlineInput_::SetActive(bool active)
{
	fActive = active;
}


/*! \brief Return the length of the inputted text.
*/
int32
_BInlineInput_::Length() const
{
	return fLength;
}


/*! \brief Sets the length of the text inputted with the input method.
	\param len The length of the text, extracted from the
	B_INPUT_METHOD_CHANGED BMessage.
*/
void
_BInlineInput_::SetLength(int32 len)
{
	fLength = len;
}


/*! \brief Returns the offset into the BTextView of the text.
*/
int32
_BInlineInput_::Offset() const
{
	return fOffset;
}


/*! \brief Sets the offset into the BTextView of the text.
	\param offset The offset where the text has been inserted.
*/
void
_BInlineInput_::SetOffset(int32 offset)
{
	fOffset = offset;
}


int32
_BInlineInput_::SelectionLength() const
{
	return fSelectionLength;
}


void
_BInlineInput_::SetSelectionLength(int32 length)
{
	fSelectionLength = length;
}


int32
_BInlineInput_::SelectionOffset() const
{
	return fSelectionOffset;
}


void
_BInlineInput_::SetSelectionOffset(int32 offset)
{
	fSelectionOffset = offset;
}


/*! \brief Adds a clause (see "The Input Server" sez. for details).
	\param start The offset into the string where the clause starts.
	\param end The offset into the string where the clause finishes.
*/
void
_BInlineInput_::AddClause(int32 start, int32 end)
{
	fClauses = (clause *)realloc(fClauses, ++fNumClauses * sizeof(clause));
	fClauses[fNumClauses - 1].start = start;
	fClauses[fNumClauses - 1].end = end;
}


/*! \brief Gets the clause at the given index.
	\param index The index of the clause to get.
	\param start A pointer to an integer which will contain the clause's start offset.
	\param end A pointer to an integer which will contain the clause's end offset.
	\return \c true if the clause exists, \c false if not.
*/
bool
_BInlineInput_::GetClause(int32 index, int32 *start, int32 *end) const
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
_BInlineInput_::CountClauses() const
{
	return fNumClauses;
}


/*! \brief Deletes any added clause.
*/
void
_BInlineInput_::ResetClauses()
{
	fNumClauses = 0;
	free(fClauses);
	fClauses = NULL;
}
