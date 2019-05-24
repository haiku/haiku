/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformPointsCommand.h"

#include <new>
#include <stdio.h>

#include "ChannelTransform.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
TransformPointsCommand::TransformPointsCommand(
								TransformBox* box,

								VectorPath* path,
								const int32* indices,
								const control_point* points,
								int32 count,

								BPoint pivot,
								BPoint translation,
								double rotation,
								double xScale,
								double yScale,

								const char* name,
								int32 nameIndex)
	: TransformCommand(pivot,
					   translation,
					   rotation,
					   xScale,
					   yScale,
					   name,
					   nameIndex),
	  fTransformBox(box),
	  fPath(path),
	  fIndices(indices && count > 0 ?
	  		   new (nothrow) int32[count] : NULL),
	  fPoints(points && count > 0 ?
	  			 new (nothrow) control_point[count] : NULL),
	  fCount(count)
{
	if (!fIndices || !fPoints)
		return;

	memcpy(fIndices, indices, fCount * sizeof(int32));
	memcpy((void*)fPoints, points, fCount * sizeof(control_point));

	if (fTransformBox)
		fTransformBox->AddListener(this);
}

// destructor
TransformPointsCommand::~TransformPointsCommand()
{
	if (fTransformBox)
		fTransformBox->RemoveListener(this);

	delete[] fIndices;
	delete[] fPoints;
}

// InitCheck
status_t
TransformPointsCommand::InitCheck()
{
	return fPath && fIndices && fPoints ? TransformCommand::InitCheck()
										: B_NO_INIT;
}

// #pragma mark -

// TransformBoxDeleted
void
TransformPointsCommand::TransformBoxDeleted(
									const TransformBox* box)
{
	if (fTransformBox == box) {
		if (fTransformBox)
			fTransformBox->RemoveListener(this);
		fTransformBox = NULL;
	}
}

// #pragma mark -

// _SetTransformation
status_t
TransformPointsCommand::_SetTransformation(
								BPoint pivot, BPoint translation,
								double rotation,
								double xScale, double yScale) const
{
	if (fTransformBox) {
		fTransformBox->SetTransformation(pivot, translation,
										 rotation, xScale, yScale);
		return B_OK;
	}


	ChannelTransform transform;
	transform.SetTransformation(pivot, translation,
								rotation, xScale, yScale);
	// restore original points and apply transformation
	for (int32 i = 0; i < fCount; i++) {
		BPoint point = transform.Transform(fPoints[i].point);
		BPoint pointIn = transform.Transform(fPoints[i].point_in);
		BPoint pointOut = transform.Transform(fPoints[i].point_out);
		if (!fPath->SetPoint(fIndices[i], point, pointIn, pointOut,
							 fPoints[i].connected))
			return B_ERROR;
	}

	return B_OK;
}

