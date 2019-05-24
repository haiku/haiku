/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "NudgePointsCommand.h"

#include <new>
#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>

#include "VectorPath.h"


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Icon-O-Matic-NudgePointsCommand"


using std::nothrow;

// constructor
NudgePointsCommand::NudgePointsCommand(VectorPath* path,
									   const int32* indices,
									   const control_point* points,
									   int32 count)
	: TransformCommand(B_ORIGIN,
					   B_ORIGIN,
					   0.0,
					   1.0,
					   1.0,
					   count > 1 ? B_TRANSLATE("Nudge Control Points") :
					   B_TRANSLATE("Nudge Control Point"),
//					   count > 1 ? NUDGE_CONTROL_POINTS : NUDGE_CONTROL_POINT),
					   -1),
	  fPath(path),
	  fIndices(NULL),
	  fPoints(NULL),
	  fCount(count)
{
	if (fCount > 0 && indices) {
		fIndices = new (nothrow) int32[fCount];
		memcpy(fIndices, indices, fCount * sizeof(int32));
	}
	if (fCount > 0 && points) {
		fPoints = new (nothrow) control_point[fCount];
		memcpy((void*)fPoints, points, fCount * sizeof(control_point));
	}
}

// destructor
NudgePointsCommand::~NudgePointsCommand()
{
	delete[] fIndices;
	delete[] fPoints;
}

// InitCheck
status_t
NudgePointsCommand::InitCheck()
{
	if (fPath && fIndices && fPoints)
		return TransformCommand::InitCheck();
	else
		return B_NO_INIT;
}

// _SetTransformation
status_t
NudgePointsCommand::_SetTransformation(BPoint pivot,
									   BPoint translation,
									   double rotation,
									   double xScale,
									   double yScale) const
{
	if (!fPath)
		return B_NO_INIT;

	AutoNotificationSuspender _(fPath);

	// restore original path
	for (int32 i = 0; i < fCount; i++) {
		fPath->SetPoint(fIndices[i], fPoints[i].point + translation,
									 fPoints[i].point_in + translation,
									 fPoints[i].point_out + translation,
									 fPoints[i].connected);
	}

	return B_OK;
}

