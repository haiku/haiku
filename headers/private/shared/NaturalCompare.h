/*
 * Copyright 2009, Dana Burkart
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2010, Axel Dörfler, axeld@pinc-software.de
 * Copyright 2010, Rene Gollent (anevilyak@gmail.com)
 * Distributed under the terms of the MIT License.
 */
#ifndef _NATURAL_COMPARE_H
#define _NATURAL_COMPARE_H


namespace BPrivate {


//! Compares two strings naturally, as opposed to lexicographically
int NaturalCompare(const char* stringA, const char* stringB);


} // namespace BPrivate


#endif	// _NATURAL_COMPARE_H
