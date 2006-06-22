/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Icon.h"

#include <new>

#include "PathContainer.h"
#include "Shape.h"

using std::nothrow;

// constructor
IconListener::IconListener()
{
}

// destructor
IconListener::~IconListener()
{
}

// #pragma mark -

// constructor
Icon::Icon()
	: fPaths(new (nothrow) PathContainer(true)),
	  fShapes(new (nothrow) ShapeContainer()),
	  fListeners(2)
{
	if (fShapes)
		fShapes->AddListener(this);
}

// destructor
Icon::~Icon()
{
	if (fShapes) {
		fShapes->MakeEmpty();
		fShapes->RemoveListener(this);
		delete fShapes;
	}
	delete fPaths;
}

// ShapeAdded
void
Icon::ShapeAdded(Shape* shape)
{
	shape->AddObserver(this);
	_NotifyAreaInvalidated(shape->Bounds(true));
}

// ShapeRemoved
void
Icon::ShapeRemoved(Shape* shape)
{
	shape->RemoveObserver(this);
	_NotifyAreaInvalidated(shape->Bounds(true));
}

// ObjectChanged
void
Icon::ObjectChanged(const Observable* object)
{
	const Shape* shape = dynamic_cast<const Shape*>(object);
	if (shape) {
		BRect area = shape->LastBounds();
		area = area | shape->Bounds(true);
		area.InsetBy(-1, -1);
		_NotifyAreaInvalidated(area);
	}
}

// AddListener
bool
Icon::AddListener(IconListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}

// RemoveListener
bool
Icon::RemoveListener(IconListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}

// #pragma mark -

// _NotifyShapeRemoved
void
Icon::_NotifyAreaInvalidated(const BRect& area) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		IconListener* listener
			= (IconListener*)listeners.ItemAtFast(i);
		listener->AreaInvalidated(area);
	}
}


