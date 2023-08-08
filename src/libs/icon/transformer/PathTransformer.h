/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PATH_TRANSFORMER_H
#define PATH_TRANSFORMER_H


#include "IconBuild.h"
#include "VertexSource.h"


_BEGIN_ICON_NAMESPACE


/*! A transformation to a VertexSource.
	It can add points, move them around, turn them to curves, etc.
*/
class PathTransformer : public VertexSource
{
public:
								PathTransformer(VertexSource& source)
									: fSource(source) {}
	virtual						~PathTransformer() {}

	// PathTransformer
	virtual	void				rewind(unsigned path_id)
									{ fSource.rewind(path_id); }
    virtual	unsigned			vertex(double* x, double* y)
									{ return fSource.vertex(x, y); }

	virtual	void				SetSource(VertexSource& source)
									{ fSource = source; }

	virtual	bool				WantsOpenPaths() const
									{ return fSource.WantsOpenPaths(); }
	virtual	double				ApproximationScale() const
									{ return fSource.ApproximationScale(); }

protected:
			VertexSource&		fSource;
};


_END_ICON_NAMESPACE

#endif	// PATH_TRANSFORMER_H
