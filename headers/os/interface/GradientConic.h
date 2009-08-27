/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRADIENT_CONIC_H
#define _GRADIENT_CONIC_H


#include <Gradient.h>


class BPoint;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku.


class BGradientConic : public BGradient {
public:
								BGradientConic();
								BGradientConic(const BPoint& center,
									float angle);
								BGradientConic(float cx, float cy,
									float angle);
	
			BPoint				Center() const;
			void				SetCenter(const BPoint& center);
			void				SetCenter(float cx, float cy);

			float				Angle() const;
			void				SetAngle(float angle);
};

#endif // _GRADIENT_CONIC_H
