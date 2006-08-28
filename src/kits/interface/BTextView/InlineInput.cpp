/*
 * Copyright 2003-2006, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

// For a deeper understanding of this class, see the BeBook, sez.
// "The Input Server".
// TODO: the bebook says we should highlight in blue/red different "clauses".
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


/*! \brief Returns the length of the selection, if any.
*/
int32
_BInlineInput_::SelectionLength() const
{
	return fSelectionLength;
}


/*! \brief Sets the length of the selection.
	\param length The length of the selection.
*/
void
_BInlineInput_::SetSelectionLength(int32 length)
{
	fSelectionLength = length;
}


/*! \brief Returns the offset into the method string of the selection.
*/
int32
_BInlineInput_::SelectionOffset() const
{
	return fSelectionOffset;
}


/*! \brief Sets the offset into the method string of the selection.
	\param offset The offset where the selection starts.
*/
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
