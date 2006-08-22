/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "Icon.h"

#include <new>
#include <stdio.h>

#include "PathContainer.h"
#include "Shape.h"
#include "Style.h"
#include "StyleContainer.h"

using std::nothrow;

#ifdef ICON_O_MATIC
IconListener::IconListener() {}
IconListener::~IconListener() {}
#endif

// #pragma mark -

// constructor
Icon::Icon()
	: fStyles(new (nothrow) StyleContainer()),
	  fPaths(new (nothrow) PathContainer(true)),
	  fShapes(new (nothrow) ShapeContainer())
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
#ifdef ICON_O_MATIC
	if (fShapes)
		fShapes->AddListener(this);
#endif
}

// constructor
Icon::Icon(const Icon& other)
	: fStyles(new (nothrow) StyleContainer()),
	  fPaths(new (nothrow) PathContainer(true)),
	  fShapes(new (nothrow) ShapeContainer())
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
	if (!fStyles || !fPaths || !fShapes)
		return;

#ifdef ICON_O_MATIC
	fShapes->AddListener(this);
#endif

	int32 styleCount = other.fStyles->CountStyles();
	for (int32 i = 0; i < styleCount; i++) {
		Style* style = other.fStyles->StyleAtFast(i);
		Style* clone = new (nothrow) Style(*style);
		if (!clone || !fStyles->AddStyle(clone)) {
			delete clone;
			return;
		}
	}

	int32 pathCount = other.fPaths->CountPaths();
	for (int32 i = 0; i < pathCount; i++) {
		VectorPath* path = other.fPaths->PathAtFast(i);
		VectorPath* clone = new (nothrow) VectorPath(*path);
		if (!clone || !fPaths->AddPath(clone)) {
			delete clone;
			return;
		}
	}

	int32 shapeCount = other.fShapes->CountShapes();
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = other.fShapes->ShapeAtFast(i);
		Shape* clone = new (nothrow) Shape(*shape);
		if (!clone || !fShapes->AddShape(clone)) {
			delete clone;
			return;
		}
		// the cloned shape references styles and paths in
		// the "other" icon, replace them with "local" styles
		// and paths
		int32 styleIndex = other.fStyles->IndexOf(shape->Style());
		clone->SetStyle(fStyles->StyleAt(styleIndex));

		clone->Paths()->MakeEmpty();
		pathCount = shape->Paths()->CountPaths();
		for (int32 j = 0; j < pathCount; j++) {
			VectorPath* remote = shape->Paths()->PathAtFast(j);
			int32 index = other.fPaths->IndexOf(remote);
			VectorPath* local = fPaths->PathAt(index);
			if (!local) {
				printf("failed to match remote and "
					   "local paths while cloning icon\n");
				continue;
			}
			if (!clone->Paths()->AddPath(local)) {
				return;
			}
		}
	}
}

// destructor
Icon::~Icon()
{
	if (fShapes) {
		fShapes->MakeEmpty();
#ifdef ICON_O_MATIC
		fShapes->RemoveListener(this);
#endif
		delete fShapes;
	}
	delete fPaths;
	delete fStyles;
}

// InitCheck
status_t
Icon::InitCheck() const
{
	return fStyles && fPaths && fShapes ? B_OK : B_NO_MEMORY;
}

#ifdef ICON_O_MATIC
// ShapeAdded
void
Icon::ShapeAdded(Shape* shape, int32 index)
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
	if (listener && !fListeners.HasItem((void*)listener)) {
		if (fListeners.AddItem((void*)listener)) {
			listener->AreaInvalidated(BRect(0, 0, 63, 63));
			return true;
		}
	}
	return false;
}

// RemoveListener
bool
Icon::RemoveListener(IconListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}
#endif // ICON_O_MATIC


// Clone
Icon*
Icon::Clone() const
{
	return new (nothrow) Icon(*this);
}

// MakeEmpty
void
Icon::MakeEmpty()
{
	fShapes->MakeEmpty();
	fPaths->MakeEmpty();
	fStyles->MakeEmpty();
}

// #pragma mark -

#ifdef ICON_O_MATIC
// _NotifyAreaInvalidated
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
#endif // ICON_O_MATIC


