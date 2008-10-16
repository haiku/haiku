/*
 * Copyright 2008, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephen Deken, stephen.deken@gmail.com
 */
#ifndef _AFFINETRANSFORM_H
#define _AFFINETRANSFORM_H

#include <InterfaceDefs.h>

#include <agg_trans_affine.h>

class BPoint;

namespace BPrivate {

class BAffineTransform {
public:
									BAffineTransform();
									BAffineTransform(const BAffineTransform& copyFrom);
		virtual						~BAffineTransform();
		virtual	BAffineTransform&	operator=(const BAffineTransform& copyFrom);

		virtual	bool				operator==(
										const BAffineTransform& other) const;
		virtual	bool				operator!=(
										const BAffineTransform& other) const;

	// Callbacks
		virtual	void				TransformationChanged() const;

	// Application
				BPoint				Apply(const BPoint& point) const;
				void				Apply(BPoint* points, uint32 count) const;

	// Rotation
				void				Rotate(float angle);
				void				Rotate(const BPoint& center, float angle);
				BAffineTransform&	RotateBySelf(float angle);
				BAffineTransform&	RotateBySelf(const BPoint& center, float angle);
				BAffineTransform	RotateByCopy(float angle) const;
				BAffineTransform	RotateByCopy(const BPoint& center, float angle) const;

	// Translation
				void				Translate(float deltaX, float deltaY);
				void				Translate(const BPoint& delta);
				BAffineTransform&	TranslateBySelf(float deltaX, float deltaY);
				BAffineTransform&	TranslateBySelf(const BPoint& delta);
				BAffineTransform	TranslateByCopy(float deltaX, float deltaY) const;
				BAffineTransform	TranslateByCopy(const BPoint& delta) const;

	// Scaling
				void				Scale(float scale);
				void				Scale(const BPoint& center, float scale);
				void				Scale(float scaleX, float scaleY);
				void				Scale(const BPoint& center, float scaleX, float scaleY);
				void				Scale(const BPoint& scale);
				void				Scale(const BPoint& center, const BPoint& scale);
				BAffineTransform&	ScaleBySelf(float scale);
				BAffineTransform&	ScaleBySelf(const BPoint& center, float scale);
				BAffineTransform&	ScaleBySelf(float scaleX, float scaleY);
				BAffineTransform&	ScaleBySelf(const BPoint& center, float scaleX, float scaleY);
				BAffineTransform&	ScaleBySelf(const BPoint& scale);
				BAffineTransform&	ScaleBySelf(const BPoint& center, const BPoint& scale);
				BAffineTransform	ScaleByCopy(float scale) const;
				BAffineTransform	ScaleByCopy(const BPoint& center, float scale) const;
				BAffineTransform	ScaleByCopy(float scaleX, float scaleY) const;
				BAffineTransform	ScaleByCopy(const BPoint& center, float scaleX, float scaleY) const;
				BAffineTransform	ScaleByCopy(const BPoint& scale) const;
				BAffineTransform	ScaleByCopy(const BPoint& center, const BPoint& scale) const;

	// Shearing
				void				Shear(float shearX, float shearY);
				void				Shear(const BPoint& center, float shearX, float shearY);
				void				Shear(const BPoint& shear);
				void				Shear(const BPoint& center, const BPoint& shear);

				BAffineTransform&	ShearBySelf(float shearX, float shearY);
				BAffineTransform&	ShearBySelf(const BPoint& center, float shearX, float shearY);
				BAffineTransform&	ShearBySelf(const BPoint& shear);
				BAffineTransform&	ShearBySelf(const BPoint& center, const BPoint& shear);
				BAffineTransform	ShearByCopy(float shearX, float shearY) const;
				BAffineTransform	ShearByCopy(const BPoint& center, float shearX, float shearY) const;
				BAffineTransform	ShearByCopy(const BPoint& shear) const;
				BAffineTransform	ShearByCopy(const BPoint& center, const BPoint& shear) const;

private:
				void				_Rotate(float angle);
				void				_Scale(float scaleX, float scaleY);
				void				_Translate(float deltaX, float deltaY);
				void				_Shear(float shearX, float shearY);

				void				_TransformPoint(BPoint& point) const;

				agg::trans_affine	fTransformMatrix;
};

} // namespace BPrivate

using namespace BPrivate;

#endif // _AFFINETRANSFORM_H
