/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ChangePointCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-ChangePointCmd"


using std::nothrow;

// constructor
ChangePointCommand::ChangePointCommand(VectorPath* path,
									  int32 index,
 									  const int32* selected,
									  int32 count)
	: PathCommand(path),
	  fIndex(index),
	  fOldSelection(NULL),
	  fOldSelectionCount(count)
{
	if (fPath && !fPath->GetPointsAt(fIndex, fPoint, fPointIn, fPointOut, &fConnected))
		fPath = NULL;
	if (fOldSelectionCount > 0 && selected) {
		fOldSelection = new (nothrow) int32[fOldSelectionCount];
		memcpy(fOldSelection, selected, fOldSelectionCount * sizeof(int32));
	}
}

// destructor
ChangePointCommand::~ChangePointCommand()
{
	delete[] fOldSelection;
}

// InitCheck
status_t
ChangePointCommand::InitCheck()
{
	// TODO: figure out if selection changed!!!
	// (this command is also used to undo changes to the selection)
	// (but tracking the selection does not yet work in Icon-O-Matic)
	
	status_t ret = PathCommand::InitCheck();
	if (ret < B_OK)
		return ret;

	BPoint point;
	BPoint pointIn;
	BPoint pointOut;
	bool connected;
	if (!fPath->GetPointsAt(fIndex, point, pointIn, pointOut, &connected))
		return B_ERROR;

	if (point != fPoint || pointIn != fPointIn
		|| pointOut != fPointOut || connected != fConnected)
		return B_OK;

	return B_ERROR;
}

// Perform
status_t
ChangePointCommand::Perform()
{
	// path point is already changed
	return B_OK;
}

// Undo
status_t
ChangePointCommand::Undo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	// set the point to the remembered state and
	// save the previous state of the point
	BPoint point;
	BPoint pointIn;
	BPoint pointOut;
	bool connected;
	if (fPath->GetPointsAt(fIndex, point, pointIn, pointOut, &connected)
		&& fPath->SetPoint(fIndex, fPoint, fPointIn, fPointOut, fConnected)) {
		// toggle the remembered settings
		fPoint = point;
		fPointIn = pointIn;
		fPointOut = pointOut;
		fConnected = connected;
		// restore old selection
		_Select(fOldSelection, fOldSelectionCount);
	} else {
		status = B_ERROR;
	}

	return status;
}

// Redo
status_t
ChangePointCommand::Redo()
{
	status_t status = Undo();
	if (status >= B_OK)
		_Select(&fIndex, 1);
	return status;
}

// GetName
void
ChangePointCommand::GetName(BString& name)
{
//	name << _GetString(MODIFY_CONTROL_POINT, "Modify Control Point");
	name << B_TRANSLATE("Modify Control Point");
}
