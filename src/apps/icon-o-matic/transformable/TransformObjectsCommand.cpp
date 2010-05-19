/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformObjectsCommand.h"

#include <new>
#include <stdio.h>

#include "ChannelTransform.h"

using std::nothrow;

// constructor
TransformObjectsCommand::TransformObjectsCommand(
								TransformBox* box,
								Transformable** const objects,
								const double* originals,
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
	  fObjects(objects && count > 0 ?
	  		   new (nothrow) Transformable*[count] : NULL),
	  fOriginals(originals && count > 0 ?
	  			 new (nothrow) double[
	  			 	count * Transformable::matrix_size] : NULL),
	  fCount(count)
{
	if (!fObjects || !fOriginals)
		return;

	memcpy(fObjects, objects, fCount * sizeof(Transformable*));
	memcpy(fOriginals, originals,
		   fCount * Transformable::matrix_size * sizeof(double));

	if (fTransformBox)
		fTransformBox->AddListener(this);
}

// destructor
TransformObjectsCommand::~TransformObjectsCommand()
{
	if (fTransformBox)
		fTransformBox->RemoveListener(this);

	delete[] fObjects;
	delete[] fOriginals;
}

// InitCheck
status_t
TransformObjectsCommand::InitCheck()
{
	return fObjects && fOriginals ? TransformCommand::InitCheck()
								  : B_NO_INIT;
}

// #pragma mark -

// TransformBoxDeleted
void
TransformObjectsCommand::TransformBoxDeleted(
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
TransformObjectsCommand::_SetTransformation(
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
	// restore original transformations
	int32 matrixSize = Transformable::matrix_size;
	for (int32 i = 0; i < fCount; i++) {
		if (fObjects[i]) {
			fObjects[i]->LoadFrom(&fOriginals[i * matrixSize]);
			fObjects[i]->Multiply(transform);
		}
	}
	return B_OK;
}

