/*
 * Copyright 2006, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef GRADIENT_H
#define GRADIENT_H

#include <GraphicsDefs.h>
#include <List.h>

#ifdef ICON_O_MATIC
# include <Archivable.h>

# include "Observable.h"
#endif ICON_O_MATIC

#include "Transformable.h"

class BMessage;

enum gradient_type {
	GRADIENT_LINEAR = 0,
	GRADIENT_CIRCULAR,
	GRADIENT_DIAMONT,
	GRADIENT_CONIC,
	GRADIENT_XY,
	GRADIENT_SQRT_XY,
};

enum interpolation_type {
	INTERPOLATION_LINEAR = 0,
	INTERPOLATION_SMOOTH,
};

struct color_step {
					color_step(const rgb_color c, float o);
					color_step(uint8 r, uint8 g, uint8 b, uint8 a, float o);
					color_step(const color_step& other);
					color_step();

			bool	operator!=(const color_step& other) const;

	rgb_color		color;
	float			offset;
};

#ifdef ICON_O_MATIC
class Gradient : public BArchivable,
				 public Observable,
				 public Transformable {
#else
class Gradient : public Transformable {
#endif

 public:
								Gradient(bool empty = false);
#ifdef ICON_O_MATIC
								Gradient(BMessage* archive);
#endif
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

			void				SetColors(const Gradient& other);


			int32				AddColor(const rgb_color& color, float offset);
			bool				AddColor(const color_step& color, int32 index);

			bool				RemoveColor(int32 index);

			bool				SetColor(int32 index, const color_step& step);
			bool				SetColor(int32 index, const rgb_color& color);
			bool				SetOffset(int32 index, float offset);

			int32				CountColors() const;
			color_step*			ColorAt(int32 index) const;
			color_step*			ColorAtFast(int32 index) const;

			void				SetType(gradient_type type);
			gradient_type		Type() const
									{ return fType; }

			void				SetInterpolation(interpolation_type type);
			interpolation_type		Interpolation() const
									{ return fInterpolation; }

			void				SetInheritTransformation(bool inherit);
			bool				InheritTransformation() const
									{ return fInheritTransformation; }

			void				MakeGradient(uint32* colors,
											 int32 count) const;

			BRect				GradientArea() const;
	virtual	void				TransformationChanged();

			void				PrintToStream() const;

 private:
			void				_MakeEmpty();

			BList				fColors;
			gradient_type		fType;
			interpolation_type	fInterpolation;
			bool				fInheritTransformation;
};


#endif // GRADIENT_H
