/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "CanvasTransformBox.h"

#include "CanvasView.h"


// constructor
CanvasTransformBox::CanvasTransformBox(CanvasView* view)
	:
	TransformBox(view, BRect(0.0, 0.0, 1.0, 1.0)),

	fCanvasView(view),
	fParentTransform()
{
}


// destructor
CanvasTransformBox::~CanvasTransformBox()
{
}


// TransformFromCanvas
void
CanvasTransformBox::TransformFromCanvas(BPoint& point) const
{
	fParentTransform.InverseTransform(&point);
	fCanvasView->ConvertFromCanvas(&point);
}


// TransformToCanvas
void
CanvasTransformBox::TransformToCanvas(BPoint& point) const
{
	fCanvasView->ConvertToCanvas(&point);
	fParentTransform.Transform(&point);
}


// ZoomLevel
float
CanvasTransformBox::ZoomLevel() const
{
	return fCanvasView->ZoomLevel();
}


// ViewSpaceRotation
double
CanvasTransformBox::ViewSpaceRotation() const
{
	Transformable t(*this);
	t.Multiply(fParentTransform);
	return t.rotation() * 180.0 / M_PI;
}
