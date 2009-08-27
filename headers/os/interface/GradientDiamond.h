/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRADIENT_DIAMOND_H
#define _GRADIENT_DIAMOND_H


#include <Gradient.h>


class BPoint;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku.


class BGradientDiamond : public BGradient {
public:
								BGradientDiamond();
								BGradientDiamond(const BPoint& center);
								BGradientDiamond(float cx, float cy);
	
			BPoint				Center() const;
			void				SetCenter(const BPoint& center);
			void				SetCenter(float cx, float cy);
};

#endif // _GRADIENT_DIAMOND_H
