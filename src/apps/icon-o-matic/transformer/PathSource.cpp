/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathSource.h"

#include "PathContainer.h"
#include "VectorPath.h"

// constructor
PathSource::PathSource(PathContainer* paths)
	: VertexSource(),
	  fPaths(paths),
	  fAGGPath(),
	  fAGGCurvedPath(fAGGPath)
{
}

// destructor
PathSource::~PathSource()
{
}

// rewind
void
PathSource::rewind(unsigned path_id)
{
	fAGGCurvedPath.rewind(path_id);
}

// vertex
unsigned
PathSource::vertex(double* x, double* y)
{
	return fAGGCurvedPath.vertex(x, y);
}

// SetLast
void
PathSource::SetLast()
{
	fAGGPath.close_polygon();
}

// Update
void
PathSource::Update()
{
	fAGGPath.remove_all();

	int32 count = fPaths->CountPaths();
	for (int32 i = 0; i < count; i++)
		fPaths->PathAtFast(i)->GetAGGPathStorage(fAGGPath);
}

