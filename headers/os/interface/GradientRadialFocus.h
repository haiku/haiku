/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRADIENT_RADIAL_FOCUS_H
#define _GRADIENT_RADIAL_FOCUS_H


#include <Gradient.h>


class BPoint;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku.


class BGradientRadialFocus : public BGradient {
public:
								BGradientRadialFocus();
								BGradientRadialFocus(const BPoint& center,
									float radius, const BPoint& focal);
								BGradientRadialFocus(float cx, float cy,
									float radius, float fx, float fy);
	
			BPoint				Center() const;
			void				SetCenter(const BPoint& center);
			void				SetCenter(float cx, float cy);
	
			BPoint				Focal() const;
			void				SetFocal(const BPoint& focal);
			void				SetFocal(float fx, float fy);
	
			float				Radius() const;
			void				SetRadius(float radius);
};

#endif // _GRADIENT_RADIAL_FOCUS_H
