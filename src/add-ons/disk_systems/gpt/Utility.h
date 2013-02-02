/*
 * Copyright 2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef UTILITY_H
#define UTILITY_H


#include <SupportDefs.h>


static inline off_t
round_down(off_t a, uint32 b)
{
	return a / b * b;
}


static inline off_t
round_up(off_t a, uint32 b)
{
	return round_down(a + b - 1, b);
}


#endif	// UTILITY_H
