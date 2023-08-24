/*
 * Copyright 2006, 2023, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Zardshard
 */

#ifndef TRANSFORMABLE_H
#define TRANSFORMABLE_H

#include <Rect.h>

#include <agg_trans_affine.h>

#include "IconBuild.h"
#include "StyleTransformer.h"
#include "Transformer.h"


_BEGIN_ICON_NAMESPACE


/*! The standard affine transformation. */
// TODO: combine with AffineTransformer
class Transformable : public StyleTransformer,
					  public agg::trans_affine {
 public:
	enum {
		matrix_size = 6,
	};

								Transformable();
								Transformable(const Transformable& other);
	virtual						~Transformable();

	// StyleTransformer interface
	virtual	void				transform(double* x, double* y) const
									{ return agg::trans_affine::transform(x, y); }
	virtual	void				Invert();
	virtual bool				IsLinear()
									{ return true; }

	// Transformable
			void				InverseTransform(double* x, double* y) const;
			void				InverseTransform(BPoint* point) const;
			BPoint				InverseTransform(const BPoint& point) const;

			void				StoreTo(double matrix[matrix_size]) const;
			void				LoadFrom(const double matrix[matrix_size]);

								// set to or combine with other matrix
			void				SetTransform(const Transformable& other);
			Transformable&		operator=(const Transformable& other);
			Transformable&		Multiply(const Transformable& other);
	virtual	void				Reset();

			bool				IsIdentity() const;
			bool				IsTranslationOnly() const;
			bool				IsNotDistorted() const;
			bool				IsValid() const;

			bool				operator==(const Transformable& other) const;
			bool				operator!=(const Transformable& other) const;

								// transforms the rectangle "bounds" and
								// returns the *bounding box* of that
			BRect				TransformBounds(BRect bounds) const;

								// some convenience functions
	virtual	void				TranslateBy(BPoint offset);
	virtual	void				RotateBy(BPoint origin, double degrees);
	virtual	void				ScaleBy(BPoint origin, double xScale, double yScale);
	virtual	void				ShearBy(BPoint origin, double xShear, double yShear);

	virtual	void				TransformationChanged();
		// hook function that is called when the transformation
		// is changed for some reason

	virtual void				PrintToStream() const;
};


_END_ICON_NAMESPACE


#endif // TRANSFORMABLE_H
