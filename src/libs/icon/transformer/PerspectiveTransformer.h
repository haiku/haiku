/*
 * Copyright 2006-2007, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */
#ifndef PERSPECTIVE_TRANSFORMER_H
#define PERSPECTIVE_TRANSFORMER_H


#include <Rect.h>
#include <Point.h>

#include <agg_conv_transform.h>
#include <agg_trans_perspective.h>

#include "IconBuild.h"
#include "Transformer.h"
#ifdef ICON_O_MATIC
#include "Observer.h"
#endif
#include "PathTransformer.h"
#include "StyleTransformer.h"
#include "VertexSource.h"


_BEGIN_ICON_NAMESPACE

class Shape;


typedef agg::conv_transform<VertexSource, agg::trans_perspective> Perspective;

/*! Transforms from the VertexSource's bounding rect to the specified
	quadrilateral. This class watches out for invalid transformations if
	\c ICON_O_MATIC is set.
*/
class PerspectiveTransformer : public Transformer,
							   public PathTransformer,
							   public StyleTransformer,
#ifdef ICON_O_MATIC
							   public Observer,
#endif
							   public Perspective,
							   public agg::trans_perspective {
public:
	enum {
		archive_code	= 'prsp',
	};

	/*! Initializes starting with the identity transformation.
		A valid perspective transformation can be rendered invalid if the shape
		changes. Listens to \a shape for updates and determines if the
		transformation is still valid if \c ICON_O_MATIC is set.
	*/
								PerspectiveTransformer(
									VertexSource& source,
									Shape* shape);
								PerspectiveTransformer(
									VertexSource& source,
									Shape* shape,
									BMessage* archive);
								PerspectiveTransformer(
									const PerspectiveTransformer& other);

	virtual						~PerspectiveTransformer();

	// Transformer interface
	virtual	Transformer*		Clone() const;

	// PathTransformer interface
	virtual	void				rewind(unsigned path_id);
	virtual	unsigned			vertex(double* x, double* y);

	virtual	void				SetSource(VertexSource& source);

	virtual	double				ApproximationScale() const;

	// StyleTransformer interface
	virtual void				transform(double* x, double* y) const
#ifdef ICON_O_MATIC
									{ if (fValid) agg::trans_perspective::transform(x, y); }
#else
									{ agg::trans_perspective::transform(x, y); }
#endif

	/*! Inverts the perspective transformation.
		\warning This class can mostly only transform points when inverted. Most
			other features either have not been tested or are missing.
	*/
	virtual	void				Invert();

#ifdef ICON_O_MATIC
	// IconObject interface
	virtual	status_t			Archive(BMessage* into,
										bool deep = true) const;

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// PerspectiveTransformer
			void				TransformTo(BPoint leftTop, BPoint rightTop,
									BPoint leftBottom, BPoint rightBottom);

			BPoint				LeftTop()
									{ return fToLeftTop; }
			BPoint				RightTop()
									{ return fToRightTop; }
			BPoint				LeftBottom()
									{ return fToLeftBottom; }
			BPoint				RightBottom()
									{ return fToRightBottom; }

private:
			void				_CheckValidity();
#endif // ICON_O_MATIC

private:
			Shape*				fShape;
#ifdef ICON_O_MATIC
			bool				fInverted;
			BRect				fFromBox;
			BPoint				fToLeftTop;
			BPoint				fToRightTop;
			BPoint				fToLeftBottom;
			BPoint				fToRightBottom;
			bool				fValid;
#endif
};


_END_ICON_NAMESPACE


#endif	// PERSPECTIVE_TRANSFORMER_H
