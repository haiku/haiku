#include "Shape.h"

#include <new>
#include <limits.h>

#include "Style.h"
#include "VectorPath.h"

using std::nothrow;

// constructor
Shape::Shape(::Style* style)
	: Observable(),
	  Referenceable(),
	  PathContainerListener(),
	  fPaths(new (nothrow) PathContainer()),
	  fStyle(style)
{
	if (fPaths)
		fPaths->AddListener(this);

	if (fStyle)
		fStyle->Acquire();
}

// destructor
Shape::~Shape()
{
	fPaths->MakeEmpty();
	fPaths->RemoveListener(this);
	delete fPaths;

	if (fStyle)
		fStyle->Release();
}

// PathAdded
void
Shape::PathAdded(VectorPath* path)
{
	path->Acquire();
}

// PathRemoved
void
Shape::PathRemoved(VectorPath* path)
{
	path->Release();
}

// #pragma mark -

// InitCheck
status_t
Shape::InitCheck() const
{
	return fPaths ? B_OK : B_NO_MEMORY;
}

// SetStyle
void
Shape::SetStyle(::Style* style)
{
	if (fStyle == style)
		return;

	if (fStyle)
		fStyle->Release();

	fStyle = style;

	if (fStyle)
		fStyle->Acquire();

	Notify();
}

// Bounds
BRect
Shape::Bounds() const
{
	BRect bounds(LONG_MAX, LONG_MAX, LONG_MIN, LONG_MIN);

	int32 count = fPaths->CountPaths();
	for (int32 i = 0; i < count; i++)
		bounds = bounds | fPaths->PathAtFast(i)->Bounds();

	return bounds;
}
