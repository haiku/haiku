#include "Shape.h"

#include "Style.h"
#include "VectorPath.h"

// constructor
Shape::Shape(VectorPath* path, ::Style* style)
	: fPath(path),
	  fStyle(style)
{
	if (fPath)
		fPath->Acquire();
	if (fStyle)
		fStyle->Acquire();
}

// destructor
Shape::~Shape()
{
	if (fPath)
		fPath->Release();
	if (fStyle)
		fStyle->Release();
}

// SetPath
void
Shape::SetPath(VectorPath* path)
{
	if (fPath == path)
		return;

	if (fPath)
		fPath->Release();

	fPath = path;

	if (fPath)
		fPath->Acquire();

	Notify();
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
	if (fPath)
		return fPath->Bounds();
	return BRect(0, 0, -1, -1);
}
