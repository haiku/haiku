/*
 * Copyright 2006-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef GRADIENT_TRANSFORMABLE_H
#define GRADIENT_TRANSFORMABLE_H


#ifdef ICON_O_MATIC
# include <Archivable.h>

# include "Observable.h"
#endif // ICON_O_MATIC

#include "IconBuild.h"
#include "Transformable.h"

#include <GraphicsDefs.h>
#include <Gradient.h>
#include <List.h>

class BMessage;

enum gradients_type {
	GRADIENT_LINEAR = 0,
	GRADIENT_CIRCULAR,
	GRADIENT_DIAMOND,
	GRADIENT_CONIC,
	GRADIENT_XY,
	GRADIENT_SQRT_XY
};

enum interpolation_type {
	INTERPOLATION_LINEAR = 0,
	INTERPOLATION_SMOOTH
};


_BEGIN_ICON_NAMESPACE


#ifdef ICON_O_MATIC
class Gradient : public BArchivable,
				 public Observable,
				 public Transformable {
#else
class Gradient : public Transformable {
#endif

 public:
								Gradient(bool empty = false);
								Gradient(BMessage* archive);
								Gradient(const Gradient& other);
	virtual						~Gradient();

#ifdef ICON_O_MATIC
			status_t			Archive(BMessage* into, bool deep = true) const;
#else
	inline	void				Notify() {}
#endif

			Gradient&			operator=(const Gradient& other);

			bool				operator==(const Gradient& other) const;
			bool				operator!=(const Gradient& other) const;
			bool				ColorStepsAreEqual(
									const Gradient& other) const;

			void				SetColors(const Gradient& other);


			int32				AddColor(const rgb_color& color, float offset);
			bool				AddColor(const BGradient::ColorStop& color,
									int32 index);

			bool				RemoveColor(int32 index);

			bool				SetColor(int32 index,
									const BGradient::ColorStop& step);
			bool				SetColor(int32 index, const rgb_color& color);
			bool				SetOffset(int32 index, float offset);

			int32				CountColors() const;
			BGradient::ColorStop* ColorAt(int32 index) const;
			BGradient::ColorStop* ColorAtFast(int32 index) const;

			void				SetType(gradients_type type);
			gradients_type		Type() const
									{ return fType; }

			void				SetInterpolation(interpolation_type type);
			interpolation_type		Interpolation() const
									{ return fInterpolation; }

			void				SetInheritTransformation(bool inherit);
			bool				InheritTransformation() const
									{ return fInheritTransformation; }

			void				MakeGradient(uint32* colors,
											 int32 count) const;

			void				FitToBounds(const BRect& bounds);
			BRect				GradientArea() const;
	virtual	void				TransformationChanged();

			void				PrintToStream() const;

 private:
			void				_MakeEmpty();

			BList				fColors;
			gradients_type		fType;
			interpolation_type	fInterpolation;
			bool				fInheritTransformation;
};


_END_ICON_NAMESPACE


#endif	// GRADIENT_TRANSFORMABLE_H
