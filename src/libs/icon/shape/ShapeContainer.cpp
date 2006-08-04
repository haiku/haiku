/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "ShapeContainer.h"

#include <stdio.h>
#include <string.h>

#include <OS.h>

#include "Shape.h"

#ifdef ICON_O_MATIC
// constructor
ShapeContainerListener::ShapeContainerListener()
{
}

// destructor
ShapeContainerListener::~ShapeContainerListener()
{
}
#endif // ICON_O_MATIC



// constructor
ShapeContainer::ShapeContainer()
	: fShapes(16)
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
}

// destructor
ShapeContainer::~ShapeContainer()
{
#ifdef ICON_O_MATIC
	int32 count = fListeners.CountItems();
	if (count > 0) {
		debugger("~ShapeContainer() - there are still"
				 "listeners attached\n");
	}
#endif // ICON_O_MATIC
	_MakeEmpty();
}

// #pragma mark -

// AddShape
bool
ShapeContainer::AddShape(Shape* shape)
{
	return AddShape(shape, CountShapes());
}

// AddShape
bool
ShapeContainer::AddShape(Shape* shape, int32 index)
{
	if (!shape)
		return false;

	// prevent adding the same shape twice
	if (HasShape(shape))
		return false;

	if (fShapes.AddItem((void*)shape, index)) {
#ifdef ICON_O_MATIC
		_NotifyShapeAdded(shape, index);
#endif
		return true;
	}

	fprintf(stderr, "ShapeContainer::AddShape() - out of memory!\n");
	return false;
}

// RemoveShape
bool
ShapeContainer::RemoveShape(Shape* shape)
{
	if (fShapes.RemoveItem((void*)shape)) {
#ifdef ICON_O_MATIC
		_NotifyShapeRemoved(shape);
#endif
		return true;
	}

	return false;
}

// RemoveShape
Shape*
ShapeContainer::RemoveShape(int32 index)
{
	Shape* shape = (Shape*)fShapes.RemoveItem(index);
#ifdef ICON_O_MATIC
	if (shape) {
		_NotifyShapeRemoved(shape);
	}
#endif

	return shape;
}

// MakeEmpty
void
ShapeContainer::MakeEmpty()
{
	_MakeEmpty();
}

// #pragma mark -

// CountShapes
int32
ShapeContainer::CountShapes() const
{
	return fShapes.CountItems();
}

// HasShape
bool
ShapeContainer::HasShape(Shape* shape) const
{
	return fShapes.HasItem((void*)shape);
}

// IndexOf
int32
ShapeContainer::IndexOf(Shape* shape) const
{
	return fShapes.IndexOf((void*)shape);
}

// ShapeAt
Shape*
ShapeContainer::ShapeAt(int32 index) const
{
	return (Shape*)fShapes.ItemAt(index);
}

// ShapeAtFast
Shape*
ShapeContainer::ShapeAtFast(int32 index) const
{
	return (Shape*)fShapes.ItemAtFast(index);
}

// #pragma mark -

#ifdef ICON_O_MATIC

// AddListener
bool
ShapeContainer::AddListener(ShapeContainerListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}

// RemoveListener
bool
ShapeContainer::RemoveListener(ShapeContainerListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}

#endif // ICON_O_MATIC

// #pragma mark -

// _MakeEmpty
void
ShapeContainer::_MakeEmpty()
{
	int32 count = CountShapes();
	for (int32 i = 0; i < count; i++) {
		Shape* shape = ShapeAtFast(i);
#ifdef ICON_O_MATIC
		_NotifyShapeRemoved(shape);
		shape->Release();
#else
		delete shape;
#endif
	}
	fShapes.MakeEmpty();
}

// #pragma mark -

#ifdef ICON_O_MATIC

// _NotifyShapeAdded
void
ShapeContainer::_NotifyShapeAdded(Shape* shape, int32 index) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeContainerListener* listener
			= (ShapeContainerListener*)listeners.ItemAtFast(i);
		listener->ShapeAdded(shape, index);
	}
}

// _NotifyShapeRemoved
void
ShapeContainer::_NotifyShapeRemoved(Shape* shape) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		ShapeContainerListener* listener
			= (ShapeContainerListener*)listeners.ItemAtFast(i);
		listener->ShapeRemoved(shape);
	}
}

#endif // ICON_O_MATIC
