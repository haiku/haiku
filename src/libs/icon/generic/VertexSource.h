/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef VERTEX_SOURCE_H
#define VERTEX_SOURCE_H


#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


class VertexSource {
 public:
								VertexSource() {}
	virtual						~VertexSource() {}

	virtual	void				rewind(unsigned path_id) = 0;
	virtual	unsigned			vertex(double* x, double* y) = 0;

	/*! Determines whether open paths should be closed or left open. */
	virtual	bool				WantsOpenPaths() const = 0;
	virtual	double				ApproximationScale() const = 0;
};


_END_ICON_NAMESPACE


#endif	// VERTEX_SOURCE_H
