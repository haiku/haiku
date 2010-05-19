/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */


#include "TransformGradientCommand.h"

#include <new>
#include <stdio.h>

#include "GradientTransformable.h"


using std::nothrow;


TransformGradientCommand::TransformGradientCommand(TransformBox* box,
		Gradient* gradient, BPoint pivot, BPoint translation, double rotation,
		double xScale, double yScale, const char* name, int32 nameIndex)
	:
	TransformCommand(pivot, translation, rotation, xScale, yScale, name,
		nameIndex),
	fTransformBox(box),
	fGradient(gradient)
{
	if (fGradient == NULL)
		return;

//	fGradient->Acquire();

	if (fTransformBox != NULL)
		fTransformBox->AddListener(this);
}


TransformGradientCommand::~TransformGradientCommand()
{
//	if (fGradient != NULL)
//		fGradient->Release();

	if (fTransformBox != NULL)
		fTransformBox->RemoveListener(this);
}


status_t
TransformGradientCommand::InitCheck()
{
	return fGradient != NULL ? TransformCommand::InitCheck() : B_NO_INIT;
}

// #pragma mark -

// TransformBoxDeleted
void
TransformGradientCommand::TransformBoxDeleted(const TransformBox* box)
{
	if (fTransformBox == box) {
		if (fTransformBox != NULL)
			fTransformBox->RemoveListener(this);
		fTransformBox = NULL;
	}
}


// #pragma mark -


status_t
TransformGradientCommand::_SetTransformation(BPoint pivot, BPoint translation,
	double rotation, double xScale, double yScale) const
{
	if (fTransformBox) {
		fTransformBox->SetTransformation(pivot, translation, rotation, xScale,
			yScale);
		return B_OK;
	}

	ChannelTransform transform;
	transform.SetTransformation(pivot, translation, rotation, xScale, yScale);

	// Reset and apply transformation. (Gradients never have an original
	// transformation that needs to be taken into account, the box always
	// assignes it completely.)
	fGradient->Reset();
	fGradient->Multiply(transform);

	return B_OK;
}

