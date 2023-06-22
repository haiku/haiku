/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#include "Icon.h"

#include <new>
#include <stdio.h>

#include "PathSourceShape.h"
#include "ReferenceImage.h"
#include "Shape.h"
#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

#ifdef ICON_O_MATIC
IconListener::IconListener() {}
IconListener::~IconListener() {}
#endif

// #pragma mark -


Icon::Icon()
	: fStyles(true),
	  fPaths(true),
	  fShapes(true)
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
#ifdef ICON_O_MATIC
	fShapes.AddListener(this);
#endif
}


Icon::Icon(const Icon& other)
	: fStyles(true),
	  fPaths(true),
	  fShapes(true)
#ifdef ICON_O_MATIC
	, fListeners(2)
#endif
{
#ifdef ICON_O_MATIC
	fShapes.AddListener(this);
#endif

	int32 styleCount = other.fStyles.CountItems();
	for (int32 i = 0; i < styleCount; i++) {
		Style* style = other.fStyles.ItemAtFast(i);
		Style* clone = new (nothrow) Style(*style);
		if (!clone || !fStyles.AddItem(clone)) {
			delete clone;
			return;
		}
	}

	int32 pathCount = other.fPaths.CountItems();
	for (int32 i = 0; i < pathCount; i++) {
		VectorPath* path = other.fPaths.ItemAtFast(i);
		VectorPath* clone = new (nothrow) VectorPath(*path);
		if (!clone || !fPaths.AddItem(clone)) {
			delete clone;
			return;
		}
	}

	int32 shapeCount = other.fShapes.CountItems();
	for (int32 i = 0; i < shapeCount; i++) {
		Shape* shape = other.fShapes.ItemAtFast(i);
		Shape* clone = shape->Clone();
		if (!clone || !fShapes.AddItem(clone)) {
			delete clone;
			return;
		}

		// PathSourceShapes require further handling
		PathSourceShape* pathSourceShape = dynamic_cast<PathSourceShape*>(shape);
		PathSourceShape* pathSourceShapeClone = dynamic_cast<PathSourceShape*>(clone);
		if (pathSourceShape != NULL && pathSourceShapeClone != NULL) {
			// the cloned shape references styles and paths in
			// the "other" icon, replace them with "local" styles
			// and paths

			int32 styleIndex = other.fStyles.IndexOf(pathSourceShape->Style());
			pathSourceShapeClone->SetStyle(fStyles.ItemAt(styleIndex));

			pathSourceShapeClone->Paths()->MakeEmpty();
			pathCount = pathSourceShape->Paths()->CountItems();
			for (int32 j = 0; j < pathCount; j++) {
				VectorPath* remote = pathSourceShape->Paths()->ItemAtFast(j);
				int32 index = other.fPaths.IndexOf(remote);
				VectorPath* local = fPaths.ItemAt(index);
				if (!local) {
					printf("failed to match remote and "
						   "local paths while cloning icon\n");
					continue;
				}
				if (!pathSourceShapeClone->Paths()->AddItem(local)) {
					return;
				}
			}
		}
	}
}


Icon::~Icon()
{
	fShapes.MakeEmpty();
#ifdef ICON_O_MATIC
	fShapes.RemoveListener(this);
#endif
}


status_t
Icon::InitCheck() const
{
	return B_OK;
}

#ifdef ICON_O_MATIC

void
Icon::ItemAdded(Shape* shape, int32 index)
{
	shape->AddObserver(this);
	_NotifyAreaInvalidated(shape->Bounds(true));
}


void
Icon::ItemRemoved(Shape* shape)
{
	shape->RemoveObserver(this);
	_NotifyAreaInvalidated(shape->Bounds(true));
}


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


bool
Icon::RemoveListener(IconListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}
#endif // ICON_O_MATIC


Icon*
Icon::Clone() const
{
	return new (nothrow) Icon(*this);
}


void
Icon::MakeEmpty()
{
	fShapes.MakeEmpty();
	fPaths.MakeEmpty();
	fStyles.MakeEmpty();
}

// #pragma mark -


#ifdef ICON_O_MATIC
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
