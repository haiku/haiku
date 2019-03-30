/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#ifndef SUPPORT_H
#define SUPPORT_H

#include <Rect.h>

// constrain
inline void
constrain(float& value, float min, float max)
{
	if (value < min)
		value = min;
	if (value > max)
		value = max;
}

#ifdef __i386__

// constrain_int32_0_255_asm
inline int32
constrain_int32_0_255_asm(int32 value) {
	asm("movl  $0,    %%ecx\n\t"
		"movl  $255,  %%edx\n\t"
		"cmpl  %%ecx, %%eax\n\t"
		"cmovl %%ecx, %%eax\n\t"
		"cmpl  %%edx, %%eax\n\t"
		"cmovg %%edx, %%eax"
		: "=a" (value)
		: "a" (value) 
		: "%ecx", "%edx" );
	return value;
}

#define constrain_int32_0_255 constrain_int32_0_255_asm

#else

inline int32
constrain_int32_0_255_c(int32 value)
{
	return max_c(0, min_c(255, value));
}

#define constrain_int32_0_255 constrain_int32_0_255_c

#endif

// rect_to_int
inline void
rect_to_int(BRect r,
			int32& left, int32& top, int32& right, int32& bottom)
{
	left = (int32)floorf(r.left);
	top = (int32)floorf(r.top);
	right = (int32)ceilf(r.right);
	bottom = (int32)ceilf(r.bottom);
}


# endif // SUPPORT_H
