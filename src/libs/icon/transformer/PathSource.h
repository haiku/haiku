/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PATH_SOURCE_H
#define PATH_SOURCE_H


#include "IconBuild.h"
#include "Transformer.h"

#include "agg_path_storage.h"
#include "agg_conv_curve.h"


_BEGIN_ICON_NAMESPACE


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
	virtual	double				ApproximationScale() const;

	// PathSource
			void				Update(bool leavePathsOpen,
									double approximationScale);

			void				SetGlobalScale(double scale);

 private:
		 	PathContainer*		fPaths;
		 	AGGPath				fAGGPath;
		 	AGGCurvedPath		fAGGCurvedPath;

		 	double				fGlobalScale;
		 	double				fLastTransformerScale;
};


_END_ICON_NAMESPACE


#endif	// PATH_SOURCE_H
