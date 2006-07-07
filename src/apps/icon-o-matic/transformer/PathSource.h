/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef PATH_SOURCE_H
#define PATH_SOURCE_H

#include "Transformer.h"

#include "agg_path_storage.h"
#include "agg_conv_curve.h"

class PathContainer;
class VectorPath;

typedef agg::path_storage					AGGPath;
typedef agg::conv_curve<AGGPath>			AGGCurvedPath;

class PathSource : public VertexSource {
 public:
								PathSource(PathContainer* paths);
	virtual						~PathSource();

    virtual	void				rewind(unsigned path_id);
    virtual	unsigned			vertex(double* x, double* y);

	virtual	bool				WantsOpenPaths() const;

	// PathSource
			void				Update(bool leavePathsOpen);

 private:
		 	PathContainer*		fPaths;
		 	AGGPath				fAGGPath;
		 	AGGCurvedPath		fAGGCurvedPath;
};

#endif // PATH_SOURCE_H
