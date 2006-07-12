/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "TransformShapesBox.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "CanvasView.h"
#include "Shape.h"
#include "StateView.h"
#include "TransformObjectsCommand.h"

using std::nothrow;

// constructor
TransformShapesBox::TransformShapesBox(CanvasView* view,
									   const Shape** shapes,
									   int32 count)
	: TransformBox(view, BRect(0.0, 0.0, 1.0, 1.0)),

	  fCanvasView(view),

	  fShapes(shapes && count > 0 ? new Shape*[count] : NULL),
	  fCount(count),

	  fOriginals(NULL),

	  fParentTransform()
{
	if (fShapes) {
		// allocate storage for the current transformations
		// of each object
		fOriginals = new double[fCount * 6];

		memcpy(fShapes, shapes, fCount * sizeof(Shape*));

		for (int32 i = 0; i < fCount; i++) {
			if (fShapes[i])
				fShapes[i]->AddObserver(this);
		}

		// trigger init
		ObjectChanged(fShapes[0]);
	} else {
		SetBox(BRect(0, 0, -1, -1));
	}
}

// destructor
TransformShapesBox::~TransformShapesBox()
{
	if (fShapes) {
		for (int32 i = 0; i < fCount; i++) {
			if (fShapes[i])
				fShapes[i]->RemoveObserver(this);
		}
		delete[] fShapes;
	}

	delete[] fOriginals;
}

// Update
void
TransformShapesBox::Update(bool deep)
{
	BRect r = Bounds();

	TransformBox::Update(deep);

	BRect dirty(r | Bounds());
	dirty.InsetBy(-8, -8);
	fView->Invalidate(dirty);

	if (!deep || !fShapes)
		return;

	for (int32 i = 0; i < fCount; i++) {
		if (!fShapes[i])
			continue;

		fShapes[i]->RemoveObserver(this);
		fShapes[i]->SuspendNotifications(true);

		// reset the objects transformation to the saved state
		fShapes[i]->LoadFrom(&fOriginals[i * 6]);
		// combined with the current transformation
		fShapes[i]->Multiply(*this);

		fShapes[i]->SuspendNotifications(false);
		fShapes[i]->AddObserver(this);
	}
}

// ObjectChanged
void
TransformShapesBox::ObjectChanged(const Observable* object)
{
	if (!fView->LockLooper())
		return;

	fParentTransform.Reset();
	// figure out bounds and store initial transformations
	BRect box(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);
	for (int32 i = 0; i < fCount; i++) {
		if (!fShapes[i])
			continue;

		box = box | fShapes[i]->Bounds();
		fShapes[i]->StoreTo(&fOriginals[i * 6]);
	}
	// any TransformObjectsCommand cannot use the TransformBox
	// anymore
	_NotifyDeleted();

	Reset();
	SetBox(box);

	fView->UnlockLooper();
}

// Perform
Command*
TransformShapesBox::Perform()
{
	return NULL;
}

// Cancel
Command*
TransformShapesBox::Cancel()
{
	SetTransformation(B_ORIGIN, B_ORIGIN, 0.0, 1.0, 1.0);

	return NULL;
}

// TransformFromCanvas
void
TransformShapesBox::TransformFromCanvas(BPoint& point) const
{
	fParentTransform.InverseTransform(&point);
	fCanvasView->ConvertFromCanvas(&point);
}

// TransformToCanvas
void
TransformShapesBox::TransformToCanvas(BPoint& point) const
{
	fCanvasView->ConvertToCanvas(&point);
	fParentTransform.Transform(&point);
}

// ZoomLevel
float
TransformShapesBox::ZoomLevel() const
{
	return fCanvasView->ZoomLevel();
}

// ViewSpaceRotation
double
TransformShapesBox::ViewSpaceRotation() const
{
	Transformable t(*this);
	t.Multiply(fParentTransform);
	return t.rotation() * 180.0 / PI;
}

// MakeCommand
TransformCommand*
TransformShapesBox::MakeCommand(const char* commandName, uint32 nameIndex)
{
	const Transformable* objects[fCount];
	for (int32 i = 0; i < fCount; i++)
		objects[i] = fShapes[i];
	
	return new TransformObjectsCommand(this, objects, fOriginals, fCount,

									   Pivot(),
									   Translation(),
									   LocalRotation(),
									   LocalXScale(),
									   LocalYScale(),

									   commandName,
									   nameIndex);
}

