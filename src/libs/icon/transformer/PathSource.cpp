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
	: VertexSource()
	, fPaths(paths)
	, fAGGPath()
	, fAGGCurvedPath(fAGGPath)

	, fGlobalScale(1.0)
	, fLastTransformerScale(1.0)
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

// WantsOpenPaths
bool
PathSource::WantsOpenPaths() const
{
	return false;
}

// ApproximationScale
double
PathSource::ApproximationScale() const
{
	return 1.0;
}

// #pragma mark -

// Update
void
PathSource::Update(bool leavePathsOpen, double approximationScale)
{
	fAGGPath.remove_all();

	int32 count = fPaths->CountPaths();
	for (int32 i = 0; i < count; i++) {
		fPaths->PathAtFast(i)->GetAGGPathStorage(fAGGPath);
		if (!leavePathsOpen)
			fAGGPath.close_polygon();
	}

	fLastTransformerScale = approximationScale;
	fAGGCurvedPath.approximation_scale(fLastTransformerScale * fGlobalScale);
}

// SetGlobalScale
void
PathSource::SetGlobalScale(double scale)
{
	fGlobalScale = scale;
	fAGGCurvedPath.approximation_scale(fLastTransformerScale * fGlobalScale);
}

