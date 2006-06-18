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
#include "ShapeContainer.h"

using std::nothrow;

// constructor
Icon::Icon()
	: fPaths(new (nothrow) PathContainer()),
	  fShapes(new (nothrow) ShapeContainer())
{
}


// destructor
Icon::~Icon()
{
	delete fShapes;
	delete fPaths;
}
