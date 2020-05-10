/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef TRANSFORMABLE_H
#define TRANSFORMABLE_H

#include <Rect.h>

#include <agg_trans_affine.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


class Transformable : public agg::trans_affine {
 public:
	enum {
		matrix_size = 6,
	};

								Transformable();
								Transformable(const Transformable& other);
	virtual						~Transformable();

			void				StoreTo(double matrix[matrix_size]) const;
			void				LoadFrom(const double matrix[matrix_size]);

								// set to or combine with other matrix
			void				SetTransform(const Transformable& other);
			Transformable&		operator=(const Transformable& other);
			Transformable&		Multiply(const Transformable& other);
	virtual	void				Reset();

			void				Invert();

			bool				IsIdentity() const;
			bool				IsTranslationOnly() const;
			bool				IsNotDistorted() const;
			bool				IsValid() const;

			bool				operator==(const Transformable& other) const;
			bool				operator!=(const Transformable& other) const;

								// transforms coordiantes
			void				Transform(double* x, double* y) const;
			void				Transform(BPoint* point) const;
			BPoint				Transform(const BPoint& point) const;

			void				InverseTransform(double* x, double* y) const;
			void				InverseTransform(BPoint* point) const;
			BPoint				InverseTransform(const BPoint& point) const;

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


_USING_ICON_NAMESPACE


#endif // TRANSFORMABLE_H

