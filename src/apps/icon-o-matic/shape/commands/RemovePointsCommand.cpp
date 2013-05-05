/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "RemovePointsCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-RemovePointsCmd"


using std::nothrow;

// constructor
// * when clicking a point in Remove mode, with other points selected
RemovePointsCommand::RemovePointsCommand(VectorPath* path,
										 int32 index,
										 const int32* selected,
										 int32 count)
	: PathCommand(path),
	  fIndex(NULL),
	  fPoint(NULL),
	  fPointIn(NULL),
	  fPointOut(NULL),
	  fConnected(NULL),
	  fCount(0),
	  fOldSelection(NULL),
	  fOldSelectionCount(count)
{
	_Init(&index, 1, selected, count);
}

// constructor
// * when hitting the Delete key, so the selected points are the
// same as the ones to be removed
RemovePointsCommand::RemovePointsCommand(VectorPath* path,
										 const int32* selected,
										 int32 count)
	: PathCommand(path),
	  fIndex(NULL),
	  fPoint(NULL),
	  fPointIn(NULL),
	  fPointOut(NULL),
	  fConnected(NULL),
	  fCount(0),
	  fOldSelection(NULL),
	  fOldSelectionCount(count)
{
	_Init(selected, count, selected, count);
}

// destructor
RemovePointsCommand::~RemovePointsCommand()
{
	delete[] fIndex;
	delete[] fPoint;
	delete[] fPointIn;
	delete[] fPointOut;
	delete[] fConnected;
	delete[] fOldSelection;
}

// InitCheck
status_t
RemovePointsCommand::InitCheck()
{
	status_t status = PathCommand::InitCheck();
	if (status < B_OK)
		return status;
	if (!fIndex || !fPoint || !fPointIn || !fPointOut || !fConnected)
		status = B_NO_MEMORY;
	return status;
}

// Perform
status_t
RemovePointsCommand::Perform()
{
	// path points are already removed
	return InitCheck();
}

// Undo
status_t
RemovePointsCommand::Undo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	AutoNotificationSuspender _(fPath);

	// add points again at their respective index
	for (int32 i = 0; i < fCount; i++) {
		if (fPath->AddPoint(fPoint[i], fIndex[i])) {
			fPath->SetPoint(fIndex[i],
							fPoint[i],
							fPointIn[i],
							fPointOut[i],
							fConnected[i]);
		} else {
			status = B_ERROR;
			break;
		}
	}

	fPath->SetClosed(fWasClosed);

	if (status >= B_OK) {
		// select the added points
		_Select(fIndex, fCount);
	}

	return status;
}

// Redo
status_t
RemovePointsCommand::Redo()
{
	status_t status = InitCheck();
	if (status < B_OK)
		return status;

	AutoNotificationSuspender _(fPath);

	// remove points
	// the loop assumes the indices in the collection
	// are increasing (removal at "index[i] - i" to account
	// for items already removed)
	for (int32 i = 0; i < fCount; i++) {
		if (!fPath->RemovePoint(fIndex[i] - i)) {
			status = B_ERROR;
			break;
		}
	}

	fPath->SetClosed(fWasClosed && fPath->CountPoints() > 1);

	if (status >= B_OK) {
		// restore selection
		_Select(fOldSelection, fOldSelectionCount);
	}

	return status;
}

// GetName
void
RemovePointsCommand::GetName(BString& name)
{
//	if (fCount > 1)
//		name << _GetString(REMOVE_CONTROL_POINTS, "Remove Control Points");
//	else
//		name << _GetString(REMOVE_CONTROL_POINT, "Remove Control Point");
	if (fCount > 1)
		name << B_TRANSLATE("Remove Control Points");
	else
		name << B_TRANSLATE("Remove Control Point");
}

// _Init
void
RemovePointsCommand::_Init(const int32* indices, int32 count,
						 const int32* selection, int32 selectionCount)
{
	if (indices && count > 0) {
		fIndex = new (nothrow) int32[count];
		fPoint = new (nothrow) BPoint[count];
		fPointIn = new (nothrow) BPoint[count];
		fPointOut = new (nothrow) BPoint[count];
		fConnected = new (nothrow) bool[count];
		fCount = count;
	}

	if (InitCheck() < B_OK)
		return;

	memcpy(fIndex, indices, count * sizeof(int32));
	for (int32 i = 0; i < count; i++) {
		if (!fPath->GetPointsAt(fIndex[i],
								fPoint[i],
								fPointIn[i],
								fPointOut[i],
								&fConnected[i])) {
			fPath = NULL;
			break;
		}
	}

	if (fPath)
		fWasClosed = fPath->IsClosed();

	if (selectionCount > 0 && selection) {
		fOldSelectionCount = selectionCount;
		fOldSelection = new (nothrow) int32[selectionCount];
		memcpy(fOldSelection, selection, selectionCount * sizeof(int32));
	}
}
