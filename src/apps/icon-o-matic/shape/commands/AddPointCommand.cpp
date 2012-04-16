/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "AddPointCommand.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-AddPointCmd"


// constructor
AddPointCommand::AddPointCommand(VectorPath* path,
								 int32 index,
								 const int32* selected,
								 int32 count)
	: PathCommand(path),
	  fIndex(index),
	  fOldSelection(NULL),
	  fOldSelectionCount(count)
{
	if (fOldSelectionCount > 0 && selected) {
		fOldSelection = new int32[fOldSelectionCount];
		memcpy(fOldSelection, selected, fOldSelectionCount * sizeof(int32));
	}
}

// destructor
AddPointCommand::~AddPointCommand()
{
	delete[] fOldSelection;
}

// Perform
status_t
AddPointCommand::Perform()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	// path point is already added,
	// but we don't know the parameters yet
	if (!fPath->GetPointsAt(fIndex, fPoint, fPointIn, fPointOut))
		status = B_NO_INIT;

	return status;
}

// Undo
status_t
AddPointCommand::Undo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	// remove point
	if (fPath->RemovePoint(fIndex)) {
		// restore selection before adding point
		_Select(fOldSelection, fOldSelectionCount);
	} else {
		status = B_ERROR;
	}

	return status;
}

// Redo
status_t
AddPointCommand::Redo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	AutoNotificationSuspender _(fPath);

	// add point again
	if (fPath->AddPoint(fPoint, fIndex)) {
		fPath->SetPoint(fIndex, fPoint, fPointIn, fPointOut, true);
		// select added point
		_Select(&fIndex, 1);
	} else {
		status = B_ERROR;
	}

	return status;
}

// GetName
void
AddPointCommand::GetName(BString& name)
{
//	name << _GetString(ADD_CONTROL_POINT, "Add Control Point");
	name << B_TRANSLATE("Add Control Point");
}
