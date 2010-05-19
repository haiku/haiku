/*
 * Copyright 2006-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformGradientBox.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "CanvasView.h"
#include "GradientTransformable.h"
#include "Shape.h"
#include "StateView.h"
#include "TransformGradientCommand.h"

using std::nothrow;


// constructor
TransformGradientBox::TransformGradientBox(CanvasView* view, Gradient* gradient,
		Shape* parentShape)
	:
	TransformBox(view, BRect(0.0, 0.0, 1.0, 1.0)),

	fCanvasView(view),

	fShape(parentShape),
	fGradient(gradient)
{
	if (fShape) {
		fShape->Acquire();
		fShape->AddObserver(this);
	}
	if (fGradient) {
		// trigger init
		ObjectChanged(fGradient);
	} else {
		SetBox(BRect(0, 0, -1, -1));
	}
}


// destructor
TransformGradientBox::~TransformGradientBox()
{
	if (fShape) {
		fShape->RemoveObserver(this);
		fShape->Release();
	}
	if (fGradient)
		fGradient->RemoveObserver(this);
}


// Update
void
TransformGradientBox::Update(bool deep)
{
	BRect r = Bounds();

	TransformBox::Update(deep);

	BRect dirty(r | Bounds());
	dirty.InsetBy(-8, -8);
	fView->Invalidate(dirty);

	if (!deep || !fGradient)
		return;

	fGradient->RemoveObserver(this);
	fGradient->SuspendNotifications(true);

	// reset the objects transformation to the saved state
	fGradient->Reset();
	// combine with the current transformation
	fGradient->Multiply(*this);

//printf("matrix:\n");
//double m[6];
//StoreTo(m);
//printf("[%5.10f] [%5.10f] [%5.10f]\n", m[0], m[1], m[2]);
//printf("[%5.10f] [%5.10f] [%5.10f]\n", m[3], m[4], m[5]);
//
	fGradient->SuspendNotifications(false);
	fGradient->AddObserver(this);
}


// ObjectChanged
void
TransformGradientBox::ObjectChanged(const Observable* object)
{
	if (!fGradient || !fView->LockLooper())
		return;

	if (object == fShape) {
		fView->Invalidate(Bounds());
		fView->UnlockLooper();
		return;
	}

	// any TransformGradientCommand cannot use the TransformBox
	// anymore
	_NotifyDeleted();

	fGradient->StoreTo(fOriginals);

	// figure out bounds and store initial transformations
	SetTransformation(*fGradient);
	SetBox(fGradient->GradientArea());

	fView->UnlockLooper();
}


// Perform
Command*
TransformGradientBox::Perform()
{
	return NULL;
}


// Cancel
Command*
TransformGradientBox::Cancel()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);

	return NULL;
}


// TransformFromCanvas
void
TransformGradientBox::TransformFromCanvas(BPoint& point) const
{
	if (fShape)
		fShape->InverseTransform(&point);
	fCanvasView->ConvertFromCanvas(&point);
}


// TransformToCanvas
void
TransformGradientBox::TransformToCanvas(BPoint& point) const
{
	fCanvasView->ConvertToCanvas(&point);
	if (fShape)
		fShape->Transform(&point);
}


// ZoomLevel
float
TransformGradientBox::ZoomLevel() const
{
	return fCanvasView->ZoomLevel();
}


// ViewSpaceRotation
double
TransformGradientBox::ViewSpaceRotation() const
{
	Transformable t(*this);
	if (fShape)
		t.Multiply(*fShape);
	return t.rotation() * 180.0 / M_PI;
}


// MakeCommand
TransformCommand*
TransformGradientBox::MakeCommand(const char* commandName, uint32 nameIndex)
{
	return new TransformGradientCommand(this, fGradient, Pivot(),
	   Translation(), LocalRotation(), LocalXScale(), LocalYScale(),
	   commandName, nameIndex);
}

