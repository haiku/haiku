/*
 * Copyright 2009, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Brecht Machiels, brecht@mos6581.org
 */

#ifndef _BINARY_UTILS_H
#define _BINARY_UTILS_H


/* macro for a one-bit bitmask
*/

#define BIT(n)	(1ULL << n)

/* macro/templates to create bitmasks
*/

#define BITMASK(high, low)	((unsigned long long)(BitMask<high, low>::value))

template<int h, int l>
struct BitMask
{
	enum { value = (BitMask<h-l-1,0>::value +
		(1ULL << h - l)) << l };
};


template<>
struct BitMask<-1,0>
{
	enum { value = 0ULL };
};


/* macro/templates to enter binary constants
*/

#define BINARY(binstring)	((unsigned long long)(Binary<0##binstring>::value))

// http://www.eptacom.net/pubblicazioni/pub_eng/binary.html
template<const unsigned long long N> struct Binary
{
	enum { value = (N % 8ULL) + 2ULL * Binary<N / 8ULL>::value };
};


template<>
struct Binary<0>
{
	enum { value = 0ULL } ;
};


/* macro/templates to determine offset

   NOTE: broken for high bit indices
*/

#define MASKOFFSET(mask)	(MaskOffset<mask, (mask & 1UL)>::count)

template<const unsigned long mask, unsigned int firstBit>
struct MaskOffset
{
	enum { count = MaskOffset<(mask >> 1), ((mask >> 1) & 1UL)>::count + 1 };
};


template<const unsigned long mask>
struct MaskOffset<mask, 1>
{
	enum { count = 0 };
};

#endif // _BINARY_UTILS_H

