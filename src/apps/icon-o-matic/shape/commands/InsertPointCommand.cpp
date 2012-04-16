/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "InsertPointCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-InsertPointCmd"


using std::nothrow;

// constructor
InsertPointCommand::InsertPointCommand(VectorPath* path,
									  int32 index,
									  const int32* selected,
									  int32 count)
	: PathCommand(path),
	  fIndex(index),
	  fOldSelection(NULL),
	  fOldSelectionCount(count)
{
	if (fPath && (!fPath->GetPointsAt(fIndex, fPoint, fPointIn, fPointOut)
				  || !fPath->GetPointOutAt(fIndex - 1, fPreviousOut)
				  || !fPath->GetPointInAt(fIndex + 1, fNextIn))) {
		fPath = NULL;
	}
	if (fOldSelectionCount > 0 && selected) {
		fOldSelection = new (nothrow) int32[count];
		memcpy(fOldSelection, selected, count * sizeof(int32));
	}
}

// destructor
InsertPointCommand::~InsertPointCommand()
{
	delete[] fOldSelection;
}

// Perform
status_t
InsertPointCommand::Perform()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	// path point is already added
	// but in/out points might still have changed
	fPath->GetPointInAt(fIndex, fPointIn);
	fPath->GetPointOutAt(fIndex, fPointOut);

	return status;
}

// Undo
status_t
InsertPointCommand::Undo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	AutoNotificationSuspender _(fPath);

	// remove the inserted point
	if (fPath->RemovePoint(fIndex)) {
		// remember current previous "out" and restore it
		BPoint previousOut = fPreviousOut;
		fPath->GetPointOutAt(fIndex - 1, fPreviousOut);
		fPath->SetPointOut(fIndex - 1, previousOut);
		// remember current next "in" and restore it
		BPoint nextIn = fNextIn;
		fPath->GetPointInAt(fIndex, fNextIn);
		fPath->SetPointIn(fIndex, nextIn);
		// restore previous selection
		_Select(fOldSelection, fOldSelectionCount);
	} else {
		status = B_ERROR;
	}

	return status;
}

// Redo
status_t
InsertPointCommand::Redo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;


	AutoNotificationSuspender _(fPath);

	// insert point again
	if (fPath->AddPoint(fPoint, fIndex)) {
		fPath->SetPoint(fIndex, fPoint, fPointIn, fPointOut, true);
		// remember current previous "out" and restore it
		BPoint previousOut = fPreviousOut;
		fPath->GetPointOutAt(fIndex - 1, fPreviousOut);
		fPath->SetPointOut(fIndex - 1, previousOut);
		// remember current next "in" and restore it
		BPoint nextIn = fNextIn;
		fPath->GetPointInAt(fIndex + 1, fNextIn);
		fPath->SetPointIn(fIndex + 1, nextIn);
		// select inserted point
		_Select(&fIndex, 1);
	} else {
		status = B_ERROR;
	}

	return status;
}

// GetName
void
InsertPointCommand::GetName(BString& name)
{
//	name << _GetString(INSERT_CONTROL_POINT, "Insert Control Point");
	name << B_TRANSLATE("Insert Control Point");
}

