//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, Haiku, Inc.
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		PatternHandler.h
//	Author:			DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	Class for easy calculation and use of patterns
//  
//------------------------------------------------------------------------------
#ifndef PATTERNHADLER_H
#define PATTERNHADLER_H

#include <SupportDefs.h>
#include "RGBColor.h"

class Pattern
{
public:

	Pattern(void) {} 

	Pattern(const uint64 &pat) { _pat.type64=pat; }

	Pattern(int8 *pat) { _pat.type64=*((uint64*)pat); }
	
	Pattern(const Pattern &src) { _pat.type64=src._pat.type64; }

	const int8 *GetInt8(void) const { return _pat.type8; }

	uint64 GetInt64(void) const { return _pat.type64; }

	void Set(int8 *pat) { _pat.type64=*((uint64*)pat); }

	void Set(const uint64 &pat) { _pat.type64=pat; }

	Pattern & operator=(const Pattern &from) { _pat.type64=from._pat.type64; return *this; }
	Pattern & operator=(const int64 &from) { _pat.type64=from; return *this; }
private:

	typedef union
	{
		uint64 type64;
		int8 type8[8];
	} pattern_union;

	pattern_union _pat;
};

/*!
	\brief Class for easy calculation and use of patterns
	
	PatternHandlers are designed specifically for DisplayDriver subclasses. 
	Pattern support can be easily added by setting the pattern to use via 
	SetTarget, and then merely retrieving the value for the coordinates 
	specified. 
*/
class PatternHandler
{
public:
	PatternHandler(void);
	PatternHandler(int8 *pat);
	PatternHandler(const uint64 &pat);
	PatternHandler(const Pattern &pat);
	~PatternHandler(void);
	void SetTarget(int8 *pat);
	void SetTarget(const uint64 &pat);
	void SetTarget(const Pattern &pat);
	void SetColors(const RGBColor &high, const RGBColor &low);
	RGBColor GetColor(const BPoint &pt);
	RGBColor GetColor(const float &x, const float &y);
	bool GetValue(const float &x, const float &y);
	bool GetValue(const BPoint &pt);
	pattern *GetR5Pattern(void) { return (pattern*)_pat.GetInt8(); }
private:
	Pattern _pat;
	RGBColor *_high,*_low;
};

extern const Pattern pat_solidhigh;
extern const Pattern pat_solidlow;
extern const Pattern pat_mixedcolors;

#endif
