/*
 * Copyright 2006-2009, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GRADIENT_LINEAR_H
#define _GRADIENT_LINEAR_H


#include <Gradient.h>


class BPoint;


// WARNING! This is experimental API and may change! Be prepared to
// recompile your software in a next version of haiku.


class BGradientLinear : public BGradient {
public:
								BGradientLinear();
								BGradientLinear(const BPoint& start,
									const BPoint& end);
								BGradientLinear(float x1, float y1,
									float x2, float y2);
	
			BPoint				Start() const;
			void				SetStart(const BPoint& start);
			void				SetStart(float x1, float y1);
	
			BPoint				End() const;
			void				SetEnd(const BPoint& end);
			void				SetEnd(float x2, float y2);
};

#endif // _GRADIENT_LINEAR_H
