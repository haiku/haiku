/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "SplitPointsCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-SplitPointsCmd"


using std::nothrow;

// constructor
// * when hitting the Delete key, so the selected points are the
// same as the ones to be removed
SplitPointsCommand::SplitPointsCommand(VectorPath* path,
									   const int32* indices,
									   int32 count)
	: PathCommand(path),
	  fIndex(NULL),
	  fPoint(NULL),
	  fPointIn(NULL),
	  fPointOut(NULL),
	  fConnected(NULL),
	  fCount(0)
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
}

// destructor
SplitPointsCommand::~SplitPointsCommand()
{
	delete[] fIndex;
	delete[] fPoint;
	delete[] fPointIn;
	delete[] fPointOut;
	delete[] fConnected;
}

// InitCheck
status_t
SplitPointsCommand::InitCheck()
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
SplitPointsCommand::Perform()
{
	status_t status = B_OK;

	AutoNotificationSuspender _(fPath);

	// NOTE: fCount guaranteed > 0
	// add points again at their respective index
	for (int32 i = 0; i < fCount; i++) {
		int32 index = fIndex[i] + 1 + i;
			// "+ 1" to insert behind existing point
			// "+ i" to adjust for already inserted points
		if (fPath->AddPoint(fPoint[i], index)) {
			fPath->SetPoint(index - 1,
							fPoint[i],
							fPointIn[i],
							fPoint[i],
							true);
			fPath->SetPoint(index,
							fPoint[i],
							fPoint[i],
							fPointOut[i],
							true);
		} else {
			status = B_ERROR;
			break;
		}
	}

	return status;
}

// Undo
status_t
SplitPointsCommand::Undo()
{
	status_t status = B_OK;

	AutoNotificationSuspender _(fPath);

	// remove inserted points and reset modified
	// points to previous condition
	for (int32 i = 0; i < fCount; i++) {
		int32 index = fIndex[i] + 1;
		if (fPath->RemovePoint(index)) {
			fPath->SetPoint(index - 1,
							fPoint[i],
							fPointIn[i],
							fPointOut[i],
							fConnected[i]);
		} else {
			status = B_ERROR;
			break;
		}
	}

	if (status >= B_OK) {
		// restore selection
		_Select(fIndex, fCount);
	}

	return status;
}

// GetName
void
SplitPointsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Split Control Points");
	else
		name << B_TRANSLATE("Split Control Point");
}

