/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformPointsBox.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "StateView.h"
#include "TransformPointsCommand.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
TransformPointsBox::TransformPointsBox(CanvasView* view,
									   PathManipulator* manipulator,
									   VectorPath* path,
									   const int32* indices,
									   int32 count)
	: CanvasTransformBox(view),
	  fManipulator(manipulator),
	  fPath(path),
	  fIndices(path && count > 0 ? new (nothrow) int32[count] : NULL),
	  fCount(count),
	  fPoints(count > 0 ? new (nothrow) control_point[count] : NULL)
{
	fPath->AcquireReference();

	BRect bounds(0, 0, -1, -1);

	if (fPoints && fIndices) {
		// copy indices
		memcpy(fIndices, indices, fCount * sizeof(int32));
		// make a copy of the points as they are and calculate bounds
		for (int32 i = 0; i < fCount; i++) {
			if (fPath->GetPointsAt(fIndices[i], fPoints[i].point,
												fPoints[i].point_in,
												fPoints[i].point_out,
												&fPoints[i].connected)) {
				BRect dummy(fPoints[i].point, fPoints[i].point);
				if (i == 0) {
					bounds = dummy;
				} else {
					bounds = bounds | dummy;
				}
				dummy.Set(fPoints[i].point_in.x, fPoints[i].point_in.y,
						  fPoints[i].point_in.x, fPoints[i].point_in.y);
				bounds = bounds | dummy;
				dummy.Set(fPoints[i].point_out.x, fPoints[i].point_out.y,
						  fPoints[i].point_out.x, fPoints[i].point_out.y);
				bounds = bounds | dummy;
			} else {
				memset((void*)&fPoints[i], 0, sizeof(control_point));
			}
		}
	}

	SetBox(bounds);
}

// destructor
TransformPointsBox::~TransformPointsBox()
{
	delete[] fIndices;
	delete[] fPoints;
	fPath->ReleaseReference();
}

// #pragma mark -

// ObjectChanged
void
TransformPointsBox::ObjectChanged(const Observable* object)
{
}

// #pragma mark -

// Update
void
TransformPointsBox::Update(bool deep)
{
	BRect r = Bounds();

	TransformBox::Update(deep);

	BRect dirty(r | Bounds());
	dirty.InsetBy(-8, -8);
	fView->Invalidate(dirty);

	if (!deep || !fIndices || !fPoints)
		return;

	for (int32 i = 0; i < fCount; i++) {

		BPoint transformed = fPoints[i].point;
		BPoint transformedIn = fPoints[i].point_in;
		BPoint transformedOut = fPoints[i].point_out;

		Transform(&transformed);
		Transform(&transformedIn);
		Transform(&transformedOut);

		fPath->SetPoint(fIndices[i], transformed,
									 transformedIn,
									 transformedOut,
									 fPoints[i].connected);
	}
}

// MakeCommand
TransformCommand*
TransformPointsBox::MakeCommand(const char* commandName,
								uint32 nameIndex)
{
	return new TransformPointsCommand(this, fPath,

									  fIndices,
									  fPoints,
									  fCount,

									  Pivot(),
									  Translation(),
									  LocalRotation(),
									  LocalXScale(),
									  LocalYScale(),

									  commandName,
									  nameIndex);
}

// #pragma mark -

// Cancel
void
TransformPointsBox::Cancel()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);
}

