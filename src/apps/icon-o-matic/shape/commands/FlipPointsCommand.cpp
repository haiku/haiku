/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "FlipPointsCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-FlipPointsCmd"


using std::nothrow;

// constructor
FlipPointsCommand::FlipPointsCommand(VectorPath* path,
									 const int32* indices,
									 int32 count)
	: PathCommand(path),
	  fIndex(NULL),
	  fCount(0)
{
	if (indices && count > 0) {
		fIndex = new (nothrow) int32[count];
		fCount = count;
	}

	if (InitCheck() < B_OK)
		return;

	memcpy(fIndex, indices, count * sizeof(int32));
}

// destructor
FlipPointsCommand::~FlipPointsCommand()
{
	delete[] fIndex;
}

// InitCheck
status_t
FlipPointsCommand::InitCheck()
{
	status_t status = PathCommand::InitCheck();
	if (status < B_OK)
		return status;
	if (!fIndex)
		status = B_NO_MEMORY;
	return status;
}

// Perform
status_t
FlipPointsCommand::Perform()
{
	status_t status = B_OK;

	AutoNotificationSuspender _(fPath);

	// NOTE: fCount guaranteed > 0
	for (int32 i = 0; i < fCount; i++) {
		BPoint point;
		BPoint pointIn;
		BPoint pointOut;
		bool connected;

		if (fPath->GetPointsAt(fIndex[i], point, pointIn, pointOut, &connected)) {
BPoint p1(point - (pointOut - point));
printf("flip: (%.1f, %.1f) -> (%.1f, %.1f)\n", pointOut.x, pointOut.y, p1.x, p1.y);
			// flip "in" and "out" control points
			fPath->SetPoint(fIndex[i],
							point,
							point - (pointIn - point),
							point - (pointOut - point),
							connected);
		} else {
			status = B_ERROR;
			break;
		}
	}

	return status;
}

// Undo
status_t
FlipPointsCommand::Undo()
{
	status_t status = Perform();

	if (status >= B_OK) {
		// restore selection
		_Select(fIndex, fCount);
	}

	return status;
}

// GetName
void
FlipPointsCommand::GetName(BString& name)
{
	if (fCount > 1)
		name << B_TRANSLATE("Flip Control Points");
	else
		name << B_TRANSLATE("Flip Control Point");
}

